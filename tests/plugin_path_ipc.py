import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path)
args = parser.parse_args()
compiler = shutil.which('cc') or shutil.which('gcc') or 'C:/msys64/ucrt64/bin/gcc.exe'
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
path = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina/source/plugin/plgloader.c'
source = path.read_text(encoding='utf8')
body = source[source.index('        case 10:'):source.index('        case 11:')]
legacy = (ROOT / 'tests/fixtures/plugin-path-before-y09.c').read_text(encoding='utf8')
prefix = r'''
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t u32;
#define IPC_BUFFER_R 2
#define IPC_BUFFER_W 4
#define IPC_BUFFER_RW 6
#define IPC_Desc_Buffer(n,d) (((n) << 4) | 8 | (d))
#define IPC_MakeHeader(c,n,t) (((c) << 16) | ((n) << 6) | (t))
#define MAKERESULT(...) 0xD9001234u
static struct { const char *pluginPath; } storage, *ctx = &storage;
static void error(uintptr_t *command, u32 result) { command[0] = 0xA0040; command[1] = result; }
'''
def handler(name, text):
    return 'static __attribute__((noinline)) void ' + name + '(uintptr_t *cmdbuf) { switch(cmdbuf[0] >> 16) {\n' + text.replace('(u32)path', '(uintptr_t)path') + '\n} }\n'
cases = r'''
static unsigned cases;
static void run(const char *value, unsigned capacity, unsigned direction, bool nullOutput)
{
    unsigned char output[512];
    memset(output, 0xA5, sizeof(output));
    ctx->pluginPath = value;
    uintptr_t command[4] = {IPC_MakeHeader(10,0,2), IPC_Desc_Buffer(capacity,direction), nullOutput ? 0 : (uintptr_t)(output+16), 0};
    bool valid = value && !nullOutput && capacity >= 2 && capacity <= 256 &&
        (direction == IPC_BUFFER_W || direction == IPC_BUFFER_RW) && strnlen(value,256) < capacity;
    current(command);
    if (valid)
    {
        assert(command[0] == IPC_MakeHeader(10,1,2) && !command[1]);
        assert(command[2] == IPC_Desc_Buffer(capacity,direction) && command[3] == (uintptr_t)(output+16));
        assert(!strcmp((const char *)(output+16),value));
        for (unsigned i = 16 + strlen(value) + 1; i < sizeof(output); i++) assert(output[i] == 0xA5);
    }
    else
    {
        assert(command[0] == 0xA0040 && (int32_t)command[1] < 0);
        for (unsigned i = 0; i < sizeof(output); i++) assert(output[i] == 0xA5);
    }
    for (unsigned i = 0; i < 16; i++) assert(output[i] == 0xA5);
    cases++;
}
int main(void)
{
    char path[301];
    for (unsigned length = 0; length <= 300; length++)
    {
        memset(path,'a',length); path[length] = 0;
        for (unsigned capacity = 0; capacity <= 257; capacity++)
        {
            run(path,capacity,IPC_BUFFER_RW,false);
            run(path,capacity,IPC_BUFFER_W,false);
        }
    }
    const char *paths[] = {"/luma/plugins/0004000000155100/default.3gx", "/luma/plugins/default.3gx", "/luma/plugins/0004000000155100/日本語.3gx"};
    for (unsigned i = 0; i < sizeof(paths)/sizeof(*paths); i++)
    {
        run(paths[i],255,IPC_BUFFER_RW,false);
        run(paths[i],256,IPC_BUFFER_RW,false);
        run(paths[i],255,IPC_BUFFER_R,false);
        run(paths[i],255,0,false);
        run(paths[i],255,IPC_BUFFER_RW,true);
    }
    run(NULL,255,IPC_BUFFER_RW,false);
    unsigned char output[300]; memset(output,0xA5,sizeof(output));
    ctx->pluginPath = paths[0];
    uintptr_t small[4] = {IPC_MakeHeader(10,0,2),IPC_Desc_Buffer(8,IPC_BUFFER_RW),(uintptr_t)output,0};
    old(small);
    assert(output[8] != 0xA5);
    memset(path,'a',255); path[255]=0; ctx->pluginPath=path;
    memset(output,0xA5,sizeof(output));
    uintptr_t full[4] = {IPC_MakeHeader(10,0,2),IPC_Desc_Buffer(255,IPC_BUFFER_RW),(uintptr_t)output,0};
    old(full);
    assert(!memchr(output,0,255));
    printf("Plugin path IPC: %u cases and 2 rejected legacy boundary failures passed\n",cases);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temporary:
    folder = Path(temporary)
    generated = folder / 'path.c'
    generated.write_text(prefix + handler('current', body) + handler('old', legacy) + cases, encoding='utf8')
    target = folder / ('path.exe' if os.name == 'nt' else 'path')
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', str(generated), '-o', str(target)], check=True, env=environment)
    result = subprocess.run([str(target)], check=True, env=environment, capture_output=True, text=True)
    print(result.stdout, end='')
count = int(result.stdout.split()[3])
assert count == 155332
report = {'all_passed': True, 'case_count': count, 'legacy_negative_controls': 2,
          'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'hardware_tested': False,
          'boundaries': ['Actual case 10 executes with pointer-width adaptation in a host fixture.',
                         'Kernel IPC mapping, physical SD reads and the reported normal-length TXT failure are not demonstrated.']}
if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
