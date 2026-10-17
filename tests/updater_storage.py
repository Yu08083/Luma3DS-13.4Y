from pathlib import Path
import copy
import ctypes as c
import hashlib
import json
import os
import shutil
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-storage-tests'
BOOT='/boot.firm';TEMP='/boot.firm.13.4Y.tmp';BACKUP='/boot.firm.13.4Y.bak';BAKTEMP='/boot.firm.13.4Y.bak.tmp';PREVIOUS='/boot.firm.13.4Y.previous'
PATHS={BOOT,TEMP,BACKUP,BAKTEMP,PREVIOUS}
class Manifest(c.Structure):
    _fields_=[('version',c.c_char*64),('size',c.c_uint32),('sha256',c.c_ubyte*32)]
Begin=c.CFUNCTYPE(c.c_bool,c.c_void_p)
End=c.CFUNCTYPE(None,c.c_void_p)
Exists=c.CFUNCTYPE(c.c_int,c.c_void_p,c.c_char_p,c.POINTER(c.c_uint64))
Open=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_char_p,c.c_bool,c.POINTER(c.c_uint32))
Read=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_uint32,c.c_uint64,c.c_void_p,c.c_uint32,c.POINTER(c.c_uint32))
Write=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_uint32,c.c_uint64,c.c_void_p,c.c_uint32,c.POINTER(c.c_uint32))
Handle=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_uint32)
Rename=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_char_p,c.c_char_p)
Remove=c.CFUNCTYPE(c.c_bool,c.c_void_p,c.c_char_p)
Cancel=c.CFUNCTYPE(c.c_bool,c.c_void_p)
Progress=c.CFUNCTYPE(None,c.c_void_p,c.c_int,c.c_uint32,c.c_uint32)
class Io(c.Structure):
    _fields_=[('context',c.c_void_p),('begin',Begin),('end',End),('exists',Exists),('open',Open),('read',Read),('write',Write),('flush',Handle),('close',Handle),('rename',Rename),('remove',Remove)]
class Hooks(c.Structure):
    _fields_=[('context',c.c_void_p),('cancel',Cancel),('progress',Progress)]

def firm(seed):
    data=bytearray((index*17+seed)%256 for index in range(32768));data[:512]=bytes(512);data[:4]=b'FIRM'
    struct.pack_into('<II',data,8,0x1FF80000,0x08001000)
    for i,(offset,address,size) in enumerate(((4096,0x08001000,8192),(16384,0x1FF80000,512))):
        struct.pack_into('<4I',data,0x40+i*0x30,offset,address,size,i)
        data[0x50+i*0x30:0x70+i*0x30]=hashlib.sha256(data[offset:offset+size]).digest()
    return bytes(data)
def manifest(data):
    out=Manifest();out.version=b'v13.4Y-fix2';out.size=len(data);out.sha256[:]=hashlib.sha256(data).digest();return out

