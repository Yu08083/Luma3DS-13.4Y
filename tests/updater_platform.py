import ctypes as c
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'updater/source/platform.c'
OUT=ROOT/'build/updater-tests/platform-fixture'


def function(text,name):
    start=text.index('ssize_t '+name+'(')
    opening=text.index('{',start)
    depth=1
    position=opening+1
    while depth:
        assert position<len(text)
        if text[position]=='{':depth+=1
        elif text[position]=='}':depth-=1
        position+=1
    return text[start:position]+'\n'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    compiler=os.environ.get('CC','cc')
    if not shutil.which(compiler) and os.name=='nt':compiler='C:/msys64/ucrt64/bin/gcc.exe'
    prefix=r'''#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
static int errorValue;
static ssize_t operationValue;
void test_configure(int error,ssize_t result){errorValue=error;operationValue=result;}
static ssize_t platform_test_send(int socket,const void *data,size_t size,int flags){(void)socket;(void)data;(void)size;(void)flags;errno=errorValue;return operationValue;}
static ssize_t platform_test_recv(int socket,void *data,size_t size,int flags){(void)socket;(void)data;(void)size;(void)flags;errno=errorValue;return operationValue;}
int test_again(void){return EAGAIN;}
int test_intr(void){return EINTR;}
int test_block(void){return EWOULDBLOCK;}
int test_reset(void){return ECONNRESET;}
int test_timeout(void){return ETIMEDOUT;}
int test_netdown(void){return ENETDOWN;}
#define send platform_test_send
#define recv platform_test_recv
'''
    text=SOURCE.read_text(encoding='utf-8')
    extracted=function(text,'socSend')+function(text,'socRecv')
    support=OUT/'platform-test.c'
    support.write_bytes((prefix+extracted).encode())
    target=OUT/('platform.dll' if os.name=='nt' else 'platform.so')
    environment=dict(os.environ)
    resolved=shutil.which(compiler) or compiler
    environment['PATH']=str(Path(resolved).parent)+os.pathsep+environment.get('PATH','')
    result=subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(support),'-o',str(target)],env=environment,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    directory=os.add_dll_directory(str(Path(resolved).parent)) if os.name=='nt' else None
    library=c.CDLL(str(target))
    library.test_configure.argtypes=[c.c_int,c.c_ssize_t]
    cases=[]
    for name in ['socSend','socRecv']:
        operation=getattr(library,name)
        operation.argtypes=[c.c_int,c.c_void_p,c.c_size_t,c.c_int]
        operation.restype=c.c_ssize_t
        for constant in ['again','intr','block','reset','timeout','netdown']:
            error=getattr(library,'test_'+constant)()
            library.test_configure(error,-1)
            actual=operation(3,None,4096,0)
            assert actual==-error,(name,constant,actual,error)
            cases.append({'operation':name,'errno':constant,'native_errno':error,'adapter_return':actual})
        for count in [0,1,15,4095,4096]:
            library.test_configure(library.test_intr(),count)
            actual=operation(3,None,4096,0)
            assert actual==count,(name,count,actual)
            cases.append({'operation':name,'byte_count':count,'adapter_return':actual})
    report={'result':'pass','checks':len(cases),'source_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'function_extract_sha256':hashlib.sha256(extracted.encode()).hexdigest(),'cases':cases,'boundary':'Actual unchanged CIA socket adapter function bodies. Mock libc uses POSIX -1 plus errno; positive and zero byte counts remain unchanged. No SOC IPC or device execution.','hardware_tested':False}
    (OUT/'proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({'result':'pass','checks':len(cases),'source_sha256':report['source_sha256']}))


if __name__=='__main__':main()
