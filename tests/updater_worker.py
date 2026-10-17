from pathlib import Path
import hashlib
import json
import os
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests/worker-fixture'

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'menus').mkdir(exist_ok=True)
    files={
    '3ds.h':r"""#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef uint8_t u8;typedef uint32_t u32;typedef int64_t s64;typedef int32_t Result;typedef uint32_t Handle;typedef int LightLock;
#define R_FAILED(v) ((v)<0)
#define CORE_SYSTEM 1
#define KEY_A 1
#define KEY_B 2
#define KEY_X 1024
void LightLock_Lock(LightLock *);
void LightLock_Unlock(LightLock *);
void svcSleepThread(int64_t);
""",
    'menu.h':r"""#pragma once
#include "3ds.h"
extern bool g_safeMode,menuShouldExit,preTerminationRequested;
extern u32 g_accentColor,g_textColor;
extern unsigned char g_menuWorkBuf[65536];
u32 waitInputWithTimeout(int32_t);
""",
    'MyThread.h':r"""#pragma once
#include "3ds.h"
typedef struct MyThread {Handle handle;void (*ep)(void);bool finished;void *stacktop;} MyThread;
Result MyThread_Create(MyThread *,void (*)(void),void *,u32,int,int);
Result MyThread_Join(MyThread *,s64);
""",
    'menus/lang.h':r"""#pragma once
#define LANG_JP 1
extern int g_language;
""",
    'menus/ftpd.h':r"""#pragma once
#include "MyThread.h"
extern volatile bool ftpdEnabled;
MyThread *ftpdCreateThread(void);
Result Ftpd_Disable(s64);
""",
    'draw.h':r"""#pragma once
#include "3ds.h"
void Draw_Lock(void);void Draw_ClearFramebuffer(void);void Draw_DrawString(int,int,u32,const char *);void Draw_DrawFormattedString(int,int,u32,const char *,...);void Draw_FlushFramebuffer(void);void Draw_Unlock(void);
""",
    'fmt.h':'#pragma once\n',
    'fixture.c':r"""#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "updater_storage.h"
#include "updater_net.h"
#include MENU_SOURCE
static unsigned checks,mode,netCalls,beginCalls,writeCalls,installCalls,closeCalls,currentCalls,stopCalls,restartCalls,lockDepth;
static unsigned bytesWritten;
static char releaseJson[1024],manifestJson[1024];
bool g_safeMode,menuShouldExit,preTerminationRequested;
u32 g_accentColor,g_textColor;
unsigned char g_menuWorkBuf[65536];
int g_language;
volatile bool ftpdEnabled;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1);}}while(0)
void LightLock_Lock(LightLock *p){(void)p;CHECK(lockDepth==0);++lockDepth;}
void LightLock_Unlock(LightLock *p){(void)p;CHECK(lockDepth==1);--lockDepth;}
void svcSleepThread(int64_t t){(void)t;}
void Draw_Lock(void){}void Draw_ClearFramebuffer(void){}void Draw_DrawString(int x,int y,u32 c,const char *s){(void)x;(void)y;(void)c;(void)s;}void Draw_DrawFormattedString(int x,int y,u32 c,const char *s,...){(void)x;(void)y;(void)c;(void)s;}void Draw_FlushFramebuffer(void){}void Draw_Unlock(void){}
u32 waitInputWithTimeout(int32_t n){(void)n;return KEY_B;}

Result MyThread_Create(MyThread *t,void (*entry)(void),void *stack,u32 size,int prio,int affinity){CHECK(stack&&size>=32768&&prio==57&&affinity==1);if(mode==1)return -1;t->ep=entry;t->handle=1;return 0;}
Result MyThread_Join(MyThread *t,s64 timeout){CHECK(timeout==0);if(view.active)return -1;t->handle=0;return 0;}
Result Ftpd_Disable(s64 timeout){CHECK(timeout==2000000000LL);++stopCalls;ftpdEnabled=false;return mode==4?-1:0;}
MyThread *ftpdCreateThread(void){CHECK(ftpdEnabled);++restartCalls;return 0;}
int UpdateNet_Get(const char *url,uint32_t limit,UpdateHttpSink sink,void *context,const UpdateHooks *hooks){
++netCalls;CHECK(hooks&&hooks->cancel&&hooks->progress);if(hooks->cancel(hooks->context))return UPDATE_CANCELLED;
if(mode==6)return UPDATE_ERR_NETWORK;
const char *data;
if(strstr(url,"/releases/latest")){CHECK(limit==65536);data=releaseJson;}
else if(strstr(url,"/update.json")){CHECK(limit==2048);data=manifestJson;}
else {CHECK(strstr(url,"/v13.4Y/boot.firm")&&limit==1024);static char firm[1024];memset(firm,'F',sizeof(firm));int result=sink(context,firm,mode==7?1023:1024);return result;}
size_t size=strlen(data),offset=0;while(offset<size){uint32_t n=(uint32_t)(size-offset);if(n>7)n=7;int result=sink(context,data+offset,n);if(result)return result;offset+=n;}return 0;
}
int UpdateStorage_CurrentMatches(const UpdateManifest *m,const UpdateHooks *h){CHECK(m->size==1024&&h);++currentCalls;return mode==2?1:mode==3?UPDATE_ERR_SD:0;}
UpdateResult UpdateStorage_Begin(const UpdateManifest *m,const UpdateHooks *h){CHECK(m->size==1024&&h);++beginCalls;return mode==5?UPDATE_ERR_SD:UPDATE_OK;}
UpdateResult UpdateStorage_Write(const void *p,uint32_t n){CHECK(p&&n<=1024);++writeCalls;bytesWritten+=n;return mode==8?UPDATE_ERR_SD:UPDATE_OK;}
UpdateResult UpdateStorage_Install(void){++installCalls;return mode==9?UPDATE_ERR_HASH:UPDATE_OK;}
void UpdateStorage_Close(void){++closeCalls;}
static void reset(void){
memset(&view,0,sizeof(view));memset(&updateThread,0,sizeof(updateThread));memset(&availableManifest,0,sizeof(availableManifest));
mode=netCalls=beginCalls=writeCalls=installCalls=closeCalls=currentCalls=stopCalls=restartCalls=bytesWritten=0;
g_safeMode=menuShouldExit=preTerminationRequested=false;ftpdEnabled=false;CHECK(lockDepth==0);
strcpy(releaseJson,"{\"tag_name\":\"v13.4Y\",\"draft\":false,\"prerelease\":false}");
strcpy(manifestJson,"{\"schema\":1,\"version\":\"v13.4Y\",\"asset\":\"boot.firm\",\"size\":1024,\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\"}");
}
static void work(void){CHECK(view.active&&updateThread.handle);updateThread.ep();CHECK(!view.active&&view.stage==UPDATE_STAGE_DONE&&lockDepth==0);}
int main(void){
reset();CHECK(view.stage==UPDATE_STAGE_NONE);CHECK(begin(false));CHECK(!begin(false));work();CHECK(view.result==UPDATE_OK&&view.available&&netCalls==2&&currentCalls==1&&!strcmp(view.version,"v13.4Y"));CHECK(!stopCalls&&!beginCalls&&!installCalls);
for(unsigned m=1;m<=3;++m){reset();mode=m;bool started=begin(false);CHECK(m==1?!started:started);if(started)work();CHECK(!view.available);CHECK(view.result==(m==1?UPDATE_ERR_BUSY:m==2?UPDATE_CURRENT:UPDATE_ERR_SD));}
reset();strcpy(releaseJson,"{\"tag_name\":\"../evil\",\"draft\":false,\"prerelease\":false}");CHECK(begin(false));work();CHECK(view.result==UPDATE_ERR_METADATA&&netCalls==1&&!currentCalls);
reset();strcpy(manifestJson,"{\"schema\":2}");CHECK(begin(false));work();CHECK(view.result==UPDATE_ERR_METADATA&&netCalls==2&&!currentCalls);
reset();CHECK(begin(false));view.cancel=true;work();CHECK(view.result==UPDATE_CANCELLED&&!view.available);
for(unsigned m=0;m<=9;++m){
reset();availableManifest.size=1024;strcpy(availableManifest.version,"v13.4Y");view.available=true;ftpdEnabled=true;mode=m;
bool started=begin(true);CHECK(m==1?!started:started);if(!started)continue;work();
CHECK(stopCalls==1);
if(m==4){CHECK(view.result==UPDATE_ERR_BUSY&&!beginCalls&&!netCalls&&!restartCalls);continue;}
CHECK(restartCalls==1&&ftpdEnabled&&closeCalls==1);
int expected=m==5||m==8?UPDATE_ERR_SD:m==6?UPDATE_ERR_NETWORK:m==7?UPDATE_ERR_SIZE:m==9?UPDATE_ERR_HASH:UPDATE_OK;
CHECK(view.result==expected);
CHECK(installCalls==(expected==UPDATE_OK||m==9));
CHECK(beginCalls==1);
if(!expected)CHECK(!view.available&&bytesWritten==1024&&writeCalls==1);
}
reset();availableManifest.size=1024;strcpy(availableManifest.version,"v13.4Y");CHECK(begin(true));view.cancel=true;work();CHECK(view.result==UPDATE_CANCELLED&&!beginCalls&&!netCalls&&closeCalls==1);
reset();availableManifest.size=1024;strcpy(availableManifest.version,"v13.4Y");CHECK(begin(true));preTerminationRequested=true;work();CHECK(view.result==UPDATE_CANCELLED&&closeCalls==1);
printf("%u\n",checks);return 0;
}
"""
    }
    for name,value in files.items():(OUT/name).write_bytes(value.encode())
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    target=OUT/('fixture.exe' if os.name=='nt' else 'fixture')
    source=ROS/'source/menus/updater.c'
    args=[compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-DMENU_SOURCE="'+str(source).replace('\\','/')+'"','-I'+str(OUT),'-I'+str(ROS/'include'),str(OUT/'fixture.c'),str(ROS/'source/updater_format.c'),'-o',str(target)]
    result=subprocess.run(args,env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    result=subprocess.run([str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    report=dict(result='pass',checks=int(result.stdout.strip()),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),hardware_tested=False,boundary='actual worker/job orchestration and parsers; platform threads/FTP/network/storage mocked; no real concurrent scheduling or SD IO')
    (OUT.parent/'worker-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))

if __name__=='__main__':main()
