from pathlib import Path
import ctypes as c
import hashlib
import json
import os
import random
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests'
READ=c.CFUNCTYPE(c.c_int,c.c_void_p,c.c_void_p,c.c_size_t)
SINK=c.CFUNCTYPE(c.c_int,c.c_void_p,c.c_void_p,c.c_uint32)

class Io(c.Structure):
    _fields_=[('context',c.c_void_p),('read',READ)]

class Response(c.Structure):
    _fields_=[('status',c.c_uint),('size',c.c_uint32),('location',c.c_char*2048),('date',c.c_char*30),('age',c.c_uint32),('hasAge',c.c_bool)]

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    target=OUT/('http.dll' if os.name=='nt' else 'http.so')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    result=subprocess.run([compiler,'-std=c11','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-I'+str(ROS/'include'),str(ROS/'source/updater_http.c'),'-o',str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    directory=os.add_dll_directory(str(Path(compiler).parent)) if os.name=='nt' else None
    lib=c.CDLL(str(target))
    lib.UpdateHttp_Read.argtypes=[c.POINTER(Io),c.c_uint32,SINK,c.c_void_p,c.POINTER(Response)]
    lib.UpdateHttp_Read.restype=c.c_int
    cases=0
    def run(wire,limit=65536,fragment=4096,read_error=None,sink_error=None):
        nonlocal cases
        position=0
        output=bytearray()
        def read(_,data,size):
            nonlocal position
            if read_error is not None and position>=read_error:return -2
            part=wire[position:position+min(size,fragment)]
            position+=len(part);c.memmove(data,part,len(part));return len(part)
        def sink(_,data,size):
            if sink_error is not None:return sink_error
            output.extend(c.string_at(data,size));return 0
        io=Io(None,READ(read));callback=SINK(sink);response=Response()
        value=lib.UpdateHttp_Read(c.byref(io),limit,callback,None,c.byref(response))
        cases+=1
        return value,bytes(output),response
    body=b'{"schema":1,"version":"v13.4Y","asset":"boot.firm"}'
    wire=b'HTTP/1.1 200 OK\r\nContent-Length: '+str(len(body)).encode()+b'\r\n\r\n'+body
    for fragment in range(1,len(wire)+2):
        value,data,response=run(wire,fragment=fragment)
        assert value==0 and data==body and response.size==len(body)
    for cut in range(len(wire)):
        assert run(wire[:cut],fragment=7)[0]<0
    chunked=b'HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n'+b'5;name=value\r\n'+body[:5]+b'\r\n'+format(len(body)-5,'X').encode()+b'\r\n'+body[5:]+b'\r\n0\r\nETag: value\r\n\r\n'
    for fragment in range(1,len(chunked)+2):
        value,data,response=run(chunked,fragment=fragment)
        assert value==0 and data==body and response.size==len(body)
    for cut in range(len(chunked)):
        assert run(chunked[:cut],fragment=3)[0]<0
    for header in (
        b'Content-Length: 1\r\nContent-Length: 1',
        b'Content-Length: +1',b'Content-Length: -1',b'Content-Length: 4294967296',
        b'Transfer-Encoding: gzip',b'Transfer-Encoding: chunked, identity',
        b'Transfer-Encoding: chunked\r\nTransfer-Encoding: chunked',
        b'Content-Length: 1\r\nTransfer-Encoding: chunked',
        b'Content-Encoding: gzip',b'Content-Encoding: identity\r\nContent-Encoding: identity',
        b' Location: https://github.com/',b'Bad Name: x',b'Bad\x00: x',
        b'Content-Length : 0',b'Header: x\nInjected: y',b'X: '+b'x'*8200):
        assert run(b'HTTP/1.1 200 OK\r\n'+header+b'\r\n\r\nx')[0]<0
    for head in (b'HTTP/2 200 OK',b'HTTP/1.1 20 OK',b'HTTP/1.1 20A OK',b'HTTP/1.1 200x',b'HTTP/1.1 100 Continue',b'HTTP/1.1 404 Not Found',b'HTTP/1.1 206 Partial Content'):
        assert run(head+b'\r\nContent-Length: 0\r\n\r\n')[0]<0
    for status in (301,302,303,307,308):
        value,data,response=run(f'HTTP/1.1 {status} Redirect\r\nLocation: https://github.com/a\r\nContent-Length: 9\r\n\r\nignoredxx'.encode())
        assert value==0 and data==b'' and response.status==status and response.location==b'https://github.com/a'
        assert run(f'HTTP/1.1 {status} Redirect\r\n\r\n'.encode())[0]<0
    assert run(wire,limit=len(body)-1)[0]==-6
    assert run(wire,limit=len(body))[0]==0
    assert run(chunked,limit=len(body)-1)[0]==-6
    assert run(wire,read_error=0)[0]==-2
    assert run(wire,fragment=1,read_error=len(wire)-2)[0]==-2
    assert run(wire,sink_error=-9)[0]==-9
    assert run(b'HTTP/1.0 200 OK\r\n\r\n'+body)[1]==body
    assert run(b'HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n')[0]==0
    assert run(wire+b'extra')[0]<0
    for tail in (b'X\r\n',b'100000000\r\n',b'-1\r\n',b';x\r\n',b'1\r\nx\n0\r\n\r\n',b'0\r\nContent-Length: 0\r\n\r\n',b'0\r\nTransfer-Encoding: chunked\r\n\r\n',b'0\r\nLocation: https://github.com/\r\n\r\n'):
        assert run(b'HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n'+tail)[0]<0
    long_header=b"Content-Security-Policy: "+b"default-src 'none'; "*200
    redirect=b'HTTP/1.1 302 Found\r\nLocation: https://release-assets.githubusercontent.com/a?'+b'x'*900+b'\r\n'+long_header+b'\r\nDate: Sat, 17 Oct 2026 16:00:00 GMT\r\nAge: 0\r\nContent-Length: 0\r\n\r\n'
    value,data,response=run(redirect,fragment=37)
    assert value==0 and response.status==302 and len(response.location)>900 and response.date==b'Sat, 17 Oct 2026 16:00:00 GMT' and response.hasAge and response.age==0
    for headers in (b'Date: x',b'Date: Sat, 17 Oct 2026 16:00:00 GMT\r\nDate: Sat, 17 Oct 2026 16:00:00 GMT',b'Age: -1',b'Age: 4294967296',b'Age: 0\r\nAge: 0'):
        assert run(b'HTTP/1.1 200 OK\r\n'+headers+b'\r\nContent-Length: 0\r\n\r\n')[0]<0
    rng=random.Random(134)
    for _ in range(1200):
        payload=rng.randbytes(rng.randrange(0,3000))
        parts=[];offset=0
        while offset<len(payload):
            amount=min(len(payload)-offset,rng.randrange(1,130))
            parts.append(format(amount,'x').encode()+b'\r\n'+payload[offset:offset+amount]+b'\r\n')
            offset+=amount
        wire2=b'HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n'+b''.join(parts)+b'0\r\n\r\n'
        value,data,response=run(wire2,len(payload),fragment=rng.randrange(1,300))
        assert value==0 and data==payload and response.size==len(payload)
    report=dict(result='pass',checks=cases,source_sha256=hashlib.sha256((ROS/'source/updater_http.c').read_bytes()).hexdigest(),hardware_tested=False,boundaries=['strict HTTP/1.x headers','content length','chunked and trailers','read and sink errors','bounded body and headers','redirect without consuming body'])
    (OUT/'http-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))

if __name__=='__main__':main()
