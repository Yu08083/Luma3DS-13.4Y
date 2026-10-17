from pathlib import Path
import argparse
import ctypes as c
import hashlib
import json
import os
import select
import socket
import subprocess
import time

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests'
SEND=c.CFUNCTYPE(c.c_int,c.c_void_p,c.c_void_p,c.c_size_t)
RECEIVE=SEND
RANDOM=SEND
TIME=c.CFUNCTYPE(c.c_int64,c.c_void_p)

class Io(c.Structure):
    _fields_=[('context',c.c_void_p),('send',SEND),('receive',RECEIVE),('random',RANDOM),('time',TIME),('milliseconds',TIME)]

def build():
    OUT.mkdir(parents=True,exist_ok=True)
    support=OUT/'tls-test-support.c'
    support.write_bytes(b'#include "updater_tls.h"\nsize_t UpdateTest_TlsSize(void){return sizeof(UpdateTls);}\nint UpdateTest_WantRead(void){return MBEDTLS_ERR_SSL_WANT_READ;}\nint UpdateTest_WantWrite(void){return MBEDTLS_ERR_SSL_WANT_WRITE;}\nint UpdateTest_Error(UpdateTls *tls){return tls->error;}\n')
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    target=OUT/('tls.dll' if os.name=='nt' else 'tls.so')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    inputs=sorted((ROS/'third_party/mbedtls/library').glob('*.c'))
    inputs += [ROS/'source/updater_tls.c',ROS/'source/updater_ca.c',support]
    args=[compiler,'-std=c11','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-DMBEDTLS_CONFIG_FILE="updater_tls_config.h"','-I'+str(ROS/'include'),'-I'+str(ROS/'third_party/mbedtls/include'),'-I'+str(ROS/'third_party/mbedtls/library'),*map(str,inputs),*(['-lws2_32'] if os.name=='nt' else []),'-o',str(target)]
    response=OUT/'tls-build.rsp'
    response.write_bytes(('\n'.join(json.dumps(a.replace('\\','/')) for a in args[1:])+'\n').encode())
    digest=hashlib.sha256(response.read_bytes())
    for source in inputs+[ROS/'include/updater_tls_config.h',ROS/'include/updater_tls.h',ROS/'include/updater_ca.h']:
        digest.update(source.read_bytes())
    stamp=OUT/'tls-input.sha256'
    key=digest.hexdigest()
    if not target.exists() or not stamp.exists() or stamp.read_text()!=key:
        result=subprocess.run([compiler,'@'+str(response)],env=env,capture_output=True,text=True)
        assert result.returncode==0,(result.stdout+result.stderr)[-6000:]
        stamp.write_bytes(key.encode())
    dll_directory=os.add_dll_directory(str(Path(compiler).parent)) if os.name=='nt' else None
    lib=c.CDLL(str(target))
    lib.UpdateTest_TlsSize.restype=c.c_size_t
    lib.UpdateTest_WantRead.restype=lib.UpdateTest_WantWrite.restype=c.c_int
    lib.UpdateTest_Error.argtypes=[c.c_void_p];lib.UpdateTest_Error.restype=c.c_int
    lib.UpdateTls_Init.argtypes=[c.c_void_p,c.c_char_p,c.POINTER(Io)];lib.UpdateTls_Init.restype=c.c_int
    lib.UpdateTls_Handshake.argtypes=[c.c_void_p];lib.UpdateTls_Handshake.restype=c.c_int
    lib.UpdateTls_Read.argtypes=[c.c_void_p,c.c_void_p,c.c_size_t];lib.UpdateTls_Read.restype=c.c_int
    lib.UpdateTls_Write.argtypes=[c.c_void_p,c.c_void_p,c.c_size_t];lib.UpdateTls_Write.restype=c.c_int
    lib.UpdateTls_Close.argtypes=[c.c_void_p]
    return lib

