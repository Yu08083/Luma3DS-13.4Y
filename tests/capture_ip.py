import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


root = Path(__file__).resolve().parents[1]
base = root / 'Luma3DS_v13_4/sysmodules/rosalina'
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, default=root / 'build/capture-ip-tests')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')


def function(source, name, replacement):
    match = re.search(r'static [^\n]+\b' + name + r'\([^\n]*\)\s*\{', source)
    assert match, name
    position = source.index('{', match.start())
    depth = 1
    end = position + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[:position] + '{' + replacement + '}' + source[end:]


navigation = ast.parse((root / 'tests/navigation.py').read_text(encoding='utf8'))
prefix = next(ast.literal_eval(node.value) for node in navigation.body if isinstance(node, ast.Assign)
              and any(isinstance(target, ast.Name) and target.id == 'prefix' for target in node.targets))
prefix = prefix.replace('static bool isN3DS = true;', '''static bool isN3DS = true;
static bool captureRunning,captureOffline,captureSleeping,themeEnabled;
static u32 cachedIp,cacheReads,snapshotReads;
typedef struct { u32 x,y,w,h,color; char text[256]; } Command;
static Command commands[128];
static unsigned commandCount,checks;
#define CHECK(value) do { if(!(value)){fprintf(stderr,"check failed line%d\\n",__LINE__);abort();}++checks; } while(0)
static int captureFooterFormat(char *out,size_t capacity,const char *format,...)
{
 CHECK(capacity==32);
 va_list values;va_start(values,format);
 int count=vsnprintf(out,capacity,format,values);va_end(values);
 CHECK(count>=0&&(size_t)count<capacity);
 return count;
}
''')
prefix = function(prefix, 'RemotePlay_IsRunning', ' return captureRunning; ')
prefix = function(prefix, 'RemotePlay_GetStatus', ' ++snapshotReads;out->noNetwork=captureOffline; ')
prefix = function(prefix, 'Sleep__IsSleeping', ' return captureSleeping; ')
prefix = function(prefix, 'miniSocGetCachedHostId', ' ++cacheReads;return cachedIp; ')
prefix = function(prefix, 'socGethostid', ' abort(); ')
prefix = function(prefix, 'gethostid', ' abort(); ')
prefix = function(prefix, 'Theme_GetBackground', ' return themeEnabled?themeTestPixels:NULL; ')
prefix = function(prefix, 'Draw_DrawString', '''
 CHECK(commandCount<128);Command *command=&commands[commandCount++];
 command->x=x;command->y=y;command->w=Text_MeasureWidth(text);command->h=10;command->color=color;
 CHECK(strlen(text)<sizeof(command->text));strcpy(command->text,text);
''')
prefix = function(prefix, 'Draw_DrawFormattedString', '''
 char text[256];va_list values;va_start(values,fmt);vsnprintf(text,sizeof(text),fmt,values);va_end(values);
 Draw_DrawString(x,y,color,text);
''')
prefix = function(prefix, 'Draw_FillRect', '''
 CHECK(commandCount<128);Command *command=&commands[commandCount++];
 command->x=x;command->y=y;command->w=w;command->h=h;command->color=color;command->text[0]=0;
 if(x==219&&y==10)CHECK(w==101&&h==10&&activeBackground==(themeEnabled?themeTestPixels:NULL));
''')
prefix += r'''
static const char *lookupTranslation(const char *text)
{
 return strcmp(text,"Rosalina menu")==0 ? "Rosalina繝｡繝九Η繝ｼ" : text;
}
static bool japanese;
'''
prefix = function(prefix, 'Lang_Translate', ' return japanese?lookupTranslation(text):text; ')
prefix = prefix.replace('static const char *Lang_Translate', 'static const char *lookupTranslation(const char *text);\nstatic bool japanese;\nstatic const char *Lang_Translate')
prefix = '#include <stdarg.h>\n' + prefix
menu = (base / 'source/menu.c').read_text(encoding='utf8')
code = menu[menu.index('typedef struct {\n    Menu *menu;'):menu.index('\nvoid menuShow(Menu *root)')]
assert code.count('int n = sprintf(text, ') == 1
code = code.replace('int n = sprintf(text, ', 'int n = captureFooterFormat(text, sizeof(text), ')
release = re.search(r'^#define FORK_RELEASE_VERSION\s+("[^"]+")', (base / 'include/menu.h').read_text(), re.MULTILINE).group(1)
assert 'socGethostid(' not in code and 'ACU_' not in code and 'miniSocRefreshHostId(' not in code

