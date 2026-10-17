from pathlib import Path
import hashlib
import io
import json
import struct
import subprocess
import unicorn as u
from unicorn import arm_const as a
from elftools.elf.elffile import ELFFile

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests/ipc-fixture'
TLS=0x08000000
CMD=TLS+0x80
STATIC=TLS+0x180
DATA=TLS+0x1000
STACK=0x09000000
STOP=0x070F0000

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    compiler=ROOT.parent/'tools/bin/arm-none-eabi-gcc.exe'
    support=OUT/'support.c'
    support.write_bytes(b'#include <3ds.h>\n#include <stddef.h>\n#include <errno.h>\nResult __wrap_svcSendSyncRequest(Handle h){(void)h;return 0;}\nvoid *memcpy(void *d,const void *s,size_t n){unsigned char *o=d;const unsigned char *i=s;while(n--)*o++=*i++;return d;}\nvoid *memset(void *d,int v,size_t n){unsigned char *o=d;while(n--)*o++=(unsigned char)v;return d;}\nint test_inprogress(void){return EINPROGRESS;}\n')
    target=OUT/'ipc.elf'
    args=[str(compiler),'-std=gnu11','-Os','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-ffunction-sections','-fdata-sections','-nostdlib','-march=armv6k','-mfpu=vfp','-mfloat-abi=hard','-mtp=soft','-D__3DS__','-I'+str(ROS/'include'),'-I'+str(ROOT/'.libctru/v2.7.0/include'),str(ROS/'source/minisoc.c'),str(support),'-Wl,--wrap=svcSendSyncRequest,--gc-sections,-e,socResolveIPv4,-u,socSetNonBlocking,-u,socConnectAsync,-u,test_inprogress,-Ttext,0x07000000',str(ROOT/'.libctru/v2.7.0/lib/libctru.a'),'-lgcc','-o',str(target)]
    result=subprocess.run(args,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    elf=ELFFile(io.BytesIO(target.read_bytes()))
    symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
    uc=u.Uc(u.UC_ARCH_ARM,u.UC_MODE_ARM)
    for start,size in ((0x07000000,0x100000),(TLS,0x10000),(STACK,0x10000)):uc.mem_map(start,size)
    for segment in elf.iter_segments():
        if segment['p_type']=='PT_LOAD' and segment['p_filesz']:uc.mem_write(segment['p_vaddr'],segment.data())
    uc.reg_write(a.UC_ARM_REG_C13_C0_3,TLS)
    uc.mem_write(symbols['miniSocEnabled'],b'\x01')
    def word(address):return struct.unpack('<I',uc.mem_read(address,4))[0]
    def put(address,value):uc.mem_write(address,struct.pack('<I',value&0xFFFFFFFF))
    count=0
    replies={}
    observations=[]
    def ipc(machine,address,size,data):
        command=word(CMD)
        observations.append([word(CMD+i*4) for i in range(7)])
        kind=command>>16
        if kind==0xD:
            output=word(STATIC+4)
            uc.mem_write(output,b'\0'*0x1A88)
            put(output+4,replies.get('count',1))
            for index,ip in enumerate(replies.get('ips',[0x0101A8C0])):
                put(output+0x1908+index*0x10,ip)
        if kind==6:
            observations[-1].append(bytes(uc.mem_read(word(CMD+24),8)).hex())
        put(CMD+4,replies.get('service',0));put(CMD+8,replies.get('network',0))
        uc.reg_write(a.UC_ARM_REG_R0,replies.get('svc',0)&0xFFFFFFFF)
        uc.reg_write(a.UC_ARM_REG_PC,uc.reg_read(a.UC_ARM_REG_LR))
    uc.hook_add(u.UC_HOOK_CODE,ipc,begin=symbols['__wrap_svcSendSyncRequest'],end=symbols['__wrap_svcSendSyncRequest'])
    def call(name,*args):
        nonlocal count
        count+=1
        for reg in range(a.UC_ARM_REG_R0,a.UC_ARM_REG_R12+1):uc.reg_write(reg,0)
        for reg,value in zip((a.UC_ARM_REG_R0,a.UC_ARM_REG_R1,a.UC_ARM_REG_R2,a.UC_ARM_REG_R3),args):uc.reg_write(reg,value)
        uc.reg_write(a.UC_ARM_REG_SP,STACK+0xFF00);uc.reg_write(a.UC_ARM_REG_LR,STOP)
        uc.emu_start(symbols[name],STOP,count=1000000)
        assert uc.reg_read(a.UC_ARM_REG_PC)==STOP
        return struct.unpack('<i',struct.pack('<I',uc.reg_read(a.UC_ARM_REG_R0)))[0]
    hostname=b'api.github.com\0';uc.mem_write(DATA,hostname)
    saved=(0x12345678,0x87654321)
    def dns(**values):
        replies.clear();replies.update(values)
        put(STATIC,saved[0]);put(STATIC+4,saved[1])
        result=call('socResolveIPv4',DATA,DATA+512)
        assert (word(STATIC),word(STATIC+4))==saved
        return result
    assert dns()==1 and word(DATA+512)==0x0101A8C0
    request=observations[-1]
    assert request[0]==0xD0082 and request[1]==len(hostname) and request[2]==0x1A88 and request[3]==(len(hostname)<<14|0xC02) and request[4]==DATA
    for values in ({'svc':0xD8000001},{'service':0xD8000001},{'network':-1},{'count':0},{'count':17}):assert dns(**values)<0
    assert dns(count=6,ips=[0,0xFFFFFFFF,0x0101A8C0,0x0101A8C0,0x0201A8C0,0x0301A8C0])==3
    assert dns(count=16,ips=list(range(1,17)))==4
    for name in (b'',b'a'*254,b'a/b',b'a b',b'a@b',b'a:b',b'\x80'):
        uc.mem_write(DATA,name+b'\0');before=len(observations);assert call('socResolveIPv4',DATA,DATA+512)<0 and len(observations)==before
    uc.mem_write(DATA,hostname)
    replies.clear();assert call('socSetNonBlocking',17)==0
    assert observations[-1][:5]==[0x1300C2,17,4,4,0x20]
    for values in ({'svc':0xD8000001},{'service':0xD8000001},{'network':-6}):
        replies.clear();replies.update(values);assert call('socSetNonBlocking',17)<0
    before=len(observations);assert call('socSetNonBlocking',0xFFFFFFFF)<0 and len(observations)==before
    address=struct.pack('<HHI8s',2,0xBB01,0x0101A8C0,b'\0'*8);uc.mem_write(DATA,address)
    replies.clear();assert call('socConnectAsync',17,DATA,16)==0
    assert observations[-1][:4]==[0x60084,17,16,0x20]
    assert observations[-1][5]==(8<<14|2) and observations[-1][-1]=='080201bbc0a80101'
    replies['network']=-26
    expected=call('test_inprogress')
    assert call('socConnectAsync',17,DATA,16)==-expected
    for length in (0,1,7,17,0xFFFFFFFF):
        before=len(observations);assert call('socConnectAsync',17,DATA,length)<0 and len(observations)==before
    put(DATA,10);before=len(observations);assert call('socConnectAsync',17,DATA,16)<0 and len(observations)==before
    uc.mem_write(symbols['miniSocEnabled'],b'\x00')
    for name,args2 in (('socResolveIPv4',(DATA,DATA+512)),('socSetNonBlocking',(17,)),('socConnectAsync',(17,DATA,16))):
        before=len(observations);assert call(name,*args2)<0 and len(observations)==before
    report=dict(result='pass',arm_calls=count,ipc_requests=len(observations),source_sha256=hashlib.sha256((ROS/'source/minisoc.c').read_bytes()).hexdigest(),hardware_tested=False,boundary='actual ARM miniSoc helpers and libctru error conversion; SVC response mocked; TLS static descriptors checked and restored on failures')
    (OUT.parent/'ipc-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))

if __name__=='__main__':main()