def connect(lib,host,peer=None,when=None,rng_failure=False):
    sock=socket.create_connection((peer or host,443),timeout=15)
    sock.setblocking(False)
    want_read=lib.UpdateTest_WantRead();want_write=lib.UpdateTest_WantWrite()
    def send(_,data,size):
        try:return sock.send(c.string_at(data,size))
        except BlockingIOError:return want_write
        except OSError:return -0x6C00
    def receive(_,data,size):
        try:
            raw=sock.recv(size)
            c.memmove(data,raw,len(raw))
            return len(raw)
        except BlockingIOError:return want_read
        except OSError:return -0x6C00
    def random(_,data,size):
        if rng_failure:return -1
        c.memmove(data,os.urandom(size),size)
        return 0
    callbacks=Io(None,SEND(send),RECEIVE(receive),RANDOM(random),TIME(lambda _:int(time.time()) if when is None else when),TIME(lambda _:time.monotonic_ns()//1000000))
    tls=c.create_string_buffer(lib.UpdateTest_TlsSize())
    try:
        assert lib.UpdateTls_Init(tls,host.encode(),c.byref(callbacks))==0
        second=c.create_string_buffer(lib.UpdateTest_TlsSize())
        assert lib.UpdateTls_Init(second,host.encode(),c.byref(callbacks))==-2
        lib.UpdateTls_Close(second)
        deadline=time.monotonic()+30
        while True:
            status=lib.UpdateTls_Handshake(tls)
            if status:break
            assert time.monotonic()<deadline,'handshake exceeded deadline'
            select.select([sock],[],[],0.01)
        error=lib.UpdateTest_Error(tls)
        if status==1:
            request=('GET / HTTP/1.1\r\nHost: '+host+'\r\nUser-Agent: Luma3DS-13.4Y\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n').encode()
            sent=0
            while sent<len(request):
                amount=lib.UpdateTls_Write(tls,request[sent:],len(request)-sent)
                assert amount!=-2,(host,lib.UpdateTest_Error(tls))
                if amount>0:sent+=amount
                assert time.monotonic()<deadline
                if amount==-1:select.select([], [sock], [], 0.01)
            data=c.create_string_buffer(4096)
            while True:
                amount=lib.UpdateTls_Read(tls,data,len(data))
                if amount!=-1:break
                assert time.monotonic()<deadline
                select.select([sock],[],[],0.01)
            assert amount>0 and data.raw[:amount].startswith(b'HTTP/1.'),(host,amount,lib.UpdateTest_Error(tls))
        return status,error
    finally:
        lib.UpdateTls_Close(tls)
        lib.UpdateTls_Close(tls)
        sock.close()

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--live',action='store_true')
    args=parser.parse_args()
    lib=build()
    cases=[]
    want_read=lib.UpdateTest_WantRead();want_write=lib.UpdateTest_WantWrite()
    failed_random=Io(None,SEND(lambda *_:want_write),RECEIVE(lambda *_:want_read),RANDOM(lambda *_:-1),TIME(lambda _:int(time.time())),TIME(lambda _:time.monotonic_ns()//1000000))
    tls=c.create_string_buffer(lib.UpdateTest_TlsSize())
    other=c.create_string_buffer(lib.UpdateTest_TlsSize())
    assert lib.UpdateTls_Init(tls,b'api.github.com',c.byref(failed_random))==0
    assert lib.UpdateTls_Init(other,b'api.github.com',c.byref(failed_random))==-2
    lib.UpdateTls_Close(other)
    assert lib.UpdateTls_Read(tls,other,len(other))==-2
    assert lib.UpdateTls_Write(tls,other,len(other))==-2
    assert lib.UpdateTls_Handshake(tls)==-2 and lib.UpdateTest_Error(tls)!=0
    lib.UpdateTls_Close(tls);lib.UpdateTls_Close(tls)
    cases.extend([dict(name=name) for name in ('offline_root_parse','single_fixed_pool','read_before_handshake','write_before_handshake','offline_rng_failure','close_idempotent')])
    assert lib.UpdateTls_Init(tls,b'api.github.com',c.byref(failed_random))==0
    lib.UpdateTls_Close(tls)
    cases.append(dict(name='pool_reuse_after_close'))
    if args.live:
        for host in ('api.github.com','github.com','release-assets.githubusercontent.com','objects.githubusercontent.com'):
            result,error=connect(lib,host)
            assert result==1 and error==0,(host,result,error)
            cases.append(dict(name='verified_tls12_http',host=host))
            print(host,'pass',flush=True)
        for name,options in (('hostname_mismatch',dict(host='not-github.invalid',peer='github.com')),('expired_date',dict(host='api.github.com',when=2524608000)),('not_yet_valid_date',dict(host='api.github.com',when=1577836800)),('rng_failure',dict(host='api.github.com',rng_failure=True))):
            result,error=connect(lib,**options)
            assert result==-2 and error!=0,(name,result,error)
            cases.append(dict(name=name,error=error))
            print(name,'rejected',flush=True)
    report=dict(result='pass',cases=cases,count=len(cases),tls='1.2 only',verification='required; hostname and validity dates enabled',pool_bytes=192*1024,hardware_tested=False,source_sha256=hashlib.sha256((ROS/'source/updater_tls.c').read_bytes()).hexdigest())
    (OUT/'tls-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))

if __name__=='__main__':main()
