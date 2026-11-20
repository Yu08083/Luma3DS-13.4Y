import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
patch = (ROOT / 'third_party/recorder/control.patch').read_text(encoding='utf-8')
files = {}
for block in patch.split('diff --git ')[1:]:
    name = block.splitlines()[0].split(' b/', 1)[1]
    if 'new file mode 100644\n' in block:
        files[name] = ''.join(line[1:] + '\n' for line in block.splitlines() if line.startswith('+') and not line.startswith('+++'))
protocol = (ROOT / 'Luma3DS_v13_4/sysmodules/rosalina/include/recorder_protocol.h').read_text(encoding='utf-8')
assert files['sysmod/source/recorder_protocol.h'] == protocol
assert patch.count('+    Control_Stop();') == 1
assert patch.count('+        Control_Stop();') == 1
assert '+    AcceptSession: 0x4A' in patch

types = '''#ifndef HOST_TYPES_H
#define HOST_TYPES_H
#include <stdint.h>
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;
#endif
'''
shared_bytes = (ROOT / 'tests/fixtures/recorder-v1.4-shared.h').read_bytes()
assert hashlib.sha256(shared_bytes).hexdigest() == 'd59618d9315d988a300d8284b2d259b5e9cca613d21913293e355da7ca908f3a'
shared = shared_bytes.decode('utf-8')
sdk = r'''#ifndef HOST_3DS_H
#define HOST_3DS_H
#include <3ds/types.h>
#include <stdbool.h>
#include <stddef.h>
typedef s32 Result;
typedef u32 Handle;
typedef void *Thread;
#define R_FAILED(r) ((r) < 0)
#define R_SUCCEEDED(r) ((r) >= 0)
#define RESET_STICKY 1
u32 *getThreadCommandBuffer(void);
Result svcReplyAndReceive(s32 *, const Handle *, s32, Handle);
Result svcCloseHandle(Handle);
Result svcAcceptSession(Handle *, Handle);
Result svcCreateEvent(Handle *, unsigned);
Result svcCreatePort(Handle *, Handle *, const char *, s32);
Result svcSignalEvent(Handle);
Thread threadCreate(void (*)(void *), void *, size_t, int, int, bool);
Result threadJoin(Thread, u64);
void threadFree(Thread);
#endif
'''
test = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "control.c"
volatile RecShared g_shared;
static u32 buffer[64];
static unsigned checked, closeCount[128], failStage, scenario, eventIndex, joined, freed, signalled;
static void (*entry)(void *);
u32 *getThreadCommandBuffer(void) { return buffer; }
Result svcCloseHandle(Handle h) { assert(h && h < 128); closeCount[h]++; return 0; }
Result svcAcceptSession(Handle *h, Handle p) { assert(p == 11); *h = 20 + scenario; return 0; }
Result svcCreateEvent(Handle *h, unsigned reset) { assert(reset == RESET_STICKY); if(failStage==1)return -1; *h=10; return 0; }
Result svcCreatePort(Handle *s, Handle *c, const char *name, s32 n) { assert(!strcmp(name,"yu:rec") && n==2); if(failStage==2)return -1;*s=11;*c=12;return 0; }
Result svcSignalEvent(Handle h) { assert(h==10); signalled++;return 0; }
Thread threadCreate(void (*fn)(void *),void *a,size_t stack,int p,int core,bool d) {assert(!a && stack==0x2000 && p==0x30 && core==1 && !d);entry=fn;return failStage==3?NULL:(Thread)(uintptr_t)99;}
Result threadJoin(Thread t,u64 ns) { assert(t==(Thread)(uintptr_t)99 && ns==1000000000ull);joined++;return failStage==4?-1:0; }
void threadFree(Thread t) {assert(t==(Thread)(uintptr_t)99);freed++;}
Result svcReplyAndReceive(s32 *index,const Handle *h,s32 n,Handle reply)
{
    assert(h[0]==10 && h[1]==11);
    switch(eventIndex++) {
    case 0: assert(n==2 && !reply);*index=1;return 0;
    case 1: assert(n==3 && h[2]==20+scenario && !reply);buffer[0]=YU_REC_STATUS_REQUEST;*index=2;return 0;
    case 2:
        assert(reply==20+scenario && buffer[0]==YU_REC_STATUS_REPLY && buffer[2]==YU_REC_MAGIC);
        if(scenario==0){buffer[0]=YU_REC_COMMAND_REQUEST;buffer[1]=YU_REC_CMD_START;*index=2;return 0;}
        if(scenario==1){*index=2;return (Result)0xC920181Au;}
        if(scenario==2){*index=-1;return (Result)0xC920181Au;}
        if(scenario==3){*index=-1;return -1;}
        *index=0;return 0;
    case 3:
        if(scenario==0){assert(reply==20 && buffer[0]==YU_REC_COMMAND_REPLY && !buffer[1]);*index=0;return 0;}
        assert(n==2 && !reply);*index=0;return 0;
    }
    assert(0);return -1;
}
static void reset(void)
{
    memset(buffer,0,sizeof(buffer));memset((void *)&g_shared,0,sizeof(g_shared));
    memset(closeCount,0,sizeof(closeCount));
    g_controlThread=NULL;g_controlStop=g_controlPort=0;g_controlExit=false;
    joined=freed=signalled=eventIndex=failStage=0;entry=NULL;
}
static void command(u32 header,u32 action) {buffer[0]=header;buffer[1]=action;YuRecorder_Handle(buffer,&g_shared,&g_controlExit);checked++;}
int main(void)
{
    _Static_assert((int)YU_REC_IDLE==(int)REC_STATE_IDLE,"state ABI");
    _Static_assert((int)YU_REC_REPLAY==(int)REC_STATE_REPLAY,"state ABI");
    _Static_assert((int)YU_REC_CMD_SAVE_CLIP==(int)REC_CMD_SAVE_CLIP,"command ABI");
    reset();g_shared.recSeconds=7;g_shared.recKb=800;g_shared.framesTop=90;g_shared.framesBot=50;g_shared.dropped=6;g_shared.files=2;g_shared.lastError=-33;g_shared.replayOn=1;
    command(YU_REC_STATUS_REQUEST,0);assert(buffer[0]==YU_REC_STATUS_REPLY && buffer[1]==0 && buffer[2]==YU_REC_MAGIC && buffer[3]==1);
    assert(buffer[5]==7 && buffer[6]==800 && buffer[7]==90 && buffer[8]==50 && buffer[9]==6 && buffer[10]==2 && buffer[11]==(u32)-33 && buffer[12]==1);
    for(u32 state=0;state<=REC_STATE_REPLAY;state++)for(u32 action=1;action<=5;action++) {
        reset();g_shared.state=state;
        bool valid=action==1 || (action==2 && (state==1 || state==6)) || (action==3 && state==2) || (action==4 && (state==1 || state==2 || state==6)) || (action==5 && state==6);
        command(YU_REC_COMMAND_REQUEST,action);
        assert(buffer[0]==YU_REC_COMMAND_REPLY);
        if(state==REC_STATE_EXITED){assert(buffer[1]==YU_REC_ERROR_EXITING && !g_shared.cmd);}
        else if(valid){assert(!buffer[1] && g_shared.cmd==action && g_controlExit==(action==1));}
        else {assert(buffer[1]==YU_REC_ERROR_STATE && !g_shared.cmd);}
    }
    for(u32 action=0;action<12;action++)if(action==0 || action>5){reset();g_shared.state=1;command(YU_REC_COMMAND_REQUEST,action);assert(buffer[1]==YU_REC_ERROR_INVALID && !g_shared.cmd);}
    const u32 invalid[]={0,0x10040,0x10002,0x10001,0x20000,0x20080,0x20042,0x20041,0x30040,0xFFFF0000,0xFFFF0040};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++){reset();g_shared.state=1;command(invalid[i],1);assert(buffer[1]==YU_REC_ERROR_INVALID && !g_shared.cmd && !g_controlExit);}
    reset();g_shared.state=1;g_shared.cmd=REC_CMD_RELOAD;command(YU_REC_COMMAND_REQUEST,YU_REC_CMD_START);assert(buffer[1]==YU_REC_ERROR_BUSY && g_shared.cmd==REC_CMD_RELOAD);
    reset();g_shared.state=1;command(YU_REC_COMMAND_REQUEST,YU_REC_CMD_EXIT);assert(!buffer[1]);g_shared.cmd=0;command(YU_REC_COMMAND_REQUEST,YU_REC_CMD_START);assert(buffer[1]==YU_REC_ERROR_EXITING && !g_shared.cmd);
    command(YU_REC_STATUS_REQUEST,0);assert(!buffer[1]);
    for(unsigned fail=1;fail<=3;fail++){reset();failStage=fail;assert(!Control_Start());assert(!g_controlThread && !g_controlStop);assert(closeCount[10]==(fail>1));assert(closeCount[11]==(fail==3));assert(closeCount[12]==(fail==3));checked++;}
    for(scenario=0;scenario<5;scenario++){reset();g_shared.state=1;assert(Control_Start());assert(Control_Start());assert(closeCount[12]==1);entry(NULL);assert(closeCount[11]==1 && closeCount[20+scenario]==1);Control_Stop();assert(signalled==1 && joined==1 && freed==1 && closeCount[10]==1);Control_Stop();assert(joined==1);checked++;}
    reset();assert(Control_Start());failStage=4;Control_Stop();assert(joined==1 && !freed && !closeCount[10] && g_controlThread);checked++;
    printf("recorder server: %u checks passed\n",checked);
    return 0;
}
'''
cc = os.environ.get('CC') or shutil.which('gcc') or shutil.which('clang')
if not cc:
    raise SystemExit('A C compiler is required')
with tempfile.TemporaryDirectory(prefix='yu-recorder-server-') as temp:
    directory = Path(temp)
    (directory / '3ds').mkdir()
    (directory / '3ds/types.h').write_text(types, encoding='utf-8')
    (directory / '3ds.h').write_text(sdk, encoding='utf-8')
    (directory / 'shared.h').write_text(shared, encoding='utf-8')
    for name, content in files.items():
        if name.startswith('sysmod/source/'):
            (directory / Path(name).name).write_text(content, encoding='utf-8')
    (directory / 'test.c').write_text(test, encoding='utf-8')
    binary = directory / ('server.exe' if os.name == 'nt' else 'server')
    subprocess.run([cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I', str(directory), str(directory / 'test.c'), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
    print(result.stdout.strip())
print(json.dumps({'patch_sha256': hashlib.sha256(patch.encode()).hexdigest(), 'protocol_sha256': hashlib.sha256(protocol.encode()).hexdigest()}))
