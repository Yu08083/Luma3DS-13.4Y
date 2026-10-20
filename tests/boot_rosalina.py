import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
GENERATED=ROOT/'tests/generated'
GENERATED.mkdir(exist_ok=True)
ARM9=ROOT/'Luma3DS_v13_4/arm9/source'
def function(source,name):
    match=re.search(r'^(?:bool|void)\s+'+re.escape(name)+r'\([^;]*?\)\s*\{',source,re.M)
    assert match,name
    start=match.start();end=match.end();depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]+'\n'
prefix=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
typedef uint32_t u32;
#define HID_PAD 0
#define SPACING_Y 10
#define COLOR_TITLE 1
#define COLOR_RED 2
#define COLOR_TEXT 3
#define COLOR_WHITE 4
static struct {u32 rosalinaMenuCombo,untouched[32];} configData;
static bool writeOk=true,japanese=true,dirOk=true,closeOk=true;
static u32 inputs[128],inputCount,inputAt,saves,opened,reads;
static char entries[24][64];
static u32 fileCount,fileAt;
static bool BootLanguage_IsJapanese(void){return japanese;}
static bool writeLumaIniConfig(void){saves++;return writeOk;}
static void initScreens(void){}
static void clearScreens(bool alternate){assert(!alternate);}
static void drawString(bool top,u32 x,u32 y,u32 color,const char *text){assert(x<(top?400:320)&&y<240&&color&&text);}
static void drawFormattedString(bool top,u32 x,u32 y,u32 color,const char *fmt,...){char text[256];va_list args;va_start(args,fmt);assert(vsnprintf(text,sizeof(text),fmt,args)<(int)sizeof(text));va_end(args);drawString(top,x,y,color,text);}
static void wait(uint64_t ticks){(void)ticks;}
static u32 waitInput(bool menu){assert(menu&&inputAt<inputCount);return inputs[inputAt++];}
static void sequence(const u32 *keys,u32 n){memcpy(inputs,keys,n*4);inputCount=n;inputAt=0;}
#define RUN(...) do{const u32 keys[]={__VA_ARGS__};sequence(keys,sizeof(keys)/4);}while(0)
typedef struct {u32 tag;} DIR;
typedef struct {char fname[64];} FILINFO;
#define FR_OK 0
static int f_opendir(DIR *dir,const char *name){assert(!strcmp(name,"payloads"));dir->tag=1;fileAt=0;opened++;return dirOk?FR_OK:1;}
static int f_readdir(DIR *dir,FILINFO *info){assert(dir->tag);reads++;strcpy(info->fname,fileAt<fileCount?entries[fileAt++]:"");return FR_OK;}
static int f_closedir(DIR *dir){assert(dir->tag);dir->tag=0;return closeOk?FR_OK:1;}
'''
buttons=(ARM9/'buttons.h').read_text()
buttons='\n'.join(line for line in buttons.splitlines() if line.startswith('#define ') and not line.startswith('#define HID_PAD'))
body=function((ARM9/'config.c').read_text(),'configSetRosalinaMenuCombo')+function((ARM9/'boot_rosalina.c').read_text(),'BootRosalina_Configure')+function((ARM9/'fs.c').read_text(),'payloadMenu')
tests=r'''
int main(void){
    for(u32 i=0;i<32;i++)configData.untouched[i]=0xA5000000u+i;
    const u32 defaults=BUTTON_L1|BUTTON_DOWN|BUTTON_SELECT;
    configData.rosalinaMenuCombo=defaults;
    assert(!configSetRosalinaMenuCombo(0)&&!saves);
    assert(!configSetRosalinaMenuCombo(1u<<16)&&!saves);
    assert(!configSetRosalinaMenuCombo(BUTTON_UP|BUTTON_DOWN)&&!saves);
    assert(!configSetRosalinaMenuCombo(BUTTON_LEFT|BUTTON_RIGHT)&&!saves);
    writeOk=false;assert(!configSetRosalinaMenuCombo(BUTTON_A)&&configData.rosalinaMenuCombo==defaults&&saves==1);
    writeOk=true;
    for(unsigned language=0;language<2;language++){
        japanese=language;configData.rosalinaMenuCombo=defaults;saves=0;
        RUN(BUTTON_A,BUTTON_START);BootRosalina_Configure();assert(configData.rosalinaMenuCombo==(defaults|BUTTON_A)&&saves==1);
        RUN(BUTTON_A,BUTTON_B);BootRosalina_Configure();assert(configData.rosalinaMenuCombo==(defaults|BUTTON_A)&&saves==1);
        RUN(BUTTON_X,BUTTON_START);BootRosalina_Configure();assert(configData.rosalinaMenuCombo==defaults&&saves==2);
        RUN(BUTTON_UP,BUTTON_A,BUTTON_DOWN,BUTTON_LEFT,BUTTON_RIGHT,BUTTON_START);BootRosalina_Configure();assert(configData.rosalinaMenuCombo==(defaults|0x8000)&&saves==3);
        configData.rosalinaMenuCombo=BUTTON_A;
        RUN(BUTTON_A,BUTTON_START,BUTTON_X,BUTTON_START);BootRosalina_Configure();assert(configData.rosalinaMenuCombo==defaults&&saves==4);
        writeOk=false;RUN(BUTTON_A,BUTTON_START,BUTTON_B);BootRosalina_Configure();assert(configData.rosalinaMenuCombo==defaults&&saves==5);writeOk=true;
    }
    for(u32 i=0;i<32;i++)assert(configData.untouched[i]==0xA5000000u+i);
    char path[128]="unchanged";bool displayed=false;
    dirOk=false;RUN(BUTTON_A,BUTTON_X,BUTTON_START,BUTTON_START);assert(!payloadMenu(path,&displayed)&&displayed&&!strcmp(path,"unchanged"));
    dirOk=true;fileCount=0;RUN(BUTTON_START);assert(!payloadMenu(path,&displayed)&&displayed);
    fileCount=1;strcpy(entries[0],"GodMode9.firm");RUN(BUTTON_A);assert(payloadMenu(path,&displayed)&&displayed&&!strcmp(path,"payloads/GodMode9.firm"));
    fileCount=3;strcpy(entries[0],"._hidden.firm");strcpy(entries[1],"ignored.txt");strcpy(entries[2],"GodMode9.firm");RUN(BUTTON_A);assert(payloadMenu(path,&displayed)&&!strcmp(path,"payloads/GodMode9.firm"));
    fileCount=20;for(u32 i=0;i<20;i++)sprintf(entries[i],"tool%02u.firm",i);
    RUN(BUTTON_UP,BUTTON_A,BUTTON_B,BUTTON_LEFT,BUTTON_DOWN,BUTTON_A);assert(payloadMenu(path,&displayed)&&!strcmp(path,"payloads/tool01.firm"));
    RUN(BUTTON_RIGHT,BUTTON_A,BUTTON_X,BUTTON_START,BUTTON_B);assert(!payloadMenu(path,&displayed));
    closeOk=false;RUN(BUTTON_B);assert(!payloadMenu(path,&displayed)&&displayed);
    puts("Boot Rosalina: legal combinations, save/cancel/default/failure, EN/JP navigation, zero/one/twenty payloads, scrolling, chainloader paths and canaries passed");
}
'''
source=GENERATED/'boot_rosalina.c';source.write_text(prefix+buttons+'\n'+body+tests,encoding='utf-8',newline='\n')
compiler=os.environ.get('CC','cc')
if not shutil.which(compiler) and os.name=='nt':compiler='C:/msys64/ucrt64/bin/gcc.exe'
env=dict(os.environ);env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
binary=source.with_suffix('.exe' if os.name=='nt' else '')
subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror',str(source),'-o',str(binary)],check=True,env=env)
subprocess.run([str(binary)],check=True,env=env)
