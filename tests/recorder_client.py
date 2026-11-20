import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'build/recorder-client-tests'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
source = module / 'source/menus/recorder.c'
protocol = module / 'include/recorder_protocol.h'
language = module / 'source/menus/lang.c'
body = '\n'.join(line for line in source.read_text().splitlines() if not line.startswith('#include'))
lang = language.read_text(encoding='utf-8')
labels = lang[lang.index('typedef struct { const char *english;'):lang.index('\nconst char *Lang(u32 id)')]
label_table = labels[:labels.index('\nconst char *Lang_Translate')]
translations = dict((json.loads(english), json.loads(japanese)) for english, japanese in re.findall(
    r'\{\s*("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")\s*\}', label_table))
for label in ['SD recording', 'Starting...', 'Ready', 'Recording', 'Saving...', 'Error', 'OFF', 'Replay',
              'Time: %lu s  Files: %lu', 'MP4 / settings: Yu_ Recorder', 'Enable external modules at boot.',
              'Stop Yu_ Capture first.', 'Operation failed (0x%08lx).', 'A: record', 'A: stop', 'Y: shut down  B: back']:
    assert label in translations, label

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "text.h"
#include "recorder_protocol.h"
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t s64;
typedef int32_t Result;
typedef uint32_t Handle;
typedef struct { u64 programId; u32 mediaType; } FS_ProgramInfo;
typedef enum { LANG_EN, LANG_JP } LangCode;
static LangCode g_language;
static bool g_langLoaded=true;
static void Lang_Load(void) { assert(false); }
static u32 g_textColor=0x5311,g_accentColor=0x7212,g_cursorColor=0x6313;
static bool menuShouldExit;
#define KEY_A 1u
#define KEY_B 2u
#define KEY_Y 4u
#define MEDIATYPE_NAND 0u
#define R_FAILED(value) ((value)<0)
#define R_SUCCEEDED(value) ((value)>=0)
#define IPC_MakeHeader(command,normal,translated) (((command)<<16)|((normal)<<6)|(translated))
static unsigned checks,case_count;
#define CHECK(value) do { assert(value); ++checks; } while(0)
typedef struct { u32 x,y,color; char text[256]; } Line;
typedef struct { unsigned count; Line lines[12]; } Frame;
static Frame frames[128];
static unsigned frame_count,lock_depth,key_count,key_index;
static u32 keys[128];
static bool port_open,streaming,live_handles[1024],override_header;
static unsigned handle_cursor,connect_count,close_count,send_count,launch_count,sleep_count,system_info_count;
static unsigned command_count,commands[128],status_count,open_after_sleep;
static Result connect_result,send_result,status_result,command_result,system_result,launch_result;
static u32 reply_header,reply_magic,reply_version,rec_state,last_error,seconds,files,launch_state;
static s64 system_options;
static u32 ipc[64];
static void (*wait_hook)(unsigned);
static void Draw_Lock(void) { CHECK(!lock_depth);lock_depth=1; }
static void Draw_Unlock(void) { CHECK(lock_depth==1);lock_depth=0; }
static void Draw_ClearFramebuffer(void) { CHECK(lock_depth==1&&frame_count<128);memset(&frames[frame_count],0,sizeof(Frame)); }
static void Draw_FlushFramebuffer(void) { CHECK(lock_depth==1);++frame_count; }
static u32 Draw_DrawString(u32 x,u32 y,u32 color,const char *text)
{
    CHECK(lock_depth==1&&frame_count<128);Frame *frame=&frames[frame_count];
    CHECK(frame->count<12&&x>=10&&x+Text_MeasureWidth(text)<=310&&y+10<=240);
    CHECK(!strchr(text,'\n'));
    CHECK(color==(y==10?g_accentColor:(y==188||y==210?g_cursorColor:g_textColor)));
    for(unsigned i=0;i<frame->count;++i)CHECK(y>=frame->lines[i].y+10||y+10<=frame->lines[i].y);
    Line *line=&frame->lines[frame->count++];line->x=x;line->y=y;line->color=color;
    CHECK(strlen(text)<sizeof(line->text));strcpy(line->text,text);return y;
}
static u32 Draw_DrawFormattedString(u32 x,u32 y,u32 color,const char *format,...)
{
    char text[256];va_list args;va_start(args,format);int n=vsnprintf(text,sizeof(text),format,args);va_end(args);
    CHECK(n>=0&&(unsigned)n<sizeof(text));return Draw_DrawString(x,y,color,text);
}
static u32 waitInputWithTimeout(u32 timeout)
{
    CHECK(!lock_depth&&timeout==500&&key_index<key_count);
    unsigned index=key_index++;if(wait_hook)wait_hook(index);return keys[index];
}
static Result svcConnectToPort(Handle *out,const char *name)
{
    CHECK(!lock_depth&&!strcmp(name,YU_REC_PORT));++connect_count;
    if(R_FAILED(connect_result))return connect_result;
    if(!port_open)return -100;
    CHECK(handle_cursor+1<1024);*out=++handle_cursor;live_handles[*out]=true;return 0;
}
static Result svcCloseHandle(Handle handle)
{
    CHECK(handle>0&&handle<1024&&live_handles[handle]);live_handles[handle]=false;++close_count;return 0;
}
static u32 *getThreadCommandBuffer(void) { CHECK(!lock_depth);return ipc; }
static Result svcSendSyncRequest(Handle handle)
{
    CHECK(!lock_depth&&handle>0&&handle<1024&&live_handles[handle]);++send_count;
    if(R_FAILED(send_result))return send_result;
    u32 request=ipc[0];
    if(request==YU_REC_STATUS_REQUEST)
    {
        ++status_count;memset(ipc,0,sizeof(ipc));ipc[0]=override_header?reply_header:YU_REC_STATUS_REPLY;
        ipc[1]=(u32)status_result;ipc[2]=reply_magic;ipc[3]=reply_version;ipc[4]=rec_state;
        ipc[5]=seconds;ipc[6]=128;ipc[7]=17;ipc[8]=19;ipc[9]=3;ipc[10]=files;ipc[11]=last_error;ipc[12]=1234;
    }
    else
    {
        CHECK(request==YU_REC_COMMAND_REQUEST&&command_count<128);
        u32 action=ipc[1];commands[command_count++]=action;ipc[0]=override_header?reply_header:YU_REC_COMMAND_REPLY;
        ipc[1]=(u32)command_result;
        if(R_SUCCEEDED(command_result))
        {
            if(action==YU_REC_CMD_START)rec_state=YU_REC_RECORDING;
            else if(action==YU_REC_CMD_STOP)rec_state=YU_REC_IDLE;
            else if(action==YU_REC_CMD_EXIT){rec_state=YU_REC_EXITED;port_open=false;}
            else CHECK(false);
        }
    }
    return 0;
}
static Result svcGetSystemInfo(s64 *out,u32 type,u32 parameter)
{
    CHECK(!lock_depth&&type==0x10000&&parameter==3);++system_info_count;*out=system_options;return system_result;
}
static Result PMAPP_LaunchTitle(const FS_ProgramInfo *program,u32 flags)
{
    CHECK(!lock_depth&&program->programId==YU_REC_TITLE_ID&&program->mediaType==MEDIATYPE_NAND&&!flags);
    ++launch_count;
    if(R_SUCCEEDED(launch_result)&&!open_after_sleep){port_open=true;rec_state=launch_state;}
    return launch_result;
}
static Result svcSleepThread(s64 duration)
{
    CHECK(!lock_depth&&duration==50000000LL);++sleep_count;
    if(open_after_sleep&&sleep_count==open_after_sleep){port_open=true;rec_state=launch_state;}
    return 0;
}
static bool RemotePlay_IsRunning(void) { CHECK(!lock_depth);return streaming; }
'''

tests = r'''
static void reset(bool active)
{
    for(unsigned i=1;i<=handle_cursor;++i)CHECK(!live_handles[i]);
    CHECK(!lock_depth);memset(frames,0,sizeof(frames));memset(keys,0,sizeof(keys));memset(ipc,0,sizeof(ipc));
    frame_count=key_index=0;key_count=1;keys[0]=KEY_B;port_open=active;streaming=false;menuShouldExit=false;
    handle_cursor=connect_count=close_count=send_count=launch_count=sleep_count=system_info_count=0;
    command_count=status_count=open_after_sleep=0;connect_result=send_result=status_result=command_result=system_result=launch_result=0;
    reply_header=0;override_header=false;reply_magic=YU_REC_MAGIC;reply_version=YU_REC_PROTOCOL_VERSION;rec_state=launch_state=YU_REC_IDLE;last_error=0;seconds=42;files=7;
    system_options=2;g_language=LANG_EN;wait_hook=NULL;++case_count;
}
static bool contains(unsigned frame,const char *text)
{
    CHECK(frame<frame_count);for(unsigned i=0;i<frames[frame].count;++i)if(strstr(frames[frame].lines[i].text,text))return true;return false;
}
static void input(u32 first) { key_count=2;keys[0]=first;keys[1]=KEY_B; }
static void no_live_handles(void) { for(unsigned i=1;i<=handle_cursor;++i)CHECK(!live_handles[i]);CHECK(close_count==handle_cursor); }
static void save_frame(const char *name,unsigned frame)
{
    FILE *file=fopen(name,"wb");CHECK(file!=NULL);
    for(unsigned i=0;i<frames[frame].count;++i){Line *line=&frames[frame].lines[i];CHECK(fprintf(file,"%u\t%u\t%u\t%s\n",line->x,line->y,line->color,line->text)>0);}
    CHECK(!fclose(file));
}
int main(void)
{
    u32 status[12];
    reset(false);memset(status,0xA5,sizeof(status));CHECK(R_FAILED(recorderRequest(1,0,status))&&send_count==0&&close_count==0);no_live_handles();
    reset(true);CHECK(!recorderRequest(1,0,status));CHECK(status[1]==YU_REC_MAGIC&&status[2]==YU_REC_PROTOCOL_VERSION&&status[3]==YU_REC_IDLE&&status[4]==42&&status[9]==7&&status[11]==1234);no_live_handles();
    reset(true);send_result=-101;CHECK(recorderRequest(1,0,status)==-101);no_live_handles();
    reset(true);status_result=-102;CHECK(recorderRequest(1,0,status)==-102);no_live_handles();
    const u32 wrong_headers[]={0,YU_REC_STATUS_REPLY+1,YU_REC_STATUS_REQUEST,YU_REC_COMMAND_REPLY};
    for(unsigned i=0;i<sizeof(wrong_headers)/sizeof(wrong_headers[0]);++i){reset(true);override_header=true;reply_header=wrong_headers[i];CHECK(R_FAILED(recorderRequest(1,0,status)));no_live_handles();}
    reset(true);reply_magic^=1;CHECK(R_FAILED(recorderRequest(1,0,status)));no_live_handles();
    reset(true);reply_version++;CHECK(R_FAILED(recorderRequest(1,0,status)));no_live_handles();
    reset(true);rec_state=YU_REC_REPLAY+1;memset(status,0xA5,sizeof(status));CHECK(recorderRequest(1,0,status)==(Result)YU_REC_ERROR_INVALID);for(unsigned i=0;i<12;++i)CHECK(status[i]==0xA5A5A5A5);no_live_handles();
    reset(true);u32 guarded[14];memset(guarded,0xA5,sizeof(guarded));CHECK(!recorderRequest(1,0,guarded+1));CHECK(guarded[0]==0xA5A5A5A5&&guarded[13]==0xA5A5A5A5);no_live_handles();
    reset(true);connect_result=-103;CHECK(recorderRequest(1,0,status)==-103&&!send_count&&!close_count);no_live_handles();
    reset(true);command_result=(Result)YU_REC_ERROR_BUSY;CHECK(recorderRequest(2,YU_REC_CMD_START,NULL)==(Result)YU_REC_ERROR_BUSY);CHECK(command_count==1&&rec_state==YU_REC_IDLE);no_live_handles();
    reset(true);override_header=true;reply_header=YU_REC_STATUS_REPLY;CHECK(R_FAILED(recorderRequest(2,YU_REC_CMD_START,NULL)));no_live_handles();
    reset(false);CHECK(!Recorder_IsRunning()&&launch_count==0);no_live_handles();
    reset(true);CHECK(Recorder_IsRunning()&&send_count==0&&launch_count==0);no_live_handles();
    for(unsigned language=0;language<2;++language)
    {
        reset(false);g_language=(LangCode)language;RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count&&frame_count==1);no_live_handles();
        CHECK(contains(0,language?"SD録画":"SD recording"));save_frame(language?"japanese-off.tsv":"english-off.tsv",0);
        reset(true);g_language=(LangCode)language;RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count&&frame_count==1);CHECK(contains(0,"42")&&contains(0,"7"));no_live_handles();
        reset(false);g_language=(LangCode)language;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&command_count==1&&commands[0]==YU_REC_CMD_START&&rec_state==YU_REC_RECORDING);no_live_handles();
        reset(true);g_language=(LangCode)language;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&command_count==1&&commands[0]==YU_REC_CMD_START&&rec_state==YU_REC_RECORDING);no_live_handles();
        CHECK(contains(frame_count-1,language?"録画中":"Recording"));save_frame(language?"japanese-recording.tsv":"english-recording.tsv",frame_count-1);
        reset(true);g_language=(LangCode)language;rec_state=YU_REC_RECORDING;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&command_count==1&&commands[0]==YU_REC_CMD_STOP&&rec_state==YU_REC_IDLE);no_live_handles();
        reset(true);g_language=(LangCode)language;input(KEY_Y);RosalinaMenu_Recorder();CHECK(command_count==1&&commands[0]==YU_REC_CMD_EXIT&&!port_open&&!launch_count);no_live_handles();
        reset(false);g_language=(LangCode)language;system_options=0;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count);CHECK(contains(frame_count-1,language?"外部モジュール":"external modules"));no_live_handles();
        reset(false);g_language=(LangCode)language;streaming=true;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count&&!system_info_count);CHECK(contains(frame_count-1,language?"Yu_ キャプチャ":"Yu_ Capture"));no_live_handles();
        reset(true);g_language=(LangCode)language;streaming=true;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count);no_live_handles();
        reset(true);g_language=(LangCode)language;streaming=true;rec_state=YU_REC_RECORDING;input(KEY_A);RosalinaMenu_Recorder();CHECK(command_count==1&&commands[0]==YU_REC_CMD_STOP);no_live_handles();
    }
    reset(false);system_result=-105;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count);no_live_handles();
    reset(false);launch_result=-106;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&!command_count&&sleep_count==0);no_live_handles();
    reset(false);open_after_sleep=1000;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&!command_count&&sleep_count==60);no_live_handles();
    reset(false);open_after_sleep=3;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&command_count==1&&commands[0]==YU_REC_CMD_START&&sleep_count==3);no_live_handles();
    reset(false);launch_state=YU_REC_REPLAY;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&command_count==1&&commands[0]==YU_REC_CMD_START&&rec_state==YU_REC_RECORDING&&!sleep_count);no_live_handles();
    reset(false);launch_state=YU_REC_RECORDING;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&!command_count&&rec_state==YU_REC_RECORDING&&!sleep_count);no_live_handles();
    reset(false);launch_state=YU_REC_ERROR;last_error=0xC920181A;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&!command_count&&!sleep_count&&contains(frame_count-1,"0xc920181a"));no_live_handles();
    reset(false);launch_state=YU_REC_ERROR;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&!command_count&&!sleep_count&&contains(frame_count-1,"0xd8a3fbf5"));no_live_handles();
    reset(false);launch_state=YU_REC_ERROR;last_error=1;input(KEY_A);RosalinaMenu_Recorder();CHECK(launch_count==1&&!command_count&&!sleep_count&&contains(frame_count-1,"0xd8a3fbf5"));no_live_handles();
    reset(true);reply_version++;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count);no_live_handles();
    reset(true);reply_magic^=1;input(KEY_A);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count);no_live_handles();
    reset(false);key_count=3;keys[0]=keys[1]=KEY_A;keys[2]=KEY_B;RosalinaMenu_Recorder();CHECK(launch_count==1&&command_count==2&&commands[0]==YU_REC_CMD_START&&commands[1]==YU_REC_CMD_STOP);no_live_handles();
    reset(true);input(KEY_A|KEY_B);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count);no_live_handles();
    reset(false);input(KEY_Y);RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count);no_live_handles();
    reset(false);menuShouldExit=true;RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count&&!send_count&&!frame_count);no_live_handles();
    reset(true);command_result=(Result)YU_REC_ERROR_STATE;input(KEY_A);RosalinaMenu_Recorder();CHECK(command_count==1&&rec_state==YU_REC_IDLE);CHECK(contains(frame_count-1,"0xd8a3fbf5"));no_live_handles();
    for(unsigned state=YU_REC_STARTING;state<=YU_REC_REPLAY;++state){reset(true);rec_state=state;seconds=files=UINT32_MAX;RosalinaMenu_Recorder();CHECK(!launch_count&&!command_count&&contains(0,"4294967295"));no_live_handles();}
    printf("%u recorder client assertions across %u cases passed\n",checks,case_count);
    return 0;
}
'''

harness = prefix + labels + body + tests
(generated / 'test.c').write_text(harness, encoding='utf-8')
target = generated / ('test.exe' if os.name == 'nt' else 'test')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-I' + str(module / 'include'),
                str(generated / 'test.c'), str(module / 'source/text.c'), '-o', str(target)], check=True, env=environment)
result = subprocess.run([str(target)], cwd=generated, capture_output=True, text=True, env=environment)
print(result.stdout.strip())
if result.returncode:
    print(result.stderr)
result.check_returncode()
snapshots = []
try:
    from PIL import Image
    glyph_header = (module / 'include/japanese_font.h').read_text()
    codepoints = [int(value, 16) for value in re.findall(r'0x([0-9A-F]{4})', glyph_header.split('japaneseGlyphs')[0])]
    glyph_bytes = bytes(int(value, 16) for value in re.findall(r'0x([0-9A-F]{2})', glyph_header.split('japaneseGlyphs')[1]))
    japanese_glyphs = {value: glyph_bytes[index * 8:(index + 1) * 8] for index, value in enumerate(codepoints)}
    ascii_font = bytes(int(value, 16) for value in re.findall(r'0x([0-9A-Fa-f]{2})', (module / 'include/font.h').read_text()))
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
                    bits = ascii_font[codepoint * 10 + row] if codepoint < 128 else japanese_glyphs[codepoint][row - 1] if 1 <= row <= 8 else 0
                    for column in range(width):
                        mask = 1 << (6 - column) if codepoint < 128 else 128 >> column
                        if bits & mask:
                            image.putpixel((x + column - 1, y + row), color)
                x += width
        image.save(path.with_suffix('.png'))
        snapshots.append(path.with_suffix('.png').name)
except ImportError:
    pass
report = {'all_passed': True, 'assertions': int(result.stdout.split()[0]), 'cases': int(result.stdout.split()[5]),
          'actual_client_source': True, 'actual_protocol_header': True, 'languages': ['JP', 'EN'],
          'physical_hardware_tested': False, 'handles_closed': True, 'no_automatic_startup': True,
          'streaming_exclusion': True, 'snapshots': snapshots,
          'source_sha256': {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in [source, protocol, language]}}
(generated / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
