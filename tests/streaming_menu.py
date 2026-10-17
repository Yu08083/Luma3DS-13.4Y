import ast
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'build/streaming-menu-tests'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
menu_path = module / 'source/menus/streaming.c'
header_path = module / 'include/remote_play.h'
if not header_path.exists():
    header_path = root.parent / 'streaming13/reference/sysmodules/rosalina/include/remote_play.h'
menu = '\n'.join(line for line in menu_path.read_text().splitlines() if not line.startswith('#include'))
backend_header = '\n'.join(line for line in header_path.read_text().splitlines()
                           if not line.startswith(('#include', '#pragma')))
lang_path = module / 'source/menus/lang.c'
lang = lang_path.read_text(encoding='utf-8')
labels = lang[lang.index('typedef struct { const char *english;'):lang.index('\nconst char *Lang(u32 id)')]
table = labels[:labels.index('\nconst char *Lang_Translate')]
pairs = [tuple(ast.literal_eval(value) for value in match) for match in re.findall(
    r'\{\s*("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")\s*\}', table)]
assert [english for english, _ in pairs] == sorted({english for english, _ in pairs})
translations = dict(pairs)
for label in ['Screen capture (wireless)', 'Close Rosalina to stream video', 'State: %s', 'OFF', 'Streaming', 'Waiting for viewer',
              'Paused (offline)', '3DS IP: %s', 'Viewer IP: %s', 'FPS: %lu   Quality: %lu',
              'Waiting for Wi-Fi.', 'Receiver: NTR mode', 'Starting...', 'Stopping...',
              'A: start     B: back', 'A: stop      B: back', 'Operation failed (0x%08lx).']:
    assert label in translations
