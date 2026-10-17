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
    body=r'''
size_t UpdateTest_TlsSize(void){return sizeof(UpdateTls);}
int UpdateTest_WantRead(void){return MBEDTLS_ERR_SSL_WANT_READ;}
int UpdateTest_WantWrite(void){return MBEDTLS_ERR_SSL_WANT_WRITE;}
int UpdateTest_Error(UpdateTls *tls){return tls->error;}
uint32_t UpdateTest_VerifyFlags(UpdateTls *tls){return tls->verifyFlags;}
void UpdateTest_ClockReady(UpdateTls *tls){tls->connected=true;}
int UpdateTest_ClockCertificate(UpdateTls *tls,const unsigned char *der,size_t size,int depth,uint32_t *flags,int variant){
    mbedtls_x509_crt certificate;
    mbedtls_x509_crt_init(&certificate);
    int result=mbedtls_x509_crt_parse_der(&certificate,der,size);
    if(!result){
        if(variant==1)--certificate.pk_raw.len;
        if(variant==2){certificate.valid_from.year=2026;certificate.valid_from.mon=10;certificate.valid_from.day=19;}
        if(variant==3){certificate.valid_to.year=2026;certificate.valid_to.mon=10;certificate.valid_to.day=20;}
        if(variant==4)certificate.valid_from.mon=13;
        result=clockVerify(tls,&certificate,depth,flags);
    }
    mbedtls_x509_crt_free(&certificate);
    return result;
}
int UpdateTest_DateFailure(int error,uint32_t flags,int mode,int connected){
    UpdateTls tls={.ownsPool=true,.error=error,.verifyFlags=flags,.clockMode=mode!=0,.connected=connected!=0};
    return UpdateTls_DateFailure(&tls);
}
'''
    support.write_bytes(('#include "'+(ROS/'source/updater_tls.c').as_posix()+'"\n'+body).encode())
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    target=OUT/('tls.dll' if os.name=='nt' else 'tls.so')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    inputs=sorted((ROS/'third_party/mbedtls/library').glob('*.c'))
    inputs += [ROS/'source/updater_clock.c',ROS/'source/updater_ca.c',support]
    args=[compiler,'-std=c11','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-DMBEDTLS_CONFIG_FILE="updater_tls_config.h"','-I'+str(ROS/'include'),'-I'+str(ROS/'third_party/mbedtls/include'),'-I'+str(ROS/'third_party/mbedtls/library'),*map(str,inputs),*(['-lws2_32'] if os.name=='nt' else []),'-o',str(target)]
    response=OUT/'tls-build.rsp'
    response.write_bytes(('\n'.join(json.dumps(a.replace('\\','/')) for a in args[1:])+'\n').encode())
    digest=hashlib.sha256(response.read_bytes())
    for source in inputs+[ROS/'source/updater_tls.c',ROS/'include/updater_tls_config.h',ROS/'include/updater_tls.h',ROS/'include/updater_ca.h',ROS/'include/updater_clock.h']:
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
    lib.UpdateTls_InitClock.argtypes=[c.c_void_p,c.POINTER(Io)];lib.UpdateTls_InitClock.restype=c.c_int
    lib.UpdateTls_ConfirmClock.argtypes=[c.c_void_p,c.c_int64];lib.UpdateTls_ConfirmClock.restype=c.c_bool
    lib.UpdateTls_DateFailure.argtypes=[c.c_void_p];lib.UpdateTls_DateFailure.restype=c.c_bool
    lib.UpdateTest_VerifyFlags.argtypes=[c.c_void_p];lib.UpdateTest_VerifyFlags.restype=c.c_uint32
    lib.UpdateTest_ClockReady.argtypes=[c.c_void_p]
    lib.UpdateTest_ClockCertificate.argtypes=[c.c_void_p,c.c_void_p,c.c_size_t,c.c_int,c.POINTER(c.c_uint32),c.c_int];lib.UpdateTest_ClockCertificate.restype=c.c_int
    lib.UpdateTest_DateFailure.argtypes=[c.c_int,c.c_uint32,c.c_int,c.c_int];lib.UpdateTest_DateFailure.restype=c.c_int
    lib.UpdateTls_Handshake.argtypes=[c.c_void_p];lib.UpdateTls_Handshake.restype=c.c_int
    lib.UpdateTls_Read.argtypes=[c.c_void_p,c.c_void_p,c.c_size_t];lib.UpdateTls_Read.restype=c.c_int
    lib.UpdateTls_Write.argtypes=[c.c_void_p,c.c_void_p,c.c_size_t];lib.UpdateTls_Write.restype=c.c_int
    lib.UpdateTls_Close.argtypes=[c.c_void_p]
    return lib

