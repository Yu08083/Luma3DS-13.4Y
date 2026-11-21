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
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "text.h"
#include "recorder_protocol.h"
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t s64;
typedef int32_t Result;
typedef uint32_t Handle;
typedef uint8_t u8;
typedef struct { Handle handle; void (*ep)(void); } MyThread;
#define CORE_SYSTEM 1
#define SYSCLOCK_ARM11 268111856ULL
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
static _Atomic unsigned checks;
static unsigned case_count;
#define CHECK(value) do { if(!(value)){fprintf(stderr,"line %d (case %u): %s\n",__LINE__,case_count,#value);exit(1);} ++checks; } while(0)
typedef struct { u32 x,y,color; char text[256]; } Line;
typedef struct { unsigned count; Line lines[12]; } Frame;
static Frame frames[128];
static unsigned frame_count,lock_depth,key_count,key_index;
static u32 keys[128];
static bool port_open,streaming,live_handles[1024],override_header;
static unsigned handle_cursor,connect_count,close_count,send_count,launch_count,system_info_count;
static unsigned command_count,commands[128],status_count;
static bool delay_port,delay_command;
static Result connect_result,send_result,status_result,command_result,system_result,launch_result;
static u32 reply_header,reply_magic,reply_version,rec_state,last_error,seconds,files,launch_state;
static s64 system_options;
static u32 ipc[64];
static void (*wait_hook)(unsigned);
typedef enum { NO_STALL, STATUS_STALL, COMMAND_STALL, LAUNCH_STALL } Stall;
static Stall stall;
static pthread_t worker;
static pthread_mutex_t worker_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t worker_signal = PTHREAD_COND_INITIALIZER;
static bool worker_paused,worker_done,in_menu;
static MyThread *active_thread;
static Result create_result;
static unsigned creates,joins,elapsed_ms;
static _Thread_local bool on_worker;

