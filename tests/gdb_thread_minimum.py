import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina/source/gdb/thread.c'
raw = SOURCE.read_text()


def function(name):
    start = raw.rfind('\n', 0, raw.index(name + '(')) + 1
    opened = raw.index('{', start)
    end, depth = opened + 1, 1
    while depth:
        depth += (raw[end] == '{') - (raw[end] == '}')
        end += 1
    return raw[start:end]


comparator = function('thread_compare_func')
assert hashlib.sha256(comparator.encode()).hexdigest() == 'd317e4c0bf9b327270b99f1086d269f70691177a061184d763e8da05c5b5dff5'
selected = function('GDB_GetCurrentThreadFromList')
assert 'qsort(' not in raw
assert re.search(r'#define\s+MAX_DEBUG_THREAD\s+127\b', (ROOT / 'Luma3DS_v13_4/sysmodules/rosalina/include/gdb.h').read_text())
prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t u32;
typedef int32_t s32;
typedef int64_t s64;
typedef u32 Handle;
typedef s32 Result;
typedef struct {u32 pid,debug;} GDBContext;
struct ThreadIdWithCtx {GDBContext *ctx;u32 id;};
#define MAX_DEBUG_THREAD 127
#define R_FAILED(r) ((r)<0)
#define DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW 1
#define DBGTHREAD_PARAMETER_PRIORITY 0
static struct {u32 mask;s32 dynamic,priority;bool fail;} threads[128];
static GDBContext context = {77,88};
static Result svcOpenProcess(Handle *p,u32 pid){assert(pid==77);*p=99;return 0;}
static Result svcOpenThread(Handle *p,Handle process,u32 id){assert(process==99 && id>0 && id<128);*p=id;return threads[id].fail?-1:0;}
static Result svcGetThreadPriority(s32 *p,Handle h){*p=threads[h].dynamic;return 0;}
static Result svcCloseHandle(Handle h){(void)h;return 0;}
static Result svcGetDebugThreadParam(s64 *dummy,u32 *p,Handle debug,u32 id,u32 type){(void)dummy;assert(debug==88);if(threads[id].fail)return -1;*p=type?threads[id].mask:(u32)threads[id].priority;return 0;}
'''
reference = r'''
static u32 reference(GDBContext *ctx,u32 *threadIds,u32 nbThreads)
{
    struct ThreadIdWithCtx lst[MAX_DEBUG_THREAD];
    for(u32 i=0;i<nbThreads;i++){lst[i].ctx=ctx;lst[i].id=threadIds[i];}
    qsort(lst,nbThreads,sizeof(struct ThreadIdWithCtx),thread_compare_func);
    return lst[0].id;
}
static u32 seed=0x13520;
static u32 next(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void)
{
    u32 counts[]={1,2,3,4,7,8,15,31,63,127};
    unsigned cases=0,ties=0,failures=0;
    assert(GDB_GetCurrentThreadFromList(&context,NULL,0)==0);
    for(u32 c=0;c<10;c++)for(u32 scenario=0;scenario<40;scenario++)
    {
        u32 count=counts[c],input[129],saved[129];
        input[0]=0xA5A5A5A5;input[count+1]=0x5A5A5A5A;
        for(u32 i=1;i<=count;i++)
        {
            input[i]=i;
            threads[i].mask=next()%4;
            threads[i].dynamic=(s32)(next()%66);
            threads[i].priority=(s32)(next()%66);
            threads[i].fail=scenario%5==0 && i%3==0;
            if(scenario%7==0){threads[i].mask=1;threads[i].dynamic=5;threads[i].priority=9;threads[i].fail=false;}
            failures+=threads[i].fail;
        }
        for(u32 i=count;i>1;i--){u32 j=1+next()%i,tmp=input[i];input[i]=input[j];input[j]=tmp;}
        memcpy(saved,input,sizeof(u32)*(count+2));
        u32 old=reference(&context,input+1,count),current=GDB_GetCurrentThreadFromList(&context,input+1,count),equivalent=0;
        struct ThreadIdWithCtx a={&context,current},b={&context,old};
        assert(thread_compare_func(&a,&b)==0);
        for(u32 i=1;i<=count;i++){struct ThreadIdWithCtx other={&context,input[i]};assert(thread_compare_func(&a,&other)<=0);equivalent+=thread_compare_func(&a,&other)==0;}
        if(equivalent==1)assert(old==current);else ties++;
        assert(memcmp(input,saved,sizeof(u32)*(count+2))==0);
        cases++;
    }
    printf("{\"selection_cases\":%u,\"tied_minimum_cases\":%u,\"service_failure_inputs\":%u}\n",cases,ties,failures);
    return 0;
}
'''
compiler = shutil.which(os.environ.get('CC', 'cc')) or shutil.which('gcc')
if not compiler and Path('C:/msys64/ucrt64/bin/gcc.exe').exists():
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
assert compiler
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='gdb-minimum-') as directory:
    stage = Path(directory)
    source = stage / 'test.c'
    source.write_text(prefix + function('GDB_GetDynamicThreadPriority') + '\n' + comparator + '\n' + selected + '\n' + reference)
    target = stage / ('test.exe' if os.name == 'nt' else 'test')
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(target)], check=True, env=environment)
    result = json.loads(subprocess.check_output([str(target)], text=True, env=environment))
report = {'all_passed': True, 'source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(), 'comparator_sha256': hashlib.sha256(comparator.encode()).hexdigest(), 'unique_minimum_exact': True, 'ties_comparator_equivalent': True, 'input_and_guards_unchanged': True, 'empty_returns_zero': True, **result}
output = ROOT / 'build/gdb-thread-minimum-tests'
output.mkdir(parents=True, exist_ok=True)
(output / 'proof.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
