from pathlib import Path
import argparse
import ctypes as c
import hashlib
import importlib.util
import json
import os
import select
import socket
import subprocess
import time
from urllib.parse import urlsplit, urljoin

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests'
spec=importlib.util.spec_from_file_location('tls_fixture',ROOT/'tests/updater_tls.py')
tls_fixture=importlib.util.module_from_spec(spec)
spec.loader.exec_module(tls_fixture)

class Response(c.Structure):
    _fields_=[('status',c.c_uint),('size',c.c_uint32),('location',c.c_char*2048),('date',c.c_char*30),('age',c.c_uint32),('hasAge',c.c_bool)]

READ=c.CFUNCTYPE(c.c_int,c.c_void_p,c.c_void_p,c.c_size_t)
SINK=c.CFUNCTYPE(c.c_int,c.c_void_p,c.c_void_p,c.c_uint32)

class HttpIo(c.Structure):
    _fields_=[('context',c.c_void_p),('read',READ)]

class Manifest(c.Structure):
    _fields_=[('version',c.c_char*64),('size',c.c_uint32),('sha256',c.c_ubyte*32)]

def build():
    OUT.mkdir(parents=True,exist_ok=True)
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    target=OUT/('online-http.dll' if os.name=='nt' else 'online-http.so')
    env=dict(os.environ)
    env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    sources=[ROS/'source/updater_http.c',ROS/'source/updater_format.c']
    result=subprocess.run([compiler,'-std=c11','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-I'+str(ROS/'include'),*map(str,sources),'-o',str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    directory=os.add_dll_directory(str(Path(compiler).parent)) if os.name=='nt' else None
    lib=c.CDLL(str(target))
    lib.UpdateHttp_Read.argtypes=[c.POINTER(HttpIo),c.c_uint32,SINK,c.c_void_p,c.POINTER(Response)]
    lib.UpdateHttp_Read.restype=c.c_int
    lib.Update_TrustedUrl.argtypes=[c.c_char_p]
    lib.Update_TrustedUrl.restype=c.c_bool
    lib.Update_ParseRelease.argtypes=[c.c_char_p,c.c_size_t,c.c_void_p]
    lib.Update_ParseRelease.restype=c.c_bool
    lib.Update_ParseManifest.argtypes=[c.c_char_p,c.c_size_t,c.c_char_p,c.POINTER(Manifest)]
    lib.Update_ParseManifest.restype=c.c_bool
    return lib

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--live',action='store_true',required=True)
    args=parser.parse_args()
    tls=tls_fixture.build()
    http=build()
    tls.UpdateTls_InitClock.argtypes=[c.c_void_p,c.POINTER(tls_fixture.Io)]
    tls.UpdateTls_InitClock.restype=c.c_int
    tls.UpdateTls_ConfirmClock.argtypes=[c.c_void_p,c.c_int64]
    tls.UpdateTls_ConfirmClock.restype=c.c_bool
    tls.UpdateClock_ParseHttpDate.argtypes=[c.c_char_p,c.POINTER(c.c_int64)]
    tls.UpdateClock_ParseHttpDate.restype=c.c_bool
    cases=[]
    want_read=tls.UpdateTest_WantRead()
    want_write=tls.UpdateTest_WantWrite()
    trusted_time=None
    trusted_tick=None

    def request(url,limit,clock=False):
        nonlocal trusted_time,trusted_tick
        parts=urlsplit(url)
        assert http.Update_TrustedUrl(url.encode())
        sock=socket.create_connection((parts.hostname,443),timeout=20)
        sock.setblocking(False)
        deadline=time.monotonic()+60
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
            c.memmove(data,os.urandom(size),size)
            return 0
        def now(_):
            if trusted_time is not None:return trusted_time+int(time.monotonic()-trusted_tick)
            return 1577836800 if clock else int(time.time())
        callbacks=tls_fixture.Io(None,tls_fixture.SEND(send),tls_fixture.RECEIVE(receive),tls_fixture.RANDOM(random),tls_fixture.TIME(now),tls_fixture.TIME(lambda _:time.monotonic_ns()//1000000))
        state=c.create_string_buffer(tls.UpdateTest_TlsSize())
        try:
            result=tls.UpdateTls_InitClock(state,c.byref(callbacks)) if clock else tls.UpdateTls_Init(state,parts.hostname.encode(),c.byref(callbacks))
            assert result==0
            while True:
                result=tls.UpdateTls_Handshake(state)
                if result:break
                assert time.monotonic()<deadline
                select.select([sock],[],[],0.01)
            assert result==1,(url,result,tls.UpdateTest_Error(state))
            path=parts.path or '/'
            if parts.query:path+='?'+parts.query
            raw=('GET '+path+' HTTP/1.1\r\nHost: '+parts.hostname+'\r\nUser-Agent: Luma3DS-13.4Y\r\nAccept: application/vnd.github+json\r\nAccept-Encoding: identity\r\nCache-Control: no-cache, no-store\r\nConnection: close\r\n\r\n').encode()
            sent=0
            while sent<len(raw):
                count=tls.UpdateTls_Write(state,raw[sent:],len(raw)-sent)
                assert count!=-2,(url,tls.UpdateTest_Error(state))
                if count>0:sent+=count
                assert time.monotonic()<deadline
                if count==-1:select.select([],[sock],[],0.01)
            output=bytearray()
            def read(_,data,size):
                while time.monotonic()<deadline:
                    count=tls.UpdateTls_Read(state,data,size)
                    if count!=-1:return count
                    select.select([sock],[],[],0.01)
                return -2
            def sink(_,data,size):
                output.extend(c.string_at(data,size))
                return 0
            io=HttpIo(None,READ(read))
            callback=SINK(sink)
            response=Response()
            result=http.UpdateHttp_Read(c.byref(io),limit,callback,None,c.byref(response))
            assert result==0,(url,result,response.status)
            if clock:
                assert response.status==200 and (not response.hasAge or response.age==0)
                utc=c.c_int64()
                assert tls.UpdateClock_ParseHttpDate(response.date,c.byref(utc))
                assert tls.UpdateTls_ConfirmClock(state,utc.value)
                trusted_time=utc.value
                trusted_tick=time.monotonic()
            cases.append(dict(host=parts.hostname,status=response.status,bytes=len(output),clock_bootstrap=clock))
            return bytes(output),response
        finally:
            tls.UpdateTls_Close(state)
            sock.close()

    def fetch(url,limit):
        for _ in range(6):
            data,response=request(url,limit)
            if response.status==200:return data
            url=urljoin(url,response.location.decode())
            assert http.Update_TrustedUrl(url.encode())
        raise AssertionError('redirect limit')

    request('https://api.github.com/?luma-clock='+os.urandom(8).hex(),32768,clock=True)
    release=fetch('https://api.github.com/repos/Yu08083/Luma3DS-13.4Y/releases/latest',65536)
    tag=c.create_string_buffer(64)
    assert http.Update_ParseRelease(release,len(release),tag)
    base='https://github.com/Yu08083/Luma3DS-13.4Y/releases/download/'+tag.value.decode()+'/'
    metadata=fetch(base+'update.json',2048)
    manifest=Manifest()
    assert http.Update_ParseManifest(metadata,len(metadata),tag,c.byref(manifest))
    firm=fetch(base+'boot.firm',manifest.size)
    assert len(firm)==manifest.size and hashlib.sha256(firm).digest()==bytes(manifest.sha256)
    assert firm[:4]==b'FIRM'
    report=dict(result='pass',tag=tag.value.decode(),firm_bytes=len(firm),firm_sha256=hashlib.sha256(firm).hexdigest(),cases=cases,wrong_rtc=1577836800,corrected_utc=trusted_time,hardware_tested=False,boundary='production MbedTLS/TLS, HTTP parser, URL/release/manifest parsers; real GitHub downloads; host socket/random callbacks; no SD writes')
    (OUT/'online-proof.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    print(json.dumps(report))

if __name__=='__main__':main()