class Disk:
    def __init__(self,lib,old=None):
        self.lib=lib;self.files={'/luma/config.ini':b'user colors','/luma/RosaliaTheme/a.png':b'user theme'}
        if old is not None:self.files[BOOT]=old
        self.handles={};self.next=1;self.trace=[];self.failure=0;self.short=0;self.bad_flush=False;self.corrupt_installed=False;self.snapshots=[]
        self.cancel_stage=None;self.cancelled=False;self.stages=[];self.selected_failures=set();self.failure_predicate=None
        self.callbacks=[Begin(self.begin),End(self.end),Exists(self.exists),Open(self.open),Read(self.read),Write(self.write),Handle(self.flush),Handle(self.close),Rename(self.rename),Remove(self.remove)]
        self.io=Io(None,*self.callbacks);self.cancel_fn=Cancel(lambda _:self.cancelled);self.progress_fn=Progress(self.progress)
        self.hooks=Hooks(None,self.cancel_fn,self.progress_fn)
        lib.UpdateStorage_TestIo(c.byref(self.io))
    def path(self,value):
        path=value.decode();assert path in PATHS,path;return path
    def operation(self,*operation):
        self.trace.append(operation);index=len(self.trace)
        return index!=self.failure and index not in self.selected_failures and not (self.failure_predicate and self.failure_predicate(operation))
    def begin(self,_):return self.operation('begin')
    def end(self,_):self.operation('end')
    def exists(self,_,path,size):
        path=self.path(path)
        if not self.operation('exists',path):return -1
        if path not in self.files:return 0
        size[0]=len(self.files[path]);return 1
    def open(self,_,path,write,handle):
        path=self.path(path)
        if not self.operation('open',path,bool(write)):return False
        if not write and path not in self.files:return False
        if write:self.files[path]=b''
        handle[0]=self.next;self.handles[self.next]=(path,bool(write));self.next+=1;return True
    def read(self,_,handle,offset,data,size,actual):
        path,_mode=self.handles[handle]
        if not self.operation('read',path,int(offset),int(size)):return False
        raw=self.files[path][offset:offset+size]
        if len(self.trace)==self.short:raw=raw[:-1]
        c.memmove(data,raw,len(raw));actual[0]=len(raw);return True
    def write(self,_,handle,offset,data,size,actual):
        path,mode=self.handles[handle];assert mode
        if not self.operation('write',path,int(offset),int(size)):return False
        raw=c.string_at(data,size)
        if len(self.trace)==self.short:raw=raw[:-1]
        prior=self.files[path];self.files[path]=prior[:offset]+raw+prior[offset+len(raw):];actual[0]=len(raw);return True
    def flush(self,_,handle):
        path,_mode=self.handles[handle]
        if not self.operation('flush',path):return False
        if self.bad_flush and path==TEMP:self.files[path]=self.files[path][:-1]+b'\xff'
        return True
    def close(self,_,handle):
        if handle not in self.handles:return False
        path,_mode=self.handles[handle]
        if not self.operation('close',path):return False
        del self.handles[handle];return True
    def rename(self,_,source,target):
        source,target=self.path(source),self.path(target)
        if not self.operation('rename',source,target):return False
        if source not in self.files or target in self.files:return False
        self.files[target]=self.files.pop(source)
        if self.corrupt_installed and target==BOOT and source==TEMP:self.files[target]=self.files[target][:-1]+b'\xff'
        self.snapshots.append(copy.deepcopy(self.files));return True
    def remove(self,_,path):
        path=self.path(path)
        if not self.operation('remove',path):return False
        if path not in self.files:return False
        del self.files[path];return True
    def progress(self,_,stage,current,total):
        assert current<=total
        self.stages.append((stage,int(current),int(total)))
        if stage==self.cancel_stage:self.cancelled=True
    def run(self,data,meta=None):
        meta=meta or manifest(data)
        result=self.lib.UpdateStorage_Begin(c.byref(meta),c.byref(self.hooks))
        if result==0:
            for offset in range(0,len(data),7001):
                raw=data[offset:offset+7001];result=self.lib.UpdateStorage_Write(raw,len(raw))
                if result:break
            if result==0:result=self.lib.UpdateStorage_Install()
        self.lib.UpdateStorage_Close();return result
    def check_user(self):
        assert self.files['/luma/config.ini']==b'user colors' and self.files['/luma/RosaliaTheme/a.png']==b'user theme'
        assert not self.handles,self.handles

