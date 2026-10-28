from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROS = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina'
OUT = ROOT / 'build/streaming-pacing-tests'
SOURCE = ROS / 'source/remote_play.c'
raw = SOURCE.read_text()
sha = lambda value: hashlib.sha256(value).hexdigest()
parser = argparse.ArgumentParser()
parser.add_argument('--arm-cc', type=Path)
parser.add_argument('--ctrulib', type=Path)
args = parser.parse_args()
OUT.mkdir(parents=True, exist_ok=True)


def function(name):
    start = raw.rfind('\n', 0, raw.index(name + '(')) + 1
    opened = raw.index('{', start)
    end, depth = opened + 1, 1
    while depth:
        depth += (raw[end] == '{') - (raw[end] == '}')
        end += 1
    return raw[start:end] + '\n'


def declaration(name):
    start = raw.index('typedef struct ' + name + '\n')
    end = raw.index('} ' + name + ';', start) + len('} ' + name + ';')
    return raw[start:end] + '\n'


reference = (ROOT / 'tests/fixtures/streaming-relative-send.c').read_text()
assert sha(reference.replace('referenceSendFrame(', 'sendFrame(').encode()) == '0e4cf6e8ca52317259279ec3570a48dc40732e4360075c4c5b570165eaa33a30'
defines = '\n'.join(line for line in raw.splitlines() if line.startswith('#define ') and line.split()[1] in {'VIDEO_PACKET_SIZE','VIDEO_HEADER_SIZE','JPEG_SLOT_CAP','TICKS_PER_US'})
prefix = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "remote_play.h"
#ifndef PACING_ARM
#include <stdio.h>
#include <stdlib.h>
#endif
#define SYSCLOCK_ARM11 268123480ULL
#define AF_INET 2
#define MAX_PACKETS 512
typedef int LightLock;
struct in_addr {u32 s_addr;};
struct sockaddr {u16 sa_family;char bytes[14];};
struct sockaddr_in {u16 sin_family,sin_port;struct in_addr sin_addr;char padding[8];};
typedef struct {u32 payload,qos,late,coarse,cost,frames,pauseAfter,failAt;int failValue;u64 pause;bool newModel,wrap,stopSleep,linkSleep,stopSend;} Scenario;
typedef struct {u32 count,sleeps,reads,errors;u64 ticks,hash,last;u64 at[MAX_PACKETS];u32 bytes[MAX_PACKETS];} Trace;
static LightLock settingsLock;
static RemotePlayStatus status;
static bool isN3DS;
static volatile bool stopRequested,linkDown;
static int videoSock=42;
static Scenario scenario;
static Trace trace,prior;
static u64 tick,elapsedTicks,audioNext,audioCalls,audioMaxDelay;
static u32 tests,oldReferences;
u32 pacingChecks,pacingFailure;
static void check(bool condition,u32 line){++pacingChecks;if(!condition){pacingFailure=line;
#ifndef PACING_ARM
fprintf(stderr,"pacing line %u; model=%u late=%u coarse=%u cost=%u count=%u\n",line,isN3DS,scenario.late,scenario.coarse,scenario.cost,trace.count);
#endif
__builtin_trap();}}
#define CHECK(value) check((value),__LINE__)
static u64 usTicks(u64 value){return (value*SYSCLOCK_ARM11+999999)/1000000;}
static void audioService(bool yielded,u64 previous){
const u64 period=SYSCLOCK_ARM11*160/32728;
while(audioNext<=elapsedTicks){u64 delay=yielded&&audioNext>=previous?0:elapsedTicks-audioNext;if(delay>audioMaxDelay)audioMaxDelay=delay;++audioCalls;audioNext+=period;}}
static void advance(u64 value,bool yielded){u64 previous=elapsedTicks;tick+=value;elapsedTicks+=value;audioService(yielded,previous);}
static u64 svcGetSystemTick(void){++trace.reads;return tick;}
static void svcSleepThread(s64 ns){
CHECK(ns>0&&ns<=10000000&&!settingsLock);
u64 duration=((u64)ns*SYSCLOCK_ARM11+999999999)/1000000000;
u64 quantum=usTicks(scenario.coarse);if(quantum)duration=((duration+quantum-1)/quantum)*quantum;
advance(duration+usTicks(scenario.late),true);++trace.sleeps;
if(scenario.stopSleep)stopRequested=true;
if(scenario.linkSleep)linkDown=true;
}
static void LightLock_Lock(LightLock *lock){CHECK(!*lock);*lock=1;}
static void LightLock_Unlock(LightLock *lock){CHECK(*lock);*lock=0;}
static u16 htons(u16 value){return (u16)(value<<8|value>>8);}
static int socSendto(int sock,const void *bytes,u32 size,int flags,const struct sockaddr *address,u32 addressSize){
CHECK(sock==42&&!flags&&!settingsLock&&addressSize==sizeof(struct sockaddr_in));
CHECK(size>4&&size<=1448&&trace.count<MAX_PACKETS);
const struct sockaddr_in *destination=(const void *)address;CHECK(destination->sin_family==AF_INET&&htons(destination->sin_port)==8001);
const u8 *packet=bytes;CHECK(packet[2]==2&&(packet[1]&0xee)==0);
u32 index=trace.count++;trace.at[index]=elapsedTicks;trace.bytes[index]=size;
for(u32 i=0;i<size;++i)trace.hash=(trace.hash^packet[i])*1099511628211ULL;
advance(usTicks(scenario.cost),false);
if(scenario.pauseAfter==trace.count)advance(scenario.pause,true);
if(scenario.stopSend)stopRequested=true;
return scenario.failAt==trace.count?scenario.failValue:(int)size;
}
#ifdef PACING_ARM
void *memcpy(void *out,const void *in,size_t size){u8 *dst=out;const u8 *src=in;for(size_t i=0;i<size;++i)dst[i]=src[i];return out;}
void *memset(void *out,int value,size_t size){u8 *dst=out;for(size_t i=0;i<size;++i)dst[i]=(u8)value;return out;}
int memcmp(const void *left,const void *right,size_t size){const u8 *a=left,*b=right;for(size_t i=0;i<size;++i)if(a[i]!=b[i])return a[i]<b[i]?-1:1;return 0;}
#endif
'''
tests = r'''
static JpegSlot slot;
static void reset(Scenario value){
scenario=value;isN3DS=value.newModel;settingsLock=0;stopRequested=linkDown=false;
memset(&trace,0,sizeof(trace));memset(&status,0,sizeof(status));memset(&sender,0,sizeof(sender));
tick=value.wrap?UINT64_MAX-usTicks(100):SYSCLOCK_ARM11;elapsedTicks=audioCalls=audioMaxDelay=0;audioNext=SYSCLOCK_ARM11*160/32728;
slot.size=value.payload;slot.top=true;slot.frameId=1;slot.destIp=0x01020304;
slot.packetIntervalTicks=(SYSCLOCK_ARM11*VIDEO_PACKET_SIZE+value.qos-1)/value.qos;
sender.lastPacketTick=tick-slot.packetIntervalTicks;
for(u32 i=0;i<slot.size;++i)slot.data[i]=(u8)(i*37+i/1444);
}
static void budget(void){
u32 shortRun=1;u64 maximum=0;
for(u32 i=0;i<trace.count;++i){
if(i){u64 gap=trace.at[i]-trace.at[i-1];shortRun=gap<slot.packetIntervalTicks/2?shortRun+1:1;if(isN3DS)CHECK(shortRun<=2);}
u64 bytes=0;for(u32 j=i;j<trace.count;++j){bytes+=trace.bytes[j];u64 elapsed=trace.at[j]-trace.at[i];u64 allowance=elapsed*scenario.qos/SYSCLOCK_ARM11+2*VIDEO_PACKET_SIZE;CHECK(bytes<=allowance);if(bytes>maximum)maximum=bytes;}}
CHECK(maximum>0&&audioMaxDelay<=usTicks(scenario.cost));
}
static void run(Scenario value,bool legacy){
reset(value);
for(u32 frame=0;frame<value.frames;++frame){slot.frameId=(u8)(frame+1);CHECK((legacy?referenceSendFrame(&slot):sendFrame(&slot))==1);}
trace.ticks=elapsedTicks;trace.last=sender.lastPacketTick;trace.errors=status.sendErrors;
CHECK(trace.count==value.frames*((value.payload+1443)/1444));
CHECK(sender.lastPacketTick==tick-usTicks(value.cost));
budget();++tests;
}
static void pair(Scenario value){
run(value,false);
if(!value.newModel){memcpy(&prior,&trace,sizeof(trace));run(value,true);CHECK(!memcmp(&prior,&trace,sizeof(trace)));++oldReferences;}
}
static void transitions(bool newModel){
Scenario value={.payload=65536,.qos=2097152,.late=500,.cost=20,.frames=1,.newModel=newModel};
reset(value);CHECK(sendFrame(&slot)==1);u64 last=elapsedTicks;
advance(usTicks(5000000),true);u32 before=trace.count;CHECK(sendFrame(&slot)==1);
CHECK(trace.at[before]>=last+usTicks(5000000));CHECK(trace.at[before+1]-trace.at[before]>=slot.packetIntervalTicks);
before=trace.count;u64 previous=sender.lastPacketTick;slot.packetIntervalTicks=(SYSCLOCK_ARM11*VIDEO_PACKET_SIZE+512000-1)/512000;
scenario.late=0;CHECK(sendFrame(&slot)==1);CHECK((sender.lastPacketTick-previous)>=slot.packetIntervalTicks);CHECK(trace.at[before]-trace.at[before-1]>=slot.packetIntervalTicks);
for(u32 i=0;i<2;++i){reset(value);if(i)linkDown=true;else stopRequested=true;CHECK(sendFrame(&slot)==0&&!trace.count);}
reset(value);scenario.stopSleep=true;sender.lastPacketTick=tick;CHECK(sendFrame(&slot)==0&&!trace.count&&trace.sleeps==1);
reset(value);scenario.linkSleep=true;sender.lastPacketTick=tick;CHECK(sendFrame(&slot)==0&&!trace.count&&trace.sleeps==1);
reset(value);scenario.stopSend=true;CHECK(sendFrame(&slot)==0&&trace.count==1);
const int errors[]={-EAGAIN,-EWOULDBLOCK,-EINTR,-ECONNRESET,7};
for(u32 i=0;i<sizeof(errors)/sizeof(*errors);++i)for(u32 at=1;at<=3;++at){reset(value);scenario.failAt=at;scenario.failValue=errors[i];CHECK(sendFrame(&slot)==(i<3?0:-1));CHECK(trace.count==at&&status.sendErrors==1);scenario.failAt=0;before=trace.count;CHECK(sendFrame(&slot)==1);CHECK(trace.at[before]-trace.at[before-1]>=slot.packetIntervalTicks);}
++tests;
}
u32 pacing_tests(u32 full){
const u32 qos[]={512000,2097152,4194304},late[]={0,100,250,500,1000,4000},cost[]={0,20,5000},payload[]={1445,32768,65536};
for(u32 model=0;model<2;++model)for(u32 q=0;q<3;++q)for(u32 l=0;l<6;++l)for(u32 c=0;c<3;++c)for(u32 p=0;p<3;++p)for(u32 wrap=0;wrap<2;++wrap){
if(!full&&(q==0||l==1||l==2||l==5||c==2||p==1))continue;
Scenario value={.payload=payload[p],.qos=qos[q],.late=late[l],.cost=cost[c],.frames=2,.newModel=model!=0,.wrap=wrap!=0};pair(value);}
for(u32 model=0;model<2;++model){
Scenario coarse={.payload=65536,.qos=4194304,.coarse=1000,.cost=20,.frames=2,.newModel=model!=0};pair(coarse);
Scenario pause={.payload=65536,.qos=2097152,.late=500,.cost=20,.frames=2,.pauseAfter=4,.pause=SYSCLOCK_ARM11*5,.newModel=model!=0};pair(pause);
transitions(model!=0);}
return pacingChecks;
}
#ifndef PACING_ARM
int main(int argc,char **argv){
if(argc==5){Scenario value={.payload=65536,.qos=(u32)strtoul(argv[1],0,10),.late=(u32)strtoul(argv[2],0,10),.cost=20,.frames=3,.newModel=strtoul(argv[3],0,10)!=0,.wrap=strtoul(argv[4],0,10)!=0};run(value,false);printf("{\"qos\":%u,\"late_us\":%u,\"new\":%s,\"elapsed_ms\":%.9f,\"send_stage_only_fps\":%.9f}\n",value.qos,value.late,value.newModel?"true":"false",(double)trace.ticks*1000/SYSCLOCK_ARM11,3.0*SYSCLOCK_ARM11/(double)trace.ticks);return 0;}
u32 count=pacing_tests(1);printf("{\"passed\":true,\"checks\":%u,\"scenarios\":%u,\"old_reference_scenarios\":%u}\n",count,tests,oldReferences);return 0;
}
#endif
'''
fixture = prefix + '\n' + defines + '\n' + declaration('JpegSlot') + declaration('SenderState') + '\nstatic SenderState sender;\n#define snd sender\n' + ''.join(function(name) for name in ['ticksToUs','statusIncrement','transient','sendFrame']) + reference + tests
path = OUT / 'pacing.c'
path.write_text(fixture, encoding='utf-8', newline='\n')
shim = OUT / 'include/3ds/types.h'
shim.parent.mkdir(parents=True, exist_ok=True)
shim.write_text('#pragma once\n#include <stdint.h>\n#include <stdbool.h>\ntypedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;typedef int64_t s64;typedef int32_t Result;\n',encoding='utf-8',newline='\n')
compiler = 'C:/msys64/ucrt64/bin/gcc.exe' if os.name == 'nt' else os.environ.get('CC','cc')
env = dict(os.environ, PATH=str(Path(compiler).parent)+os.pathsep+os.environ.get('PATH',''))
target = OUT / ('pacing.exe' if os.name == 'nt' else 'pacing')
compiled = subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(OUT/'include'),'-I'+str(ROS/'include'),str(path),'-o',str(target)],env=env,capture_output=True,text=True)
assert compiled.returncode == 0, compiled.stdout+compiled.stderr
result = subprocess.run([str(target)],env=env,capture_output=True,text=True,timeout=60)
assert result.returncode == 0, result.stdout+result.stderr
report = json.loads(result.stdout)
report['samples'] = []
for qos in [2097152,4194304]:
    for late in [0,500,1000]:
        for model in [0,1]:
            result = subprocess.run([str(target),str(qos),str(late),str(model),'0'],env=env,capture_output=True,text=True)
            assert result.returncode == 0, result.stdout+result.stderr
            report['samples'].append(json.loads(result.stdout))
if args.arm_cc:
    assert args.ctrulib and (args.ctrulib/'include/3ds/types.h').is_file()
    from elftools.elf.elffile import ELFFile
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE, UC_HOOK_INTR
    from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC
    elf_path = OUT/'pacing-arm.elf'
    command = [str(args.arm_cc),'-std=gnu11','-Os','-marm','-march=armv6k','-mtune=mpcore','-mfpu=vfp','-mfloat-abi=hard','-mtp=soft','-fno-builtin','-ffunction-sections','-fdata-sections','-DPACING_ARM','-I'+str(args.ctrulib/'include'),'-I'+str(ROS/'include'),'-nostdlib',str(path),'-Wl,-e,pacing_tests,-Ttext=0x100000,--gc-sections','-lgcc','-o',str(elf_path)]
    result = subprocess.run(command,capture_output=True,text=True)
    assert result.returncode == 0, result.stdout+result.stderr
    with elf_path.open('rb') as stream:
        elf = ELFFile(stream)
        loads = [(segment['p_vaddr'],segment['p_memsz'],segment.data()) for segment in elf.iter_segments() if segment['p_type']=='PT_LOAD']
        symbols = {item.name:item['st_value'] for item in elf.get_section_by_name('.symtab').iter_symbols() if item.name}
    cpu = Uc(UC_ARCH_ARM,UC_MODE_ARM)
    low = min(row[0] for row in loads)&~4095
    high = (max(row[0]+row[1] for row in loads)+4095)&~4095
    cpu.mem_map(low,high-low)
    for address,_,data in loads:cpu.mem_write(address,data)
    cpu.mem_map(0x03000000,0x20000)
    cpu.mem_map(0x06000000,4096)
    def boundary(cpu,address,size,unused):
        a,b,n = (cpu.reg_read(reg) for reg in [UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2])
        if address==symbols['memcpy']:cpu.mem_write(a,bytes(cpu.mem_read(b,n)));value=a
        elif address==symbols['memset']:cpu.mem_write(a,bytes([b&255])*n);value=a
        else:
            left,right = bytes(cpu.mem_read(a,n)),bytes(cpu.mem_read(b,n));value=0 if left==right else (-1 if left<right else 1)
        cpu.reg_write(UC_ARM_REG_R0,value&0xffffffff)
        cpu.reg_write(UC_ARM_REG_PC,cpu.reg_read(UC_ARM_REG_LR))
    for name in ['memcpy','memset','memcmp']:
        cpu.hook_add(UC_HOOK_CODE,boundary,begin=symbols[name],end=symbols[name])
    failures=[]
    def interrupt(cpu,number,unused):failures.append(number);cpu.emu_stop()
    cpu.hook_add(UC_HOOK_INTR,interrupt)
    cpu.reg_write(UC_ARM_REG_SP,0x0301fff0)
    cpu.reg_write(UC_ARM_REG_LR,0x06000000)
    cpu.reg_write(UC_ARM_REG_R0,0)
    cpu.emu_start(symbols['pacing_tests'],0x06000000,count=500000000)
    failed = int.from_bytes(cpu.mem_read(symbols['pacingFailure'],4),'little')
    assert not failures and not failed and cpu.reg_read(UC_ARM_REG_PC)==0x06000000,(failures,failed,hex(cpu.reg_read(UC_ARM_REG_PC)))
    report['arm'] = dict(passed=True,checks=cpu.reg_read(UC_ARM_REG_R0),elf_sha256=sha(elf_path.read_bytes()),sdk_headers=True,actual_arm_instructions=True,memory_primitives_are_host_boundaries=True,hardware_tested=False)
report.update(source_sha256=sha(SOURCE.read_bytes()),reference_function_sha256='0e4cf6e8ca52317259279ec3570a48dc40732e4360075c4c5b570165eaa33a30',producer_sha256=sha(Path(__file__).read_bytes()),actual_C=True,old_relative_trace_identical=True,packet_window_extra_bytes=2*1448,maximum_catchup_packets=2,audio_service_is_explicit_priority_model=True,hardware_tested=False,scope='Actual new sendFrame and frozen relative reference run against delayed/quantized sleeps, UDP cost, long descheduling, unsigned clock wrap, frame/interval reset, partial/transient failures, stop/link checks and explicit higher-priority audio-service opportunities. Wire-window bounds are checked at every packet pair. No physical scheduler, Wi-Fi or acoustic claim; FPS samples exclude capture/JPEG and real resource contention.')
(OUT/'proof.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({key:report[key] for key in ['passed','checks','scenarios','old_reference_scenarios','old_relative_trace_identical','maximum_catchup_packets']} | ({'arm':report['arm']} if 'arm' in report else {})))