def connect(lib,host,peer=None,when=None,rng_failure=False,bootstrap=False):
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
        assert (lib.UpdateTls_InitClock(tls,c.byref(callbacks)) if bootstrap else lib.UpdateTls_Init(tls,host.encode(),c.byref(callbacks)))==0
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
            response=bytearray()
            while True:
                amount=lib.UpdateTls_Read(tls,data,len(data))
                if amount!=-1:
                    assert amount>0
                    response.extend(data.raw[:amount])
                    if not bootstrap or b'\r\n\r\n' in response:break
                assert time.monotonic()<deadline
                assert len(response)<8192
                select.select([sock],[],[],0.01)
            assert response.startswith(b'HTTP/1.'),(host,amount,lib.UpdateTest_Error(tls))
            if bootstrap:
                from email.utils import parsedate_to_datetime
                values=[line[6:] for line in bytes(response).split(b'\r\n\r\n')[0].split(b'\r\n') if line.startswith(b'Date: ')]
                assert len(values)==1
                seconds=int(parsedate_to_datetime(values[0].decode()).timestamp())
                assert lib.UpdateTls_ConfirmClock(tls,seconds)
        elif not bootstrap and when is not None and not rng_failure:
            assert bool(lib.UpdateTls_DateFailure(tls))==(host=='api.github.com')
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
    der=(ROOT/'tests/fixtures/updater-clock-api.der').read_bytes()
    def certificate(depth,flags=0,variant=0):
        value=c.c_uint32(flags)
        assert lib.UpdateTest_ClockCertificate(tls,der,len(der),depth,c.byref(value),variant)==0
        return value.value
    assert not lib.UpdateTls_ConfirmClock(None,1792249200) and not lib.UpdateTls_DateFailure(None)
    for flags in [0,1,0x200,0x201]+[1<<bit for bit in range(32)]:
        assert lib.UpdateTls_InitClock(tls,c.byref(failed_random))==0
        remaining=certificate(0,flags)
        lib.UpdateTest_ClockReady(tls)
        permitted=not (flags&~0x201)
        assert remaining==(0 if permitted else (flags&~0x201)|0x100)
        assert bool(lib.UpdateTls_ConfirmClock(tls,1792249200))==permitted
        lib.UpdateTls_Close(tls)
    cases.append(dict(name='clock_only_expired_future_flags_relaxed',flag_patterns=36))
    assert lib.UpdateTls_InitClock(tls,c.byref(failed_random))==0
    assert certificate(1,0x201,2)==0 and certificate(0,0x201)==0
    assert not lib.UpdateTls_ConfirmClock(tls,1792368000)
    lib.UpdateTest_ClockReady(tls)
    assert not lib.UpdateTls_ConfirmClock(tls,1792249200)
    assert lib.UpdateTls_ConfirmClock(tls,1792368000)
    assert not lib.UpdateTls_ConfirmClock(tls,2524608000)
    assert not lib.UpdateTls_ConfirmClock(tls,1792249199)
    lib.UpdateTls_Close(tls)
    cases.append(dict(name='clock_verified_chain_intersection_build_floor_and_handshake_required'))
    for depths,variant in [([0],1),([0],4),([2,0],0),([0,0],0),([-1],0),([16],0)]:
        assert lib.UpdateTls_InitClock(tls,c.byref(failed_random))==0
        for depth in depths:certificate(depth,0,variant)
        lib.UpdateTest_ClockReady(tls)
        assert not lib.UpdateTls_ConfirmClock(tls,1792249200)
        lib.UpdateTls_Close(tls)
    cases.append(dict(name='clock_rejects_wrong_pin_invalid_cert_duplicate_missing_and_excess_depth',patterns=6))
    for error,flags,mode,connected,expected in [(-0x2700,1,0,0,1),(-0x2700,0x200,0,0,1),(-0x2700,0x201,0,0,1),(-0x2700,5,0,0,0),(-0x2700,0,0,0,0),(-0x2700,0xFFFFFFFF,0,0,0),(-0x2700,1,1,0,0),(-0x2700,1,0,1,0),(-0x6C00,1,0,0,0)]:
        assert lib.UpdateTest_DateFailure(error,flags,mode,connected)==expected
    cases.append(dict(name='normal_retry_only_date_failure',patterns=9))
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
        for when in [1577836800,2524608000]:
            result,error=connect(lib,'api.github.com',when=when,bootstrap=True)
            assert result==1 and error==0
            cases.append(dict(name='pinned_clock_bootstrap_wrong_rtc',rtc=when))
            print('clock bootstrap',when,'pass',flush=True)
    report=dict(result='pass',cases=cases,count=len(cases),tls='1.2 only',verification='required; hostname and validity dates enabled',pool_bytes=192*1024,hardware_tested=False,source_sha256=hashlib.sha256((ROS/'source/updater_tls.c').read_bytes()).hexdigest())
    (OUT/'tls-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))

if __name__=='__main__':main()