def main():
    OUT.mkdir(parents=True,exist_ok=True);shim=OUT/'include/3ds';shim.mkdir(parents=True,exist_ok=True)
    (shim/'types.h').write_text('#pragma once\n#include <stdint.h>\ntypedef uint8_t u8; typedef uint32_t u32; typedef uint64_t u64;\n')
    compiler=os.environ.get('CC') or shutil.which('cc') or 'C:/msys64/ucrt64/bin/gcc.exe';env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    target=OUT/('storage.dll' if os.name=='nt' else 'storage.so')
    command=[compiler,'-std=c11','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-DUPDATE_STORAGE_TEST','-I'+str(OUT/'include'),'-I'+str(ROS/'include'),*[str(ROS/'source'/name) for name in ('updater_storage.c','updater_format.c','sha256.c')],'-o',str(target)]
    built=subprocess.run(command,env=env,capture_output=True,text=True);assert built.returncode==0,built.stderr
    dll=c.CDLL(str(target));dll.UpdateStorage_TestIo.argtypes=[c.POINTER(Io)];dll.UpdateStorage_Begin.argtypes=[c.POINTER(Manifest),c.POINTER(Hooks)];dll.UpdateStorage_Write.argtypes=[c.c_void_p,c.c_uint32];dll.UpdateStorage_CurrentMatches.argtypes=[c.POINTER(Manifest),c.POINTER(Hooks)]
    old,new=firm(4),firm(99);cases=[];keepers=[]
    def disk(old_data=old):
        item=Disk(dll,old_data);keepers.append(item);return item
    def check(name,value):assert value,name;cases.append(name)
    baseline=disk();check('verified update',baseline.run(new)==0 and baseline.files[BOOT]==new and baseline.files[BACKUP]==old and PREVIOUS not in baseline.files);baseline.check_user()
    baseline_trace=list(baseline.trace);snapshots=copy.deepcopy(baseline.snapshots)
    for fail in range(1,len(baseline_trace)+1):
        item=disk();item.failure=fail;result=item.run(new);item.check_user()
        check(f'IO failure {fail} {baseline_trace[fail-1]}',item.files.get(BOOT) in (old,new) and (result==0)==(item.files.get(BOOT)==new))
    for index,op in enumerate(baseline_trace,1):
        if op[0] not in ('read','write'):continue
        item=disk();item.short=index;result=item.run(new);item.check_user()
        check(f'short {index} {op}',item.files[BOOT] in (old,new) and (result==0)==(item.files[BOOT]==new))
    for stage in (4,5,6,7):
        item=disk();item.cancel_stage=stage;result=item.run(new);item.check_user()
        check(f'cancel stage {stage}',(result==0 and item.files[BOOT]==new) if stage==7 else result==-1 and item.files[BOOT]==old)
    item=disk();item.bad_flush=True;check('flush/readback corruption',item.run(new)==-7 and item.files[BOOT]==old);item.check_user()
    item=disk();item.corrupt_installed=True;check('final readback rollback',item.run(new)==-7 and item.files[BOOT]==old and item.files[BACKUP]==old);item.check_user()
    item=disk(None);item.corrupt_installed=True;check('new SD install rollback to absence',item.run(new)==-7 and BOOT not in item.files);item.check_user()
    item=disk(None);check('install without old SD firm',item.run(new)==0 and item.files[BOOT]==new and BACKUP not in item.files);item.check_user()
    broken=bytearray(new);broken[4096]^=1
    item=disk();check('section SHA mismatch',item.run(bytes(broken))==-8 and item.files[BOOT]==old);item.check_user()
    broken=bytearray(new);broken[:4]=b'FAIL'
    item=disk();check('invalid FIRM',item.run(bytes(broken))==-8 and item.files[BOOT]==old);item.check_user()
    item=disk();meta=manifest(new);meta.sha256[0]^=1;check('manifest SHA mismatch',item.run(new,meta)==-7 and item.files[BOOT]==old);item.check_user()
    for index,snapshot in enumerate(snapshots):
        item=disk();item.files=snapshot;meta=manifest(new)
        result=dll.UpdateStorage_Begin(c.byref(meta),c.byref(item.hooks));dll.UpdateStorage_Close();item.check_user()
        check(f'interrupted rename recovery {index}',result==0 and item.files.get(BOOT) in (old,new) and PREVIOUS not in item.files)
    item=disk();item.files[PREVIOUS]=old;item.files[BACKUP]=old;item.files[BOOT]=b'broken'
    meta=manifest(new);result=dll.UpdateStorage_Begin(c.byref(meta),c.byref(item.hooks));dll.UpdateStorage_Close();check('corrupt root recovery',result==0 and item.files[BOOT]==old);item.check_user()
    item=disk();item.files[PREVIOUS]=new;item.files[BACKUP]=old;before=copy.deepcopy(item.files);check('foreign previous preserved',item.run(new)==-11 and all(item.files.get(path)==raw for path,raw in before.items()));item.check_user()
    item=disk(new);meta=manifest(new);check('latest exact firm',dll.UpdateStorage_CurrentMatches(c.byref(meta),c.byref(item.hooks))==1);item.check_user()
    item=disk(old);check('older firm differs',dll.UpdateStorage_CurrentMatches(c.byref(meta),c.byref(item.hooks))==0);item.check_user()
    item=disk();check('begin',dll.UpdateStorage_Begin(c.byref(meta),c.byref(item.hooks))==0)
    check('busy cannot overwrite active',dll.UpdateStorage_Begin(c.byref(meta),c.byref(item.hooks))==-13 and dll.UpdateStorage_CurrentMatches(c.byref(meta),c.byref(item.hooks))==-13)
    check('oversize download rejected',dll.UpdateStorage_Write(new,len(new)+1)==-6)
    check('short download rejected',dll.UpdateStorage_Install()==-6);dll.UpdateStorage_Close();dll.UpdateStorage_Close();item.check_user()
    item=disk();item.failure_predicate=lambda op:op[0]=='rename' and ((op[1],op[2]) in ((TEMP,BOOT),(PREVIOUS,BOOT)))
    result=item.run(new);check('rollback failure preserves both old copies',result==-11 and item.files[PREVIOUS]==old and item.files[BACKUP]==old);item.check_user()
    item.failure_predicate=None
    check('retry recovers failed rollback',dll.UpdateStorage_Begin(c.byref(meta),c.byref(item.hooks))==0 and item.files[BOOT]==old)
    dll.UpdateStorage_Close();item.check_user()
    recovery=disk(new);recovery.files[PREVIOUS]=old;recovery.files[BACKUP]=old
    check('recovery baseline',dll.UpdateStorage_Begin(c.byref(meta),c.byref(recovery.hooks))==0);dll.UpdateStorage_Close();recovery.check_user()
    for failure in range(1,len(recovery.trace)+1):
        item=disk(new);item.files[PREVIOUS]=old;item.files[BACKUP]=old;item.failure=failure
        dll.UpdateStorage_Begin(c.byref(meta),c.byref(item.hooks));dll.UpdateStorage_Close();item.check_user()
        check(f'recovery IO failure {failure}',item.files.get(BOOT)==new and item.files.get(BACKUP)==old)
    report=dict(result='pass',checks=len(cases),cases=cases,source_sha256=hashlib.sha256((ROS/'source/updater_storage.c').read_bytes()).hexdigest(),baseline_operations=len(baseline_trace),hardware_tested=False)
    (OUT/'storage-proof.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8');print(json.dumps({key:report[key] for key in ('result','checks','baseline_operations','source_sha256')}))

if __name__=='__main__':main()
