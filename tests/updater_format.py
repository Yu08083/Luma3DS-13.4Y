from pathlib import Path
import argparse
import ctypes as c
import hashlib
import json
import os
import random
import shutil
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests'

class Manifest(c.Structure):
    _fields_=[('version',c.c_char*64),('size',c.c_uint32),('sha256',c.c_ubyte*32)]

class Section(c.Structure):
    _fields_=[('offset',c.c_uint32),('address',c.c_uint32),('size',c.c_uint32),('method',c.c_uint32),('hash',c.c_ubyte*32)]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--compiler')
    parser.add_argument('--firm',type=Path)
    args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    compiler=args.compiler or os.environ.get('CC') or shutil.which('cc') or 'C:/msys64/ucrt64/bin/gcc.exe'
    target=OUT/('format.dll' if os.name=='nt' else 'format.so')
    env=dict(os.environ)
    env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    command=[compiler,'-std=c11','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-I'+str(ROS/'include'),str(ROS/'source/updater_format.c'),'-o',str(target)]
    result=subprocess.run(command,env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    lib=c.CDLL(str(target))
    lib.Update_ParseRelease.argtypes=[c.c_char_p,c.c_size_t,c.c_void_p]
    lib.Update_ParseManifest.argtypes=[c.c_char_p,c.c_size_t,c.c_char_p,c.POINTER(Manifest)]
    lib.Update_TrustedUrl.argtypes=[c.c_char_p]
    lib.Update_AssetUrl.argtypes=[c.c_void_p,c.c_char_p,c.c_char_p]
    lib.Update_FirmHeader.argtypes=[c.c_void_p,c.c_uint32,c.POINTER(Section),c.POINTER(c.c_uint32)]
    for name in ('Update_ParseRelease','Update_ParseManifest','Update_TrustedUrl','Update_AssetUrl','Update_FirmHeader'):getattr(lib,name).restype=c.c_bool
    checks=0
    def expect(actual,wanted=True):
        nonlocal checks
        assert bool(actual)==bool(wanted),(checks,actual,wanted)
        checks+=1
    def release(data):
        raw=data if isinstance(data,bytes) else data.encode('utf-8')
        out=c.create_string_buffer(64)
        return lib.Update_ParseRelease(raw,len(raw),out),out.value
    tag='v13.4Y'
    base={'schema':1,'version':tag,'asset':'boot.firm','size':1024,'sha256':hashlib.sha256(b'firmware').hexdigest()}
    def manifest(data):
        raw=data if isinstance(data,bytes) else data.encode('utf-8')
        out=Manifest()
        return lib.Update_ParseManifest(raw,len(raw),tag.encode(),c.byref(out)),out
    valid_release=json.dumps({'tag_name':tag,'draft':False,'prerelease':False,'body':'更新\n🌸','assets':[{'n':'nested tag_name','v':None}]},ensure_ascii=False).encode()
    ok,value=release(valid_release);expect(ok);assert value==tag.encode()
    for i in range(len(valid_release)):expect(release(valid_release[:i])[0],False)
    good=json.dumps(base,separators=(',',':')).encode()
    ok,m=manifest(good);expect(ok);assert bytes(m.sha256)==bytes.fromhex(base['sha256']) and m.size==1024
    for i in range(len(good)):expect(manifest(good[:i])[0],False)
    for key in base:
        test=dict(base);test.pop(key);expect(manifest(json.dumps(test))[0],False)
        addition=good[:-1]+b','+json.dumps(key).encode()+b':'+json.dumps(base[key]).encode()+b'}'
        expect(manifest(addition)[0],False)
    for key,values in {'schema':[0,2,True,'1',1.0,-1],'version':['v13.4Z','../v13.4Y','v13.4Y/evil',''],'asset':['a.firm','/boot.firm',None],'size':[0,1023,4194305,-1,True,'1024',1024.5,4294967296],'sha256':['0'*63,'0'*65,'g'*64,base['sha256'].upper(),None]}.items():
        for value in values:
            test=dict(base);test[key]=value;expect(manifest(json.dumps(test))[0],False)
    for bad in (good+b'{}',b'\xef\xbb\xbf'+good,good+b'\0',b'[]',good[:-1]+b',}',good.replace(b'1024',b'01024'),good.replace(b'1024',b'1024e0')):
        expect(manifest(bad)[0],False)
    for depth in (1,8,16):
        test=good[:-1]+b',"other":'+b'['*depth+b'null'+b']'*depth+b'}'
        expect(manifest(test)[0])
    expect(manifest(good[:-1]+b',"other":'+b'['*17+b'null'+b']'*17+b'}')[0],False)
    for bad in ({'tag_name':'v13.4Y','draft':True,'prerelease':False},{'tag_name':'v13.4Y','draft':False,'prerelease':True},{'tag_name':'../evil','draft':False,'prerelease':False},{'tag_name':'v13.4Y','draft':False}):expect(release(json.dumps(bad))[0],False)
    for host in ('api.github.com','github.com','release-assets.githubusercontent.com','objects.githubusercontent.com'):expect(lib.Update_TrustedUrl(('https://'+host+'/path?a=1&sig=ok%2F').encode()))
    for url in ('http://github.com/path','https://github.com.evil/path','https://user@github.com/path','https://github.com:443/path','https://evil/github.com/path','https://github.com/\\evil','https://github.com/path#fragment','https://github.com/path\r\nX: yes','https://github.com','https://GITHUB.COM/path'):
        expect(lib.Update_TrustedUrl(url.encode()),False)
    url=c.create_string_buffer(2048)
    expect(lib.Update_AssetUrl(url,tag.encode(),b'boot.firm'));assert url.value==b'https://github.com/Yu08083/Luma3DS-13.4Y/releases/download/v13.4Y/boot.firm'
    expect(lib.Update_AssetUrl(url,b'../x',b'boot.firm'),False)
    expect(lib.Update_AssetUrl(url,tag.encode(),b'bad.json'),False)
    h=bytearray(512);h[:4]=b'FIRM';struct.pack_into('<II',h,8,0x1FF80000,0x08001000)
    for i,address in enumerate((0x08001000,0x1FF80000,0x21000000,0x18000000)):struct.pack_into('<4I',h,0x40+i*0x30,512+i*512,address,512,i%3)
    def firm(header,size=2560):
        rows=(Section*4)();count=c.c_uint32();raw=bytes(header)
        return lib.Update_FirmHeader(raw,size,rows,c.byref(count)),count.value
    ok,count=firm(h);expect(ok);assert count==4
    for offset,value in ((0,0),(12,0),(12,0x08002000),(8,0x1FF90000),(0x40,0),(0x40,513),(0x44,0),(0x44,0xFFFFFFFF),(0x48,1),(0x48,0xFFFFFF00),(0x4C,3),(0x70,512),(0x74,0x08001000)):
        test=bytearray(h);struct.pack_into('<I',test,offset,value);expect(firm(test)[0],False)
    for size in (0,511,512,1023,2048,4194305,0xFFFFFFFF):expect(firm(h,size)[0],False)
    rng=random.Random(134)
    for _ in range(1000):expect(firm(rng.randbytes(512))[0],False)
    if args.firm:
        data=args.firm.read_bytes();expect(firm(data[:512],len(data))[0])
    report={'result':'pass','checks':checks,'source_sha256':hashlib.sha256((ROS/'source/updater_format.c').read_bytes()).hexdigest(),'boundaries':['Pure JSON/URL/header parser only. TLS, section hashing and SD transactions run in separate tests.']}
    (OUT/'format-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))

if __name__=='__main__':main()
