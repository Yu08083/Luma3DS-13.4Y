from pathlib import Path
import argparse
import ctypes as c
import hashlib
import json
import os
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[2]
APP=ROOT/'updater'
CORE=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=APP/'build/firm-gate-test'
READ=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_uint32,c.c_void_p,c.c_uint32)

def fixture(last=0x3BC00):
    sizes=(0x6000,0x2400,0x4800,0x2400,last)
    data=bytearray(512+sum(sizes));data[:4]=b'FIRM'
    struct.pack_into('<I',data,12,0x18180000)
    struct.pack_into('<4I',data,0x40,512,0x18180000,sum(sizes),0)
    position=512
    for name,size in zip((b'loader',b'sm',b'pm',b'pxi',b'rosalina'),sizes):
        data[position+256:position+260]=b'NCCH'
        struct.pack_into('<I',data,position+260,size//512)
        data[position+512:position+512+len(name)]=name
        position+=size
    return data

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--firm',type=Path);parser.add_argument('--expect-reject',action='store_true');args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    target=OUT/('gate.dll' if os.name=='nt' else 'gate.so')
    result=subprocess.run([compiler,'-std=c11','-O2','-shared','-fPIC','-Wall','-Wextra','-Werror','-I'+str(APP/'include'),'-I'+str(CORE/'include'),str(APP/'source/firm_gate.c'),str(CORE/'source/updater_format.c'),'-o',str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    directory=os.add_dll_directory(str(Path(compiler).parent)) if os.name=='nt' else None
    lib=c.CDLL(str(target));lib.UpdaterFirm_Gate.argtypes=[READ,c.c_void_p,c.c_uint32];lib.UpdaterFirm_Gate.restype=c.c_int
    cases=0
    def run(data,failure=None):
        nonlocal cases
        def read(_,offset,out,size):
            if failure is not None and offset>=failure:return False
            if offset+size>len(data):return False
            c.memmove(out,bytes(data[offset:offset+size]),size);return True
        callback=READ(read);cases+=1;return lib.UpdaterFirm_Gate(callback,None,len(data))
    good=fixture();assert run(good)==0
    assert run(fixture(0x5EA00))==-6
    assert run(fixture(0x3BE00))==-6
    for index,value in ((0,0),(12,0),(0x44,0x18000000),(0x48,0xFFFFFFFF),(0x4C,3),(0x300,0),(0x304,0),(0x304,0xFFFFFFFF),(0x38C,0x00010000)):
        bad=bytearray(good);struct.pack_into('<I',bad,index,value);assert run(bad)<0
    for name in (b'unknown\0',b'loaderX\0',b'\0'*8):
        bad=bytearray(good);bad[0x400:0x408]=name;assert run(bad)<0
    for size in (0,100,511,len(good)-1):assert run(good[:size])<0
    for at in (0,512,0x6200):assert run(good,at)==-9
    if args.firm:assert (run(args.firm.read_bytes())<0)==args.expect_reject
    report=dict(result='pass',checks=cases,baseline=hex(0x4AC00),source_sha256=hashlib.sha256((APP/'source/firm_gate.c').read_bytes()).hexdigest(),hardware_tested=False)
    (OUT/'proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))

if __name__=='__main__':main()
