import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/plugin-menu-tests'


def block(text,start):
    begin=text.index(start)
    opening=text.index('{',begin)
    depth=1
    end=opening+1
    while depth:
        assert end<len(text)
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    return text[begin:end]


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    menu_path=ROS/'source/menus.c'
    loader_path=ROS/'source/plugin/plgloader.c'
    navigation_path=ROS/'source/menu.c'
    draw_path=ROS/'source/draw.c'
    language_path=ROS/'source/menus/lang.c'
    menu=block(menu_path.read_text(encoding='utf-8'),'Menu rosalinaMenu =')+';\n'
    loader=loader_path.read_text(encoding='utf-8')
    callback=block(loader,'void        PluginLoader__MenuCallback(')
    update=block(loader,'void        PluginLoader__UpdateMenu(')
    source=navigation_path.read_text(encoding='utf-8')
    navigation='\n'.join(block(source,name) for name in ['static inline bool menuItemIsHidden(', 'u32 menuCountItems(', 'static u32 menuVisibleItems(', 'static u32 menuMoveSelection('])
    draw=draw_path.read_text(encoding='utf-8')
    render=draw[draw.index('static u32 drawDirtyFirst'):draw.index('u32 Draw_DrawFormattedString(')]
    render=render.replace('(volatile u16 *)FB_BOTTOM_VRAM_ADDR','(volatile u16 *)vram')
    language=language_path.read_text(encoding='utf-8')
    labels=language[language.index('typedef struct { const char *english;'):language.index('\nconst char *Lang(u32 id)')]
    method_names=set(re.findall(r'\.method\s*=\s*&?([A-Za-z_]\w*)',menu))
    visibility_names=set(re.findall(r'\.visibility\s*=\s*&?([A-Za-z_]\w*)',menu))
    menu_names=set(re.findall(r'\.menu\s*=\s*&?([A-Za-z_]\w*)',menu))
    prefix=r'''#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "text.h"
#include "font.h"
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t s32;
typedef enum {MENU_END,METHOD,MENU} MenuItemAction;
typedef struct MenuItem {const char *title;MenuItemAction action_type;union {struct Menu *menu;void(*method)(void);};bool(*visibility)(void);} MenuItem;
typedef struct Menu {const char *title;MenuItem items[32];} Menu;
static struct {bool isEnabled;} PluginLoaderCtx;
static u32 saves,checks;
static u32 cheatInvocations;
static bool visible;
static int g_language;
static bool g_langLoaded=true;
#define LANG_JP 1
static void Lang_Load(void){g_langLoaded=true;}
static void LumaConfig_RequestSaveSettings(void){++saves;}
void PluginLoader__MenuCallback(void);
void PluginLoader__UpdateMenu(void);
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240
#define SPACING_X 6
#define SPACING_Y 11
#define FB_BOTTOM_SIZE (320*240*2)
static u16 pixels[320*240+32];
#define vram (pixels+16)
static u16 g_menuBgColor;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %d: %s\n",__LINE__,#v);return 1;}}while(0)
'''
    declarations='\n'.join('static void '+name+'(void){'+('++cheatInvocations;' if name=='RosalinaMenu_Cheats' else '')+'}' for name in sorted(method_names-{'PluginLoader__MenuCallback'}))+'\n'
    declarations+='\n'.join('static bool '+name+'(void){return visible;}' for name in sorted(visibility_names))+'\n'
    declarations+='\n'.join('static Menu '+name+';' for name in sorted(menu_names))+'\n'
    harness=r'''
static u32 pluginIndex(void){for(u32 i=0;i<menuCountItems(&rosalinaMenu);++i)if(rosalinaMenu.items[i].action_type==METHOD&&rosalinaMenu.items[i].method==PluginLoader__MenuCallback)return i;return UINT32_MAX;}
static void inserted(void){}
static int snapshot(const char *name,u32 selected){
for(u32 i=0;i<320*240+32;++i)pixels[i]=0x5A5A;
memset(vram,0,FB_BOTTOM_SIZE);
u32 items[32],count=menuVisibleItems(&rosalinaMenu,items);
u32 selectedRow=0;for(u32 i=0;i<count;++i)if(items[i]==selected)selectedRow=i;
u32 first=selectedRow>=16?selectedRow-15:0;
for(u32 i=first;i<count&&i<first+16;++i)Draw_DrawMenuRow(30+(i-first)*SPACING_Y,0xFFFF,Lang_Translate(rosalinaMenu.items[items[i]].title),0xC49F,items[i]==selected,0);
for(u32 i=0;i<16;++i){CHECK(pixels[i]==0x5A5A);CHECK(pixels[320*240+16+i]==0x5A5A);}
FILE *file=fopen(name,"wb");CHECK(file);fprintf(file,"P6\n320 240\n255\n");
for(u32 y=0;y<240;++y){for(u32 x=0;x<320;++x){u16 value=vram[x*240+239-y];unsigned char rgb[3]={(unsigned char)(((value>>11)&31)*255/31),(unsigned char)(((value>>5)&63)*255/63),(unsigned char)((value&31)*255/31)};fwrite(rgb,1,3,file);}}
CHECK(!fclose(file));return 0;}
int main(void){
Menu original=rosalinaMenu;
u32 variants=0;
for(u32 variant=0;variant<5;++variant)for(u32 shown=0;shown<2;++shown){
rosalinaMenu=original;visible=shown;u32 count=menuCountItems(&rosalinaMenu),index=pluginIndex();CHECK(index!=UINT32_MAX&&count<32);
if(variant==1){MenuItem temp=rosalinaMenu.items[3];rosalinaMenu.items[3]=rosalinaMenu.items[index];rosalinaMenu.items[index]=temp;}
if(variant==2){MenuItem temp=rosalinaMenu.items[index];memmove(rosalinaMenu.items+index,rosalinaMenu.items+index+1,(20-index)*sizeof(MenuItem));rosalinaMenu.items[20]=temp;}
if(variant==3){memmove(rosalinaMenu.items+1,rosalinaMenu.items,(count+1)*sizeof(MenuItem));rosalinaMenu.items[0]=(MenuItem){"Inserted item",METHOD,.method=inserted};}
if(variant==4)for(u32 a=0,b=count-1;a<b;++a,--b){MenuItem temp=rosalinaMenu.items[a];rosalinaMenu.items[a]=rosalinaMenu.items[b];rosalinaMenu.items[b]=temp;}
index=pluginIndex();count=menuCountItems(&rosalinaMenu);CHECK(index!=UINT32_MAX);
Menu before=rosalinaMenu;
u32 list[32],beforeCount=menuVisibleItems(&rosalinaMenu,list);
u32 moves[32][4];for(u32 item=0;item<count;++item){moves[item][0]=menuMoveSelection(&rosalinaMenu,item,1,true);moves[item][1]=menuMoveSelection(&rosalinaMenu,item,-1,true);moves[item][2]=menuMoveSelection(&rosalinaMenu,item,16,false);moves[item][3]=menuMoveSelection(&rosalinaMenu,item,-16,false);}
for(u32 enabled=0;enabled<2;++enabled){PluginLoaderCtx.isEnabled=enabled;PluginLoader__UpdateMenu();
CHECK(!strcmp(rosalinaMenu.items[index].title,enabled?"Plugin Loader: [Enabled]":"Plugin Loader: [Disabled]"));
for(u32 item=0;item<count;++item){CHECK(rosalinaMenu.items[item].title&&*rosalinaMenu.items[item].title);if(item!=index)CHECK(rosalinaMenu.items[item].title==before.items[item].title);CHECK(rosalinaMenu.items[item].action_type==before.items[item].action_type);CHECK(rosalinaMenu.items[item].method==before.items[item].method);CHECK(rosalinaMenu.items[item].visibility==before.items[item].visibility);}
u32 afterList[32],afterCount=menuVisibleItems(&rosalinaMenu,afterList);CHECK(afterCount==beforeCount&&!memcmp(list,afterList,afterCount*sizeof(u32)));
for(u32 item=0;item<count;++item){CHECK(moves[item][0]==menuMoveSelection(&rosalinaMenu,item,1,true));CHECK(moves[item][1]==menuMoveSelection(&rosalinaMenu,item,-1,true));CHECK(moves[item][2]==menuMoveSelection(&rosalinaMenu,item,16,false));CHECK(moves[item][3]==menuMoveSelection(&rosalinaMenu,item,-16,false));}
u32 saved=saves,cheats=cheatInvocations;CHECK(rosalinaMenu.items[index].method==PluginLoader__MenuCallback);rosalinaMenu.items[index].method();CHECK(PluginLoaderCtx.isEnabled!=enabled&&saves==saved+1&&cheatInvocations==cheats);CHECK(!strcmp(rosalinaMenu.items[index].title,enabled?"Plugin Loader: [Disabled]":"Plugin Loader: [Enabled]"));
for(u32 lang=0;lang<2;++lang){g_language=lang;const char *label=Lang_Translate(rosalinaMenu.items[index].title);CHECK(Text_MeasureWidth(label)==(lang?80:enabled?150:144));
memset(vram,0,FB_BOTTOM_SIZE);Draw_DrawMenuRow(30,0xFFFF,label,0xC49F,true,0);u32 cursor=0;for(u32 x=9;x<15;++x)for(u32 y=30;y<40;++y)if(vram[x*240+239-y]==0xC49F)++cursor;CHECK(cursor>0);
if(!variant&&shown){char name[64];snprintf(name,sizeof(name),"plugin-%s-%s.ppm",lang?"jp":"en",enabled?"disabled":"enabled");CHECK(!snapshot(name,index));}
}
}
++variants;
}
g_language=LANG_JP;CHECK(Text_MeasureWidth(Lang_Translate("Plugin Loader: [Enabled]"))==Text_MeasureWidth(Lang_Translate("Plugin Loader: [Disabled]")));
printf("{\"checks\":%u,\"variants\":%u}\n",checks,variants);return 0;
}
'''
    compiler=os.environ.get('CC','cc')
    if not shutil.which(compiler) and os.name=='nt':compiler='C:/msys64/ucrt64/bin/gcc.exe'
    env=dict(os.environ)
    resolved=shutil.which(compiler) or compiler
    env['PATH']=str(Path(resolved).parent)+os.pathsep+env.get('PATH','')
    def build_run(body,name,test_code=harness):
        path=OUT/(name+'.c')
        path.write_bytes((prefix+declarations+menu+callback+'\n'+body+'\n'+navigation+'\n'+labels+'\n'+render+'\n'+test_code).encode())
        target=OUT/(name+('.exe' if os.name=='nt' else ''))
        result=subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-finput-charset=UTF-8','-I'+str(ROS/'include'),str(path),str(ROS/'source/text.c'),'-o',str(target)],env=env,capture_output=True,text=True)
        assert result.returncode==0,result.stdout+result.stderr
        folder=OUT if name=='actual' else OUT/'negative'
        folder.mkdir(exist_ok=True)
        return subprocess.run([str(target)],cwd=folder,env=env,capture_output=True,text=True)
    result=build_run(update,'actual')
    assert result.returncode==0,result.stdout+result.stderr
    counts=json.loads(result.stdout)
    fixed='void PluginLoader__UpdateMenu(void){static const char *status[]={"Plugin Loader: [Disabled]","Plugin Loader: [Enabled]"};rosalinaMenu.items[4].title=status[PluginLoaderCtx.isEnabled];}'
    negative=build_run(fixed,'fixed-index-mutant')
    assert negative.returncode!=0,'Hardcoded index4 unexpectedly survived menu reorder tests'
    prior=fixed.replace('items[4]','items[3]')
    prior_case=harness.replace('int main(void){','int regressions_main(void){',1)+r'''
int main(void){PluginLoaderCtx.isEnabled=true;PluginLoader__UpdateMenu();CHECK(!strcmp(rosalinaMenu.items[3].title,"Plugin Loader: [Enabled]"));CHECK(rosalinaMenu.items[3].method==RosalinaMenu_Cheats);u32 before=cheatInvocations,saved=saves;rosalinaMenu.items[3].method();CHECK(cheatInvocations==before+1&&saves==saved&&PluginLoaderCtx.isEnabled);CHECK(pluginIndex()!=3);return 0;}
'''
    prior_result=build_run(prior,'old-label-callback-mismatch',prior_case)
    assert prior_result.returncode==0,prior_result.stdout+prior_result.stderr
    snapshots=[]
    try:
        from PIL import Image
        for path in OUT.glob('plugin-*.ppm'):
            Image.open(path).save(path.with_suffix('.png'))
            snapshots.append(path.with_suffix('.png').name)
    except ImportError:
        pass
    paths=[menu_path,loader_path,navigation_path,draw_path,language_path,ROS/'source/text.c',Path(__file__)]
    report={'result':'pass',**counts,'fixed_index4_negative_control_rejected':True,'old_index3_label_callback_mismatch_reproduced':True,'source_sha256':{str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths},'boundary':'Actual main menu initializer, plugin update/callback, visibility/navigation helpers, UTF8 measurement/translation and menu-row rasterizer. Other menu callbacks and hardware visibility services are stubs.','snapshots':snapshots,'hardware_tested':False}
    (OUT/'proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report))


if __name__=='__main__':main()