static void *run_worker(void *arg)
{
    on_worker=true;
    ((MyThread *)arg)->ep();
    pthread_mutex_lock(&worker_lock);
    worker_done=true;
    pthread_cond_broadcast(&worker_signal);
    pthread_mutex_unlock(&worker_lock);
    return NULL;
}
static void wait_for_worker(void)
{
    while(!worker_done&&!worker_paused)
        pthread_cond_wait(&worker_signal,&worker_lock);
}
static Result MyThread_Create(MyThread *thread,void (*entry)(void),void *stack,u32 size,int prio,int affinity)
{
    CHECK(!on_worker&&!active_thread&&!thread->handle&&!lock_depth&&stack&&size>0&&prio>0&&affinity==CORE_SYSTEM);
    ++creates;
    if(R_FAILED(create_result))return create_result;
    pthread_mutex_lock(&worker_lock);
    thread->handle=1024;thread->ep=entry;active_thread=thread;
    worker_paused=worker_done=false;
    CHECK(!pthread_create(&worker,NULL,run_worker,thread));
    wait_for_worker();
    pthread_mutex_unlock(&worker_lock);
    return 0;
}
static Result svcWaitSynchronization(Handle handle,s64 timeout)
{
    CHECK(!on_worker&&timeout==0&&active_thread&&handle==1024);
    if(!worker_done)return (Result)0x09401BFE;
    return 0;
}
static void stall_at(Stall point)
{
    if(stall!=point)return;
    CHECK(on_worker);
    pthread_mutex_lock(&worker_lock);
    worker_paused=true;
    pthread_cond_broadcast(&worker_signal);
    while(stall==point)pthread_cond_wait(&worker_signal,&worker_lock);
    pthread_mutex_unlock(&worker_lock);
}
static void release_worker(void)
{
    pthread_mutex_lock(&worker_lock);
    CHECK(worker_paused&&!worker_done);
    stall=NO_STALL;worker_paused=false;
    pthread_cond_broadcast(&worker_signal);
    wait_for_worker();
    pthread_mutex_unlock(&worker_lock);
}
static u64 svcGetSystemTick(void){return (u64)elapsed_ms*SYSCLOCK_ARM11/1000;}
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
    CHECK(!lock_depth&&frame_count&&timeout>0&&timeout<=500&&key_index<key_count);
    elapsed_ms+=timeout;
    unsigned index=key_index++;if(wait_hook)wait_hook(index);return keys[index];
}
static Result svcConnectToPort(Handle *out,const char *name)
{
    CHECK(!lock_depth&&!strcmp(name,YU_REC_PORT));++connect_count;
    if(R_FAILED(connect_result))return connect_result;
    if(!port_open)return (Result)0xD88007FA;
    CHECK(handle_cursor+1<1024);*out=++handle_cursor;live_handles[*out]=true;return 0;
}
static Result svcCloseHandle(Handle handle)
{
    if(handle==1024)
    {
        CHECK(!on_worker&&active_thread&&worker_done);
        CHECK(!pthread_join(worker,NULL));
        active_thread=NULL;++joins;
        return 0;
    }
    CHECK(handle>0&&handle<1024&&live_handles[handle]);live_handles[handle]=false;++close_count;return 0;
}
static u32 *getThreadCommandBuffer(void) { CHECK(!lock_depth);return ipc; }
static Result svcSendSyncRequest(Handle handle)
{
    CHECK(!lock_depth&&handle>0&&handle<1024&&live_handles[handle]);++send_count;
    CHECK(!in_menu||on_worker);
    if(in_menu)CHECK(frame_count>0);
    stall_at(ipc[0]==YU_REC_STATUS_REQUEST?STATUS_STALL:COMMAND_STALL);
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
        if(R_SUCCEEDED(command_result) && !delay_command)
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
    CHECK(!in_menu||on_worker);stall_at(LAUNCH_STALL);
    if(R_SUCCEEDED(launch_result)&&!delay_port){port_open=true;rec_state=launch_state;}
    return launch_result;
}
static bool RemotePlay_IsRunning(void) { CHECK(!lock_depth);return streaming; }
'''

tests = r'''
static void reset(bool active)
{
    CHECK(!active_thread);
    for(unsigned i=1;i<=handle_cursor;++i)CHECK(!live_handles[i]);
    CHECK(!lock_depth);memset(frames,0,sizeof(frames));memset(keys,0,sizeof(keys));memset(ipc,0,sizeof(ipc));
    frame_count=key_index=0;key_count=6;keys[5]=KEY_B;port_open=active;streaming=false;menuShouldExit=false;
    creates=joins=0;create_result=0;stall=NO_STALL;
    handle_cursor=connect_count=close_count=send_count=launch_count=system_info_count=0;
    command_count=status_count=0;delay_port=delay_command=false;connect_result=send_result=status_result=command_result=system_result=launch_result=0;
    reply_header=0;override_header=false;reply_magic=YU_REC_MAGIC;reply_version=YU_REC_PROTOCOL_VERSION;rec_state=launch_state=YU_REC_IDLE;last_error=0;seconds=42;files=7;
    system_options=2;g_language=LANG_EN;wait_hook=NULL;++case_count;
}
static bool contains(unsigned frame,const char *text)
{
    CHECK(frame<frame_count);for(unsigned i=0;i<frames[frame].count;++i)if(strstr(frames[frame].lines[i].text,text))return true;return false;
}
static void input(u32 first)
{
    memset(keys,0,sizeof(keys));key_index=0;key_count=13;keys[3]=first;keys[12]=KEY_B;
}
static void menu(void)
{
    in_menu=true;RosalinaMenu_Recorder();in_menu=false;
}
static unsigned find_frame(const char *message)
{
    for(unsigned i=0;i<frame_count;++i)if(contains(i,message))return i;
    return frame_count;
}
static bool shown(const char *message){return find_frame(message)<frame_count;}
static void leave_system_menu(unsigned index) { if(index==1)menuShouldExit=true; }
static void late_port(unsigned index) { if(index==6) {port_open=true;rec_state=YU_REC_IDLE;} }
static void slow_boot(unsigned index) { if(index==75)rec_state=YU_REC_IDLE; }
static void expire_boot(unsigned index) { if(index==7)elapsed_ms+=121000; }
static void end_menu_case(void)
{
    if(active_thread)
    {
        CHECK(worker_done&&!worker_paused);
        Recorder_IsRunning();
        CHECK(!active_thread);
    }
}
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
        const char *recording=language?"録画中":"Recording";
        reset(false);g_language=(LangCode)language;menu();
        CHECK(!launch_count&&!command_count&&shown(language?"SD録画":"SD recording")&&shown(language?"無効":"OFF"));
        save_frame(language?"japanese-off.tsv":"english-off.tsv",frame_count-1);end_menu_case();no_live_handles();

        reset(true);g_language=(LangCode)language;menu();
        CHECK(!launch_count&&!command_count&&shown("42")&&shown("7"));end_menu_case();no_live_handles();
        for(unsigned was_running=0;was_running<2;++was_running)
        {
            reset(was_running);g_language=(LangCode)language;input(KEY_A);menu();
            CHECK(launch_count==!was_running&&command_count==1&&commands[0]==YU_REC_CMD_START&&rec_state==YU_REC_RECORDING);
            CHECK(shown(recording));save_frame(language?"japanese-recording.tsv":"english-recording.tsv",find_frame(recording));
            end_menu_case();no_live_handles();
        }
        reset(true);g_language=(LangCode)language;rec_state=YU_REC_RECORDING;input(KEY_A);menu();
        CHECK(!launch_count&&command_count==1&&commands[0]==YU_REC_CMD_STOP&&rec_state==YU_REC_IDLE);end_menu_case();no_live_handles();
        reset(true);g_language=(LangCode)language;input(KEY_Y);menu();
        CHECK(command_count==1&&commands[0]==YU_REC_CMD_EXIT&&!port_open&&!launch_count);end_menu_case();no_live_handles();
        reset(false);g_language=(LangCode)language;system_options=0;input(KEY_A);menu();
        CHECK(!launch_count&&!command_count&&shown(language?"外部モジュール":"external modules"));end_menu_case();no_live_handles();
        reset(false);g_language=(LangCode)language;streaming=true;input(KEY_A);menu();
        CHECK(!launch_count&&!command_count&&!system_info_count&&shown(language?"Yu_ キャプチャ":"Yu_ Capture"));end_menu_case();no_live_handles();
        reset(true);g_language=(LangCode)language;streaming=true;input(KEY_A);menu();
        CHECK(!launch_count&&!command_count);end_menu_case();no_live_handles();
        reset(true);g_language=(LangCode)language;streaming=true;rec_state=YU_REC_RECORDING;input(KEY_A);menu();
        CHECK(command_count==1&&commands[0]==YU_REC_CMD_STOP);end_menu_case();no_live_handles();

        reset(true);g_language=(LangCode)language;stall=STATUS_STALL;key_count=2;keys[1]=KEY_B;menu();
        CHECK(creates==1&&!joins&&worker_paused&&active_thread&&frame_count&&shown(language?"戻る":"back"));
        unsigned probes=connect_count;CHECK(Recorder_IsRunning()&&connect_count==probes);
        key_index=0;key_count=64;memset(keys,0,sizeof(keys));keys[1]=KEY_A;keys[2]=KEY_Y;keys[63]=KEY_B;
        menu();
        const char *timeout=language?"応答しません":"not responding";
        CHECK(creates==1&&!joins&&worker_paused&&!launch_count&&!command_count&&shown(timeout));
        save_frame(language?"japanese-timeout.tsv":"english-timeout.tsv",find_frame(timeout));
        release_worker();CHECK(worker_done);
        key_index=0;key_count=6;memset(keys,0,sizeof(keys));keys[5]=KEY_B;
        frame_count=0;menu();CHECK(shown("42")&&joins>=1&&!command_count);
        end_menu_case();no_live_handles();
    }
    reset(false);system_result=-105;input(KEY_A);menu();CHECK(!launch_count&&!command_count);end_menu_case();no_live_handles();
    reset(false);launch_result=-106;input(KEY_A);menu();CHECK(launch_count==1&&!command_count);end_menu_case();no_live_handles();
    reset(false);delay_port=true;wait_hook=expire_boot;input(KEY_A);menu();CHECK(launch_count==1&&!command_count&&shown("not responding"));end_menu_case();no_live_handles();
    reset(false);delay_port=true;wait_hook=late_port;input(KEY_A);keys[12]=0;key_count=20;keys[19]=KEY_B;menu();CHECK(launch_count==1&&command_count==1&&commands[0]==YU_REC_CMD_START);end_menu_case();no_live_handles();
    reset(false);launch_state=YU_REC_REPLAY;input(KEY_A);menu();CHECK(launch_count==1&&command_count==1&&commands[0]==YU_REC_CMD_START&&rec_state==YU_REC_RECORDING);end_menu_case();no_live_handles();
    reset(false);launch_state=YU_REC_RECORDING;input(KEY_A);menu();CHECK(launch_count==1&&!command_count&&rec_state==YU_REC_RECORDING);end_menu_case();no_live_handles();
    reset(false);launch_state=YU_REC_ERROR;last_error=0xC920181A;input(KEY_A);menu();CHECK(launch_count==1&&!command_count&&shown("0xc920181a"));end_menu_case();no_live_handles();
    reset(false);launch_state=YU_REC_ERROR;input(KEY_A);menu();CHECK(launch_count==1&&!command_count&&shown("0xd8a3fbf5"));end_menu_case();no_live_handles();
    reset(false);launch_state=YU_REC_ERROR;last_error=1;input(KEY_A);menu();CHECK(launch_count==1&&!command_count&&shown("0xd8a3fbf5"));end_menu_case();no_live_handles();
    reset(false);launch_state=YU_REC_STARTING;wait_hook=slow_boot;input(KEY_A);keys[12]=0;key_count=94;keys[93]=KEY_B;menu();
    CHECK(launch_count==1&&command_count==1&&commands[0]==YU_REC_CMD_START&&shown("Starting...")&&!shown("not responding"));end_menu_case();no_live_handles();
    reset(false);launch_state=YU_REC_STARTING;input(KEY_A);menu();CHECK(launch_count==1&&!command_count);end_menu_case();no_live_handles();
    for(unsigned s=0;s<2;s++) {
        reset(true);rec_state=s?YU_REC_RECORDING:YU_REC_IDLE;delay_command=true;input(KEY_A);keys[10]=KEY_A;keys[12]=0;key_count=22;keys[16]=KEY_A;keys[21]=KEY_B;menu();
        CHECK(command_count==1&&commands[0]==(s?YU_REC_CMD_STOP:YU_REC_CMD_START));end_menu_case();no_live_handles();
    }
    reset(true);reply_version++;input(KEY_A);menu();CHECK(!launch_count&&!command_count&&shown("0x"));end_menu_case();no_live_handles();
    reset(true);reply_magic^=1;input(KEY_A);menu();CHECK(!launch_count&&!command_count&&shown("0x"));end_menu_case();no_live_handles();
    reset(true);send_result=(Result)0xC920181A;menu();CHECK(shown("0xc920181a")&&!command_count&&!launch_count);end_menu_case();no_live_handles();
    reset(true);status_result=(Result)0xD88007FA;input(KEY_A);menu();CHECK(shown("0xd88007fa")&&!command_count&&!launch_count);end_menu_case();no_live_handles();
    reset(true);connect_result=(Result)0xD8E007F7;input(KEY_A);menu();CHECK(shown("0xd8e007f7")&&!command_count&&!launch_count&&Recorder_IsRunning());end_menu_case();no_live_handles();
    reset(false);input(KEY_A);keys[9]=KEY_A;menu();CHECK(launch_count==1&&command_count==2&&commands[0]==YU_REC_CMD_START&&commands[1]==YU_REC_CMD_STOP);end_menu_case();no_live_handles();
    reset(true);input(KEY_A|KEY_B);menu();CHECK(!launch_count&&!command_count);end_menu_case();no_live_handles();
    reset(false);input(KEY_Y);menu();CHECK(!launch_count&&!command_count);end_menu_case();no_live_handles();
    reset(false);menuShouldExit=true;menu();CHECK(!launch_count&&!command_count&&!send_count&&!frame_count);end_menu_case();no_live_handles();
    reset(true);command_result=(Result)YU_REC_ERROR_STATE;input(KEY_A);menu();CHECK(command_count==1&&rec_state==YU_REC_IDLE&&shown("0xd8a3fbf5"));end_menu_case();no_live_handles();
    for(unsigned state=YU_REC_STARTING;state<=YU_REC_REPLAY;++state)
    {
        reset(true);rec_state=state;seconds=files=UINT32_MAX;
        if(state!=YU_REC_IDLE&&state!=YU_REC_REPLAY&&state!=YU_REC_RECORDING)input(KEY_A);
        menu();
        CHECK(!launch_count&&!command_count&&shown("4294967295"));end_menu_case();no_live_handles();
        if(state!=YU_REC_IDLE&&state!=YU_REC_REPLAY&&state!=YU_REC_RECORDING)CHECK(!shown("A: record")&&!shown("A: stop"));
    }
    reset(false);create_result=-107;input(KEY_A);menu();
    CHECK(creates>0&&!active_thread&&!launch_count&&!command_count&&shown("0xffffff95"));end_menu_case();no_live_handles();
    reset(false);stall=LAUNCH_STALL;input(KEY_A);menu();
    CHECK(worker_paused&&launch_count==1&&!command_count);
    release_worker();CHECK(!command_count&&worker_done);end_menu_case();no_live_handles();
    reset(false);stall=STATUS_STALL;input(KEY_A);menu();
    CHECK(worker_paused&&launch_count==1&&!command_count);
    release_worker();CHECK(!command_count&&worker_done);end_menu_case();no_live_handles();
    reset(true);stall=STATUS_STALL;wait_hook=leave_system_menu;menu();
    CHECK(worker_paused&&key_index==2&&!command_count);
    release_worker();end_menu_case();no_live_handles();
    reset(true);stall=COMMAND_STALL;input(KEY_A);keys[6]=KEY_A;keys[7]=KEY_Y;menu();
    CHECK(worker_paused&&active_thread&&!command_count);
    release_worker();CHECK(command_count==1&&commands[0]==YU_REC_CMD_START);
    end_menu_case();no_live_handles();
    printf("%u recorder client assertions across %u cases passed\n",checks,case_count);
    return 0;
}
'''

harness = prefix + labels + body + tests
(generated / 'test.c').write_text(harness, encoding='utf-8')
target = generated / ('test.exe' if os.name == 'nt' else 'test')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-pthread', '-I' + str(module / 'include'),
                str(generated / 'test.c'), str(module / 'source/text.c'), '-o', str(target)], check=True, env=environment)
result = subprocess.run([str(target)], cwd=generated, capture_output=True, text=True, env=environment, timeout=20)
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
          'streaming_exclusion': True, 'snapshots': snapshots, 'wedged_ipc_tested': True,
          'menu_exit_while_blocked': True, 'cancellation_during_launch': True,
          'source_sha256': {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in [source, protocol, language]}}
(generated / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
