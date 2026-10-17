from pathlib import Path
import hashlib
import json
import os
import subprocess

ROOT=Path(__file__).resolve().parents[2]
APP=ROOT/'updater'
CORE=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=APP/'build/app-test'

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    files={
    '3ds.h':r"""#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;typedef int32_t Result;typedef uint32_t Handle;typedef uint64_t FS_Archive;typedef int LightLock;typedef struct {int type;const void *data;} FS_Path;
typedef struct Thread_tag {void (*entry)(void *);void *arg;bool done;} *Thread;
#define R_FAILED(v) ((v)<0)
#define R_SUCCEEDED(v) ((v)>=0)
#define ARCHIVE_SDMC 9
#define PATH_EMPTY 1
#define PATH_ASCII 3
#define FS_OPEN_READ 1
void LightLock_Init(LightLock *);void LightLock_Lock(LightLock *);void LightLock_Unlock(LightLock *);
Thread threadCreate(void (*)(void *),void *,size_t,int,int,bool);
Result threadJoin(Thread,u64);void threadFree(Thread);
FS_Path fsMakePath(int,const void *);
Result FSUSER_OpenArchive(FS_Archive *,int,FS_Path);
Result FSUSER_CloseArchive(FS_Archive);
Result FSUSER_OpenFile(Handle *,FS_Archive,FS_Path,int,int);
Result FSFILE_Read(Handle,u32 *,u64,void *,u32);
Result FSFILE_GetSize(Handle,u64 *);
Result FSFILE_Close(Handle);
""",
    'fixture.c':r"""#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "storage.h"
#include "updater_net.h"
#include "firm_gate.h"
#include APP_SOURCE
static unsigned checks,mode,netCalls,beginCalls,writeCalls,installCalls,closeCalls,currentCalls,prepareCalls,gateCalls,freeCalls,lockDepth,bytesWritten,sequence,flushSequence,gateSequence;
static char releaseJson[256],manifestJson[512];
static struct Thread_tag mockThread;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1);}}while(0)
void LightLock_Init(LightLock *p){*p=1;}
void LightLock_Lock(LightLock *p){(void)p;CHECK(!lockDepth);++lockDepth;}
void LightLock_Unlock(LightLock *p){(void)p;CHECK(lockDepth==1);--lockDepth;}
Thread threadCreate(void (*entry)(void *),void *arg,size_t size,int prio,int core,bool detached){CHECK(size==65536&&prio==48&&core==-2&&!detached);if(mode==1)return NULL;mockThread=(struct Thread_tag){entry,arg,false};return &mockThread;}
Result threadJoin(Thread thread,u64 timeout){CHECK(thread==&mockThread);if(!thread->done&&timeout==UINT64_MAX){thread->entry(thread->arg);thread->done=true;}return thread->done?0:-1;}
void threadFree(Thread thread){CHECK(thread==&mockThread&&thread->done);++freeCalls;}
FS_Path fsMakePath(int type,const void *data){return (FS_Path){type,data};}
Result FSUSER_OpenArchive(FS_Archive *archive,int id,FS_Path path){CHECK(id==ARCHIVE_SDMC&&path.type==PATH_EMPTY);if(mode==12)return -1;*archive=7;return 0;}
Result FSUSER_CloseArchive(FS_Archive archive){CHECK(archive==7);return 0;}
Result FSUSER_OpenFile(Handle *file,FS_Archive archive,FS_Path path,int flags,int attrs){CHECK(archive==7&&path.type==PATH_ASCII&&flags==FS_OPEN_READ&&!attrs);CHECK(!strcmp(path.data,installRequested?"/boot.firm.13.4Y.tmp":"/boot.firm"));if(mode==13)return -1;*file=42;return 0;}
Result FSFILE_GetSize(Handle file,u64 *size){CHECK(file==42);*size=1024;return 0;}
Result FSFILE_Read(Handle file,u32 *actual,u64 offset,void *data,u32 size){CHECK(file==42);(void)offset;memset(data,0,size);*actual=size;return 0;}
Result FSFILE_Close(Handle file){CHECK(file==42);return 0;}
UpdateResult UpdaterFirm_Gate(UpdaterFirmRead read,void *context,uint32_t size){CHECK(read&&context&&size==1024);++gateCalls;gateSequence=++sequence;return mode==10?UPDATE_ERR_SIZE:UPDATE_OK;}
int UpdateNet_Get(const char *url,uint32_t limit,UpdateHttpSink sink,void *context,const UpdateHooks *hooks){
++netCalls;CHECK(hooks&&hooks->cancel&&hooks->progress);if(hooks->cancel(hooks->context))return UPDATE_CANCELLED;if(mode==6)return UPDATE_ERR_NETWORK;
const char *data;if(strstr(url,"/releases/latest")){CHECK(limit==65536&&strstr(url,"/Yu08083/Luma3DS-13.4Y/"));data=releaseJson;}
else if(strstr(url,"/update.json")){CHECK(limit==2048);data=manifestJson;}
else {CHECK(limit==1024&&strstr(url,"/v13.4Y/boot.firm"));char firm[1024];memset(firm,'F',sizeof(firm));if(mode==11)UpdaterApp_Cancel();return sink(context,firm,mode==7?1023:1024);}
for(size_t size=strlen(data),offset=0;offset<size;){uint32_t n=(uint32_t)(size-offset);if(n>7)n=7;int result=sink(context,data+offset,n);if(result)return result;offset+=n;}return 0;
}
int UpdateStorage_CurrentMatches(const UpdateManifest *m,const UpdateHooks *h){CHECK(m->size==1024&&h);++currentCalls;return mode==2?1:mode==3?UPDATE_ERR_SD:0;}
UpdateResult UpdateStorage_Begin(const UpdateManifest *m,const UpdateHooks *h){CHECK(m->size==1024&&h);++beginCalls;return mode==5?UPDATE_ERR_SD:UPDATE_OK;}
UpdateResult UpdateStorage_Write(const void *p,uint32_t n){CHECK(p&&n<=1024);++writeCalls;bytesWritten+=n;return mode==8?UPDATE_ERR_SD:UPDATE_OK;}
UpdateResult UpdaterStorage_PrepareRead(void){++prepareCalls;flushSequence=++sequence;return mode==4?UPDATE_ERR_SD:UPDATE_OK;}
UpdateResult UpdateStorage_Install(void){CHECK(flushSequence&&gateSequence>flushSequence);++installCalls;return mode==9?UPDATE_ERR_HASH:UPDATE_OK;}
void UpdateStorage_Close(void){++closeCalls;}
static void reset(void){
mode=netCalls=beginCalls=writeCalls=installCalls=closeCalls=currentCalls=prepareCalls=gateCalls=freeCalls=bytesWritten=sequence=flushSequence=gateSequence=0;CHECK(!lockDepth);
UpdaterApp_Init();
strcpy(releaseJson,"{\"tag_name\":\"v13.4Y\",\"draft\":false,\"prerelease\":false}");
strcpy(manifestJson,"{\"schema\":1,\"version\":\"v13.4Y\",\"asset\":\"boot.firm\",\"size\":1024,\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\"}");
}
static void work(void){CHECK(view.busy&&worker);worker->entry(worker->arg);worker->done=true;CHECK(!view.busy&&view.stage==UPDATE_STAGE_DONE&&!lockDepth);}
int main(void){
reset();CHECK(!UpdaterApp_RequestInstall());CHECK(UpdaterApp_RequestCheck());CHECK(!UpdaterApp_RequestCheck());work();CHECK(view.result==UPDATE_OK&&view.available&&netCalls==2&&currentCalls==1&&!beginCalls&&!gateCalls&&!strcmp(view.version,"v13.4Y"));
for(unsigned m=1;m<=3;++m){reset();mode=m;bool started=UpdaterApp_RequestCheck();CHECK(m==1?!started:started);if(started)work();CHECK(!view.available);CHECK(view.result==(m==1?UPDATE_ERR_BUSY:m==2?UPDATE_CURRENT:UPDATE_ERR_SD));CHECK(gateCalls==(m==2));}
reset();strcpy(releaseJson,"{\"tag_name\":\"../evil\",\"draft\":false,\"prerelease\":false}");CHECK(UpdaterApp_RequestCheck());work();CHECK(view.result==UPDATE_ERR_METADATA&&netCalls==1&&!currentCalls);
reset();CHECK(UpdaterApp_RequestCheck());UpdaterApp_Cancel();work();CHECK(view.result==UPDATE_CANCELLED&&!view.available);
for(unsigned m=0;m<=13;++m){
reset();availableManifest.size=1024;strcpy(availableManifest.version,"v13.4Y");view.available=true;mode=m;
bool started=UpdaterApp_RequestInstall();CHECK(m==1?!started:started);if(!started)continue;work();
int expected=m==4||m==5||m==8||m==12||m==13?UPDATE_ERR_SD:m==6?UPDATE_ERR_NETWORK:m==7||m==10?UPDATE_ERR_SIZE:m==9?UPDATE_ERR_HASH:m==11?UPDATE_CANCELLED:UPDATE_OK;
CHECK(view.result==expected&&closeCalls==1);
CHECK(installCalls==(expected==UPDATE_OK||m==9));
if(!expected)CHECK(!view.available&&bytesWritten==1024&&prepareCalls==1&&gateCalls==1);
}
reset();CHECK(UpdaterApp_RequestCheck());UpdaterApp_Shutdown();CHECK(!view.busy&&view.result==UPDATE_CANCELLED&&freeCalls==1&&!UpdaterApp_RequestCheck());
UpdaterAppView snapshot;UpdaterApp_Snapshot(&snapshot);CHECK(!memcmp(&snapshot,&view,sizeof(view)));
printf("%u\n",checks);return 0;
}
"""
    }
    for name,value in files.items():(OUT/name).write_bytes(value.encode())
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    target=OUT/('app.exe' if os.name=='nt' else 'app')
    source=APP/'source/app.c'
    result=subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-DAPP_SOURCE="'+source.as_posix()+'"','-I'+str(OUT),'-I'+str(APP/'include'),'-I'+str(CORE/'include'),str(OUT/'fixture.c'),str(CORE/'source/updater_format.c'),'-o',str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    result=subprocess.run([str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    report=dict(result='pass',checks=int(result.stdout.strip()),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),hardware_tested=False,boundary='actual app orchestration; platform threads/network/storage mocked; flush before module gate and safe shutdown verified')
    (OUT/'proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))

if __name__=='__main__':main()