main = r'''
static bool has(const char *text)
{
 for(unsigned i=0;i<commandCount;i++)if(strcmp(commands[i].text,text)==0)return true;
 return false;
}
static bool captureAreaCleared(void)
{
 for(unsigned i=0;i<commandCount;i++)if(!commands[i].text[0]&&commands[i].x==219&&commands[i].y==10&&commands[i].w==101&&commands[i].h==10)return true;
 return false;
}
static void emitString(const char *text)
{
 putchar('"');for(;*text;text++){if((unsigned char)*text==0xF8){fputs("\\u00f8",stdout);continue;}if(*text=='"'||*text=='\\')putchar('\\');putchar(*text);}putchar('"');
}
static void frame(const char *name)
{
 printf("{\"name\":");emitString(name);printf(",\"commands\":[");
 for(unsigned i=0;i<commandCount;i++){
  Command *p=commands+i;if(i)putchar(',');
  printf("{\"x\":%u,\"y\":%u,\"width\":%u,\"height\":%u,\"color\":%u,\"text\":",p->x,p->y,p->w,p->h,p->color);emitString(p->text);putchar('}');
 }
 printf("]}\n");
}
static void draw(Menu *menu,MenuDrawState *state,bool refresh)
{
 memset(commands,0,sizeof(commands));commandCount=0;menuDraw(menu,0,refresh,state);
}
static void geometry(void)
{
 for(unsigned i=0;i<commandCount;i++){
  Command *p=commands+i;
  if(p->y==10&&p->text[0])CHECK(p->x+p->w<=310);
  if(p->y==10&&p->x>=220&&p->text[0])CHECK(captureAreaCleared()&&p->x-1>=219);
  if(p->y==220&&p->text[0])CHECK(p->x+p->w<=310);
  if(!p->text[0]||(p->y!=10&&p->y!=220))continue;
  for(unsigned j=0;j<i;j++){
   Command *q=commands+j;
   if(q->text[0]&&q->y==p->y)CHECK(p->x>=q->x+q->w||q->x>=p->x+p->w);
  }
 }
}
int main(void)
{
 rosalinaMenu.title="Rosalina menu";rosalinaMenu.nbItems=16;
 for(unsigned i=0;i<16;i++)rosalinaMenu.items[i]=(MenuItem){.title="Menu item",.action_type=METHOD};
 Menu submenu=rosalinaMenu;submenu.title="Other menu";
 MenuDrawState state={0};miniSocEnabled=true;cachedIp=0x6401A8C0;
 g_textColor=0xFFFF;g_accentColor=0xF81F;g_cursorColor=0xFFFF;
 draw(&rosalinaMenu,&state,false);CHECK(!has("192.168.1.100")&&cacheReads==0&&snapshotReads==0);
 captureRunning=true;draw(&rosalinaMenu,&state,false);
 CHECK(has("192.168.1.100")&&cacheReads==1&&snapshotReads==1);geometry();frame("waiting-viewer-menu-open");
 for(unsigned i=0;i<100;i++)draw(&rosalinaMenu,&state,false);
 CHECK(cacheReads==1&&snapshotReads==1&&commandCount==0);
 captureOffline=true;testNow=1000;draw(&rosalinaMenu,&state,true);CHECK(has("IP: --")&&!has("192.168.1.100"));frame("offline-stale-cache-hidden");
 captureOffline=false;captureSleeping=true;testNow=2000;draw(&rosalinaMenu,&state,true);CHECK(has("IP: --"));
 captureSleeping=false;cachedIp=0x04030201;testNow=3000;draw(&rosalinaMenu,&state,true);CHECK(has("1.2.3.4"));
 cachedIp=0;testNow=4000;draw(&rosalinaMenu,&state,true);CHECK(has("IP: --")&&!has("0.0.0.0"));
 cachedIp=UINT32_MAX;testNow=5000;draw(&rosalinaMenu,&state,true);CHECK(has("IP: --")&&!has("255.255.255.255"));
 captureRunning=false;unsigned reads=cacheReads;draw(&rosalinaMenu,&state,false);CHECK(!has("IP: --")&&cacheReads==reads&&captureAreaCleared());frame("stopped-cleared");
 isN3DS=false;captureRunning=true;state.valid=false;draw(&rosalinaMenu,&state,false);CHECK(has("IP: --")&&cacheReads==reads+1);geometry();frame("old3ds-visible");reads=cacheReads;
 isN3DS=true;cachedIp=0xFEFFFFFF;state.valid=false;draw(&submenu,&state,false);CHECK(!has("255.255.255.254")&&cacheReads==reads);
 for(unsigned language=0;language<2;language++)for(unsigned safe=0;safe<2;safe++)for(unsigned theme=0;theme<2;theme++){
  japanese=language;g_safeMode=safe;themeEnabled=theme;state.valid=false;
  menuIsRelease=false;menuVersionLoaded=true;batteryTemperature=255;batteryVoltage=5.25f;batteryPercentage=100;
  testNow+=1000;draw(&rosalinaMenu,&state,true);CHECK(has("255.255.255.254"));geometry();
  CHECK(has("Luma3DS ver.y18-0000002a"));
  char name[64];snprintf(name,sizeof(name),"language%u-safe%u-theme%u",language,safe,theme);frame(name);
 }
 char address[18];memset(address,0x7A,sizeof(address));
 CHECK(MenuCaptureIp_Format(address+1,true,true,true,false,false,0xFEFFFFFF)==MENU_CAPTURE_IP_READY);
 CHECK(address[0]==0x7A&&address[17]==0x7A&&strcmp(address+1,"255.255.255.254")==0);
 CHECK(MenuCaptureIp_Format(address,true,true,true,true,false,1)==MENU_CAPTURE_IP_OFFLINE);
 CHECK(MenuCaptureIp_Format(address,true,true,true,true,true,1)==MENU_CAPTURE_IP_SLEEPING);
 CHECK(MenuCaptureIp_Format(address,true,true,true,false,false,0)==MENU_CAPTURE_IP_WAITING);
 CHECK(MenuCaptureIp_Format(address,false,true,true,false,false,1)==MENU_CAPTURE_IP_HIDDEN&&address[0]==0);
 CHECK(MenuCaptureIp_Format(NULL,true,true,true,false,false,1)==MENU_CAPTURE_IP_HIDDEN);
 printf("{\"passed\":true,\"checks\":%u,\"cache_reads\":%u,\"status_snapshots\":%u,\"soc_ac_calls\":0}\n",checks,cacheReads,snapshotReads);
 return 0;
}
'''
target = args.output / 'capture_ip.c'
target.write_text('#define NEWER 1\n#define FORK_DISPLAY_VERSION "v13.4Y"\n#define FORK_RELEASE_VERSION ' + release + '\n' + prefix + code + main, encoding='utf8')
binary = target.with_suffix('.exe' if os.name == 'nt' else '')
command = [compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-variable',
           '-I' + str(base / 'include'), str(target), str(base / 'source/menu_capture_ip.c'), str(base / 'source/text.c'), '-o', str(binary)]
subprocess.run(command, check=True, env=environment)
run = subprocess.run([str(binary)], check=True, capture_output=True, text=True, encoding='utf8', env=environment)
values = [json.loads(line) for line in run.stdout.splitlines()]
result = values[-1]
assert result['passed'] and result['soc_ac_calls'] == 0
result['frames'] = values[:-1]
result['source_sha256'] = {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
                          for path in [base / 'source/menu.c', base / 'source/menu_capture_ip.c', base / 'include/menu_capture_ip.h', base / 'include/menu.h', Path(__file__)]}
result['hardware_tested'] = False
result['footer_fixture_boundary'] = 'Only the unchanged battery-footer sprintf is bounded to its actual 32-byte capacity in the host fixture; every call asserts no truncation. Production source, input conversions, format text and geometry checks are preserved.'
result['scope'] = 'Actual production menuDraw and capture formatter execute; existing status, cached IP, sleep, draw, MCU and SDK boundaries are fixtures. No SOC/AC service from rendering.'
(args.output / 'report.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
print(json.dumps({key: result[key] for key in ('passed', 'checks', 'cache_reads', 'status_snapshots', 'soc_ac_calls')}))
