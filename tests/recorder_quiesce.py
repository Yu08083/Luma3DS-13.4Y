import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def run(source):
    code = source.read_text(encoding="utf-8")
    start = code.index("static void QuiesceWorkers(void)")
    stop = code.index("static void StopRecording(", start)
    quiesce = code[start:stop]
    cleanup = code[stop:code.index("static void WriteSystemStop(", stop)]
    for operation in ("sl->state = 0;", "ReleaseEncoderMemory(aq);", "BigFreeAll();"):
        assert cleanup.index("QuiesceWorkers();") < cleanup.index(operation)
    for name, work in (("ConverterMain", "ConverterStep();"), ("PackerMain", "PackStage();")):
        body = re.search(r"static void " + name + r"\(void \*arg\)\n\{(.*?)\n\}", code, re.S).group(1)
        assert body.index("capture_gate_enter(") < body.index(work) < body.index("capture_gate_leave(")
    harness = r'''
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdatomic.h>
#include "capturegate.h"
typedef unsigned u32;
typedef int s32;
typedef int Handle;
#define CONVERTER_PRIO_BOOST 16
#define R_SUCCEEDED(result) ((result) >= 0)
static uint32_t g_captureGate = CAPTURE_GATE_CLOSED;
static u32 g_convOwner=1, g_convCount=2;
static Handle g_convThreads[]={1,2}, g_convThread=2, g_packThread=3;
static int g_convEvent, g_packEvent;
static atomic_uint entered, releaseMask, writes, sleeps, freed;
static int prio[]={0,40,41,42};
static Handle threadGetHandle(Handle h){return h;}
static void LightEvent_Signal(int *p){assert(p==&g_convEvent || p==&g_packEvent);}
static int svcGetThreadPriority(s32 *p,Handle h){*p=prio[h];return 0;}
static int svcSetThreadPriority(Handle h,s32 p){prio[h]=p;return 0;}
static void svcSleepThread(uint64_t ns){assert(ns==5000000);++sleeps;sched_yield();}
''' + quiesce + r'''
static void *worker(void *p) {
    unsigned bit=(unsigned)(uintptr_t)p;
    assert(capture_gate_enter(&g_captureGate));
    atomic_fetch_or(&entered,bit);
    while (!(atomic_load(&releaseMask)&bit))sched_yield();
    assert(!atomic_load(&freed));
    atomic_fetch_or(&writes,bit);
    capture_gate_leave(&g_captureGate);
    return NULL;
}
static void *stop(void *unused) {
    (void)unused;
    QuiesceWorkers();
    assert(atomic_load(&writes)==3);
    atomic_store(&freed,1);
    return NULL;
}
int main(void) {
    for(unsigned first=1;first<=2;++first) {
        pthread_t a,b,s;
        entered=releaseMask=writes=sleeps=freed=0;
        assert(!capture_gate_enter(&g_captureGate));
        capture_gate_open(&g_captureGate);
        assert(!pthread_create(&a,NULL,worker,(void *)(uintptr_t)1));
        assert(!pthread_create(&b,NULL,worker,(void *)(uintptr_t)2));
        while(atomic_load(&entered)!=3)sched_yield();
        assert(!pthread_create(&s,NULL,stop,NULL));
        while(atomic_load(&sleeps)<6600)sched_yield();
        assert(!capture_gate_enter(&g_captureGate) && !atomic_load(&freed));
        atomic_store(&releaseMask,first);
        assert(!pthread_join(first==1?a:b,NULL));
        assert(!atomic_load(&freed) && !capture_gate_enter(&g_captureGate));
        atomic_store(&releaseMask,3);
        assert(!pthread_join(first==1?b:a,NULL));
        assert(!pthread_join(s,NULL));
        assert(atomic_load(&freed) && !capture_gate_busy(&g_captureGate));
        assert(prio[1]==40 && prio[3]==42);
    }
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="recorder-quiesce-") as tmp:
        tmp = Path(tmp)
        test = tmp / "test.c"
        test.write_text(harness, encoding="utf-8")
        image = tmp / "test.exe"
        compiler = shutil.which("cc") or shutil.which("gcc")
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-pthread",
                        "-I" + str(source.parent), str(test), "-o", str(image)], check=True)
        subprocess.run([str(image)], check=True, timeout=20)
    print("Recording stop preserved both outstanding workers across both completion orders.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    run(parser.parse_args().source)
