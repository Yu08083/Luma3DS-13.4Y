from pathlib import Path
import hashlib
import json
import os
import subprocess

ROOT=Path(__file__).resolve().parents[2]
APP=ROOT/'updater'
CORE=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=APP/'build/main-test'

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    files={
    '3ds.h':r"""#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t Result;
#define R_FAILED(v) ((v)<0)
#define R_SUCCEEDED(v) ((v)>=0)
#define GSP_BGR8_OES 1
#define CFG_LANGUAGE_JP 0
#define GFX_TOP 0
#define GFX_BOTTOM 1
#define GFX_LEFT 0
Result fsInit(void);void fsExit(void);
void gfxInit(int,int,bool);void gfxSet3D(bool);void gfxExit(void);
Result cfguInit(void);Result CFGU_GetSystemLanguage(u8 *);void cfguExit(void);
bool aptMainLoop(void);void aptSetHomeAllowed(bool);
void hidScanInput(void);u32 hidKeysDown(void);
u8 *gfxGetFramebuffer(int,int,u16 *,u16 *);
void gfxFlushBuffers(void);void gfxSwapBuffers(void);void gspWaitForVBlank(void);
""",
    'fixture.c':r"""#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include "ui.h"
#define main appMain
#include MAIN_SOURCE
#undef main
static unsigned checks,mode,step,frame,appCalls,cleanup,draws,actions,cancels;
static bool busy,shut;
static u8 top[400*240*3],bottom[320*240*3];
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1);}}while(0)
Result fsInit(void){CHECK(!step);step=1;return mode==1?-1:0;}
void gfxInit(int topFmt,int bottomFmt,bool useVram){CHECK(step==1&&topFmt==1&&bottomFmt==1&&!useVram);step=2;}
void gfxSet3D(bool enabled){CHECK(!enabled&&step==2);step=3;}
Result cfguInit(void){CHECK(step==3);return mode==2?-1:0;}
Result CFGU_GetSystemLanguage(u8 *value){CHECK(step==3&&*value==0);*value=mode==3?1:0;return mode==4?-1:0;}
void cfguExit(void){CHECK(step==3);}
void UpdaterApp_Init(void){CHECK(step==3);step=4;++appCalls;}
void UpdaterUi_Init(UpdaterUi *ui,bool japanese){CHECK(step==4&&japanese==(mode!=3));memset(ui,0,sizeof(*ui));ui->japanese=japanese;step=5;}
bool aptMainLoop(void){CHECK(step==5);return frame<5;}
void hidScanInput(void){CHECK(step==5);}
u32 hidKeysDown(void){return frame;}
void UpdaterApp_Snapshot(UpdaterAppView *view){CHECK(step==5);memset(view,0,sizeof(*view));view->busy=busy;}
void aptSetHomeAllowed(bool allowed){CHECK(allowed==!busy);if(shut)CHECK(step==6);}
UpdaterUiAction UpdaterUi_Input(UpdaterUi *ui,const UpdaterAppView *view,u32 keys){CHECK(ui&&view->busy==busy&&keys==frame);static const UpdaterUiAction values[]={UPDATER_UI_CHECK,UPDATER_UI_INSTALL,UPDATER_UI_CANCEL,UPDATER_UI_NONE,UPDATER_UI_EXIT};return mode==5?UPDATER_UI_NONE:values[frame];}
bool UpdaterApp_RequestCheck(void){CHECK(!frame);++actions;busy=true;return true;}
bool UpdaterApp_RequestInstall(void){CHECK(frame==1&&busy);++actions;return false;}
void UpdaterApp_Cancel(void){CHECK(busy);++cancels;}
u8 *gfxGetFramebuffer(int screen,int side,u16 *width,u16 *height){CHECK(side==GFX_LEFT);*width=240;*height=screen==GFX_TOP?400:320;return screen==GFX_TOP?top:bottom;}
void UpdaterUi_Draw(UpdaterUi *ui,const UpdaterAppView *view,const UpdaterUiSurface *a,const UpdaterUiSurface *b){CHECK(ui&&view->busy==busy);CHECK(a->pixels==top&&a->width==400&&a->height==240&&a->size==sizeof(top));CHECK(b->pixels==bottom&&b->width==320&&b->height==240&&b->size==sizeof(bottom));++draws;}
void gfxFlushBuffers(void){CHECK(draws==frame+1);}
void gfxSwapBuffers(void){CHECK(draws==frame+1);}
void gspWaitForVBlank(void){++frame;}
void UpdaterApp_Shutdown(void){CHECK(step==5);CHECK(mode==5?!busy:busy);busy=false;shut=true;step=6;}
void gfxExit(void){CHECK(shut&&!busy&&step==6);++cleanup;step=7;}
void fsExit(void){CHECK(step==7&&cleanup==1);++cleanup;step=8;}
int main(void){for(mode=0;mode<6;++mode){step=frame=appCalls=cleanup=draws=actions=cancels=0;busy=shut=false;int result=appMain();if(mode==1){CHECK(result==1&&step==1&&!appCalls&&!cleanup&&!draws);continue;}CHECK(!result&&step==8&&appCalls==1&&cleanup==2&&draws==5&&shut&&!busy);CHECK(mode==5?!actions&&!cancels:actions==2&&cancels==2);}printf("%u\n",checks);return 0;}
"""
    }
    for name,value in files.items():(OUT/name).write_bytes(value.encode())
    compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
    env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
    target=OUT/('main.exe' if os.name=='nt' else 'main')
    source=APP/'source/main.c'
    result=subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-DMAIN_SOURCE="'+source.as_posix()+'"','-I'+str(OUT),'-I'+str(APP/'include'),'-I'+str(CORE/'include'),str(OUT/'fixture.c'),'-o',str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    result=subprocess.run([str(target)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    report=dict(result='pass',checks=int(result.stdout.strip()),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),hardware_tested=False,boundary='actual main; libctru graphics/input/app mocked; physical framebuffer rotation, JP/EN selection, exit-before-graphics-and-FS verified')
    (OUT/'proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))

if __name__=='__main__':main()