glyph_header = (module / 'include/japanese_font.h').read_text()
glyphs = {int(value, 16) for value in re.findall(r'0x([0-9A-F]{4})', glyph_header.split('japaneseGlyphs')[0])}
assert {ord(character) for text in translations.values() for character in text if ord(character) >= 128} <= glyphs

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "text.h"
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t Result;
typedef enum { LANG_EN, LANG_JP } LangCode;
static LangCode g_language;
static bool g_langLoaded=true;
static void Lang_Load(void) { assert(false); }
static u32 g_textColor=0x5311,g_accentColor=0x7212,g_cursorColor=0x6313;
static bool menuShouldExit;
#define KEY_A 1u
#define KEY_B 2u
#define R_FAILED(value) ((value)<0)
typedef struct Line { u32 x,y,color; char text[512]; } Line;
typedef struct Frame { unsigned count; Line lines[16]; } Frame;
static Frame frames[64];
static unsigned frame_count,lock_depth,start_count,stop_count,save_count,ip_count,wait_count,key_count,checks;
static u32 keys[32],console_ip;
static bool running;
static Result start_result,stop_result;
static void (*wait_hook)(unsigned);
#define CHECK(value) do { assert(value); ++checks; } while(0)
static void Draw_Lock(void) { CHECK(lock_depth==0);lock_depth=1; }
static void Draw_Unlock(void) { CHECK(lock_depth==1);lock_depth=0; }
static void Draw_ClearFramebuffer(void) { CHECK(lock_depth==1);CHECK(frame_count<64);memset(&frames[frame_count],0,sizeof(Frame)); }
static void Draw_FlushFramebuffer(void) { CHECK(lock_depth==1);++frame_count; }
static u32 Draw_DrawString(u32 x,u32 y,u32 color,const char *text)
{
    CHECK(lock_depth==1);CHECK(frame_count<64);
    Frame *frame=&frames[frame_count];CHECK(frame->count<16);
    CHECK(x>=10&&x+Text_MeasureWidth(text)<=310&&y>=10&&y+10<=240);
    CHECK(!strchr(text,'\n'));
    for(unsigned i=0;i<frame->count;++i)CHECK(y>=frame->lines[i].y+10||y+10<=frame->lines[i].y);
    Line *line=&frame->lines[frame->count++];line->x=x;line->y=y;line->color=color;
    CHECK(strlen(text)<sizeof(line->text));strcpy(line->text,text);return y;
}
static u32 Draw_DrawFormattedString(u32 x,u32 y,u32 color,const char *format,...)
{
    char text[512];va_list args;va_start(args,format);int size=vsnprintf(text,sizeof(text),format,args);va_end(args);
    CHECK(size>=0&&(unsigned)size<sizeof(text));return Draw_DrawString(x,y,color,text);
}
static long socGethostid(void) { CHECK(running);++ip_count;return (long)console_ip; }
static u32 waitInputWithTimeout(u32 timeout)
{
    CHECK(lock_depth==0&&timeout==500&&wait_count<key_count);
    unsigned index=wait_count++;if(wait_hook)wait_hook(index);return keys[index];
}
'''

mocks = r'''
static RemotePlayStatus status;
bool RemotePlay_IsRunning(void) { CHECK(lock_depth==0);return running; }
void RemotePlay_GetStatus(RemotePlayStatus *out) { CHECK(lock_depth==0);*out=status; }
Result RemotePlay_Start(void)
{
    CHECK(lock_depth==0);++start_count;
    if(!R_FAILED(start_result)){running=true;status.state=REMOTEPLAY_WAITING;}
    return start_result;
}
Result RemotePlay_Stop(void)
{
    CHECK(lock_depth==0);++stop_count;
    if(!R_FAILED(stop_result)){running=false;status.state=REMOTEPLAY_OFF;}
    return stop_result;
}
void RemotePlay_SaveViewer(void) { CHECK(lock_depth==0&&running);++save_count; }
'''

tests = r'''
static void reset(bool active)
{
    CHECK(lock_depth==0);memset(frames,0,sizeof(frames));memset(&status,0,sizeof(status));
    frame_count=start_count=stop_count=save_count=ip_count=wait_count=0;
    key_count=1;keys[0]=KEY_B;running=active;menuShouldExit=false;start_result=stop_result=0;
    console_ip=0x0302010A;status.viewerIp=0x08070605;status.state=active?REMOTEPLAY_STREAMING:REMOTEPLAY_OFF;
    status.fps=30;status.quality=80;g_language=LANG_EN;wait_hook=NULL;
    g_textColor=0x5311;g_accentColor=0x7212;g_cursorColor=0x6313;
}
static bool contains(unsigned frame,const char *text)
{
    for(unsigned i=0;i<frames[frame].count;++i)if(strstr(frames[frame].lines[i].text,text))return true;
    return false;
}
static void colors(unsigned frame)
{
    CHECK(frames[frame].lines[0].color==g_accentColor);
    for(unsigned i=1;i<frames[frame].count;++i)
        CHECK(frames[frame].lines[i].color==(frames[frame].lines[i].y==210?g_cursorColor:g_textColor));
}
static void change_language(unsigned index) { if(!index)g_language=LANG_JP; }
static void change_color(unsigned index) { if(!index)g_textColor=0x3313; }
static void unseen_stat(unsigned index) { (void)index;++status.framesTop; }
static void terminate(unsigned index) { (void)index;menuShouldExit=true; }
static void change_running(unsigned index) { if(!index)running=false; }
static void save_frame(const char *name,unsigned frame)
{
    FILE *file=fopen(name,"wb");CHECK(file!=NULL);
    for(unsigned i=0;i<frames[frame].count;++i)
    {
        Line *line=&frames[frame].lines[i];
        CHECK(fprintf(file,"%u\t%u\t%u\t%s\n",line->x,line->y,line->color,line->text)>0);
    }
    CHECK(!fclose(file));
}
int main(void)
{
    char buffer[18];memset(buffer,0xA7,sizeof(buffer));formatIp(buffer+1,UINT32_MAX);
    CHECK(!strcmp(buffer+1,"255.255.255.255"));CHECK((unsigned char)buffer[0]==0xA7&&(unsigned char)buffer[17]==0xA7);
    formatIp(buffer+1,0x0302010A);CHECK(!strcmp(buffer+1,"10.1.2.3"));
    for(unsigned language=0;language<2;++language)
    {
        reset(false);g_language=(LangCode)language;N3DSMenu_Streaming();
        CHECK(frame_count==1&&start_count==0&&stop_count==0&&save_count==0&&ip_count==0);
        CHECK(contains(0,language?"画面キャプチャ (無線)":"Screen capture (wireless)"));
        CHECK(contains(0,language?"A: 開始":"A: start"));colors(0);
        reset(true);g_language=(LangCode)language;status.fps=status.quality=UINT32_MAX;N3DSMenu_Streaming();
        CHECK(frame_count==1&&save_count==1&&ip_count==1);
        CHECK(contains(0,language?"Rosalinaを閉じる":"Close Rosalina"));
        CHECK(contains(0,"10.1.2.3")&&contains(0,"5.6.7.8")&&contains(0,"4294967295"));colors(0);
        reset(true);g_language=(LangCode)language;status.noNetwork=true;N3DSMenu_Streaming();
        CHECK(ip_count==0&&save_count==1&&contains(0,language?"一時停止":"Paused"));
        CHECK(contains(0,language?"Wi-Fiの接続待ち":"Waiting for Wi-Fi"));
        CHECK(!contains(0,"10.1.2.3")&&!contains(0,"5.6.7.8"));
        CHECK(!contains(0,language?"Rosalinaを閉じる":"Close Rosalina"));
        reset(false);g_language=(LangCode)language;key_count=2;keys[0]=KEY_A;keys[1]=KEY_B;N3DSMenu_Streaming();
        CHECK(start_count==1&&stop_count==0&&save_count==1&&frame_count==3);
        CHECK(contains(1,language?"開始中":"Starting"));CHECK(contains(2,language?"接続待ち":"Waiting for viewer"));
        reset(false);g_language=(LangCode)language;start_result=(Result)0xD8E007F7;key_count=2;keys[0]=KEY_A;keys[1]=KEY_B;N3DSMenu_Streaming();
        CHECK(start_count==1&&!running&&save_count==0&&contains(2,"0xd8e007f7"));
        reset(true);g_language=(LangCode)language;key_count=2;keys[0]=KEY_A;keys[1]=KEY_B;N3DSMenu_Streaming();
        CHECK(stop_count==1&&!running&&save_count==0&&frame_count==3);
        CHECK(contains(1,language?"停止処理中":"Stopping"));
        reset(true);g_language=(LangCode)language;stop_result=(Result)0xC920181A;key_count=2;keys[0]=KEY_A;keys[1]=KEY_B;N3DSMenu_Streaming();
        CHECK(stop_count==1&&running&&save_count==1&&contains(2,"0xc920181a"));
    }
    reset(false);keys[0]=KEY_A|KEY_B;N3DSMenu_Streaming();CHECK(start_count==0&&stop_count==0);
    reset(true);key_count=3;keys[0]=keys[1]=0;keys[2]=KEY_B;N3DSMenu_Streaming();CHECK(frame_count==1&&save_count==1&&ip_count==3);
    reset(true);key_count=3;keys[0]=keys[1]=0;keys[2]=KEY_B;wait_hook=unseen_stat;N3DSMenu_Streaming();CHECK(frame_count==1&&save_count==1);
    reset(true);key_count=2;keys[0]=0;keys[1]=KEY_B;wait_hook=change_language;N3DSMenu_Streaming();CHECK(frame_count==2&&contains(1,"画面キャプチャ (無線)"));
    reset(true);key_count=2;keys[0]=0;keys[1]=KEY_B;wait_hook=change_color;N3DSMenu_Streaming();CHECK(frame_count==2);colors(1);
    reset(true);console_ip=UINT32_MAX;N3DSMenu_Streaming();CHECK(!contains(0,"3DS IP"));
    reset(true);console_ip=0;N3DSMenu_Streaming();CHECK(!contains(0,"3DS IP"));
    reset(true);status.state=(RemotePlayState)255;N3DSMenu_Streaming();CHECK(contains(0,"Waiting for viewer")&&!contains(0,"FPS:"));
    reset(true);wait_hook=terminate;keys[0]=0;N3DSMenu_Streaming();CHECK(wait_count==1&&save_count==1);
    reset(true);menuShouldExit=true;N3DSMenu_Streaming();CHECK(frame_count==0&&wait_count==0&&save_count==1);
    reset(true);key_count=2;keys[0]=KEY_A;keys[1]=KEY_B;wait_hook=change_running;N3DSMenu_Streaming();CHECK(start_count==1&&stop_count==0);
    for(unsigned language=0;language<2;++language)
    {
        reset(true);g_language=(LangCode)language;g_textColor=g_accentColor=g_cursorColor=0xC4FF;
        N3DSMenu_Streaming();save_frame(language?"japanese-streaming.tsv":"english-streaming.tsv",0);
        reset(true);g_language=(LangCode)language;g_textColor=g_accentColor=g_cursorColor=0xC4FF;status.noNetwork=true;
        N3DSMenu_Streaming();save_frame(language?"japanese-offline.tsv":"english-offline.tsv",0);
    }
    CHECK(lock_depth==0);
    printf("%u streaming menu assertions passed\n",checks);
    return 0;
}
'''

harness = prefix + backend_header + mocks + labels + menu + tests
(generated / 'test.c').write_bytes(harness.encode('utf-8'))
target = generated / ('test.exe' if os.name == 'nt' else 'test')
command = [compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-I' + str(module / 'include'),
           str(generated / 'test.c'), str(module / 'source/text.c'), '-o', str(target)]
subprocess.run(command, check=True, env=env)
result = subprocess.run([str(target)], cwd=generated, check=True, env=env, capture_output=True, text=True)
print(result.stdout.strip())
snapshots = []
try:
    from PIL import Image
    ascii_font = bytes(int(value, 16) for value in re.findall(r'0x([0-9A-Fa-f]{2})', (module / 'include/font.h').read_text()))
    codepoints = [int(value, 16) for value in re.findall(r'0x([0-9A-F]{4})', glyph_header.split('japaneseGlyphs')[0])]
    glyph_bytes = bytes(int(value, 16) for value in re.findall(r'0x([0-9A-F]{2})', glyph_header.split('japaneseGlyphs')[1]))
    japanese_glyphs = {codepoint: glyph_bytes[index * 8:(index + 1) * 8] for index, codepoint in enumerate(codepoints)}
    for path in generated.glob('*.tsv'):
        image = Image.new('RGB', (320, 240), (0, 0, 0))
        for line in path.read_text(encoding='utf-8').splitlines():
            x, y, value, text = line.split('\t', 3)
            x, y, value = int(x), int(y), int(value)
            color = (((value >> 11) & 31) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31)
            for character in text:
                codepoint = ord(character)
                width = 6 if codepoint < 128 else 8
                for row in range(10):
                    bits = ascii_font[codepoint * 10 + row] if codepoint < 128 else \
                           japanese_glyphs[codepoint][row - 1] if 1 <= row <= 8 else 0
                    for column in range(width):
                        mask = 1 << (6 - column) if codepoint < 128 else 128 >> column
                        if bits & mask:
                            image.putpixel((x + column - 1, y + row), color)
                x += width
        image.save(path.with_suffix('.png'))
        snapshots.append(path.with_suffix('.png').name)
except ImportError:
    pass
report = {'all_passed': True, 'assertions': int(result.stdout.split()[0]), 'languages': ['JP', 'EN'],
          'frame_bounds': [320, 240], 'backend_calls_outside_draw_lock': True,
          'viewer_save_on_exit_only': True, 'unchanged_status_skips_draw': True,
          'snapshots': snapshots,
          'source_sha256': {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in (menu_path, lang_path, header_path) if path.is_relative_to(root)},
          'hardware_tested': False}
(generated / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
