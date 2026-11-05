import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
compiler = shutil.which(os.environ.get('CC', 'cc')) or shutil.which('gcc')
if not compiler and Path('C:/msys64/ucrt64/bin/gcc.exe').exists():
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
assert compiler, 'A native C compiler is required'
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
source = (ROOT / 'Luma3DS_v13_4/k11_extension/source/svc/TerminateProcess.c').read_text(encoding='utf-8')
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
typedef uint32_t u32;
typedef int64_t s64;
typedef uint32_t Handle;
typedef int32_t Result;
typedef struct KAutoObject KAutoObject;
typedef struct { void (*DecrementReferenceCount)(KAutoObject *); } Vtable;
struct KAutoObject { Vtable *vtable; };
typedef struct { KAutoObject autoObject; u32 customFlags; } KProcess;
typedef struct { int unused; } KProcessHandleTable;
typedef struct { struct { KProcess *currentProcess; } objectContext; } KCoreContext;
#define CUR_PROCESS_HANDLE 0xFFFF8001u
#define KPROCESS_GET_PTR(process, member) (&(process)->member)
enum { SignalOnExit = 4, PLG_CFG_RUNNING = 1, PLG_CFG_INHOME = 2, PLG_CFG_EXIT_EVENT = 9 };
static KCoreContext core;
static KCoreContext *currentCoreContext = &core;
static KProcessHandleTable handles;
static KProcess current, foreign;
static KProcess *expected;
static void *officialSVCs[0x80];
static unsigned resets, releases, references, signals, originals;
static u32 pluginStatus;
static s64 observedTimeout;
static Handle observedHandle;
static KProcessHandleTable *handleTableOfProcess(KProcess *process) {
    assert(process == &current);
    return &handles;
}
static KProcess *KProcessHandleTable__ToKProcess(KProcessHandleTable *table, Handle handle) {
    assert(table == &handles);
    return handle == 0x123u ? &foreign : NULL;
}
static void KAutoObject__AddReference(KAutoObject *object) {
    assert(object == &current.autoObject);
    references++;
}
static void release(KAutoObject *object) {
    assert(object == (KAutoObject *)expected);
    releases++;
}
static void PlgUdsResetProcess(KProcess *process) {
    assert(process == expected && signals == 0 && originals == 0);
    resets++;
}
static u32 PLG_GetStatus(void) { return pluginStatus; }
static void PLG_SignalEventWithTimeout(u32 event, s64 timeout) {
    assert(event == PLG_CFG_EXIT_EVENT && resets == 1 && originals == 0);
    observedTimeout = timeout;
    signals++;
}
static Result originalTerminate(Handle handle) {
    assert(resets == (expected != NULL));
    assert(releases == (expected != NULL));
    observedHandle = handle;
    originals++;
    return 1234;
}
'''
suffix = r'''
static void run(Handle handle, u32 flags, u32 status, bool signalExpected) {
    resets = releases = references = signals = originals = 0;
    observedTimeout = 0;
    current.customFlags = foreign.customFlags = flags;
    pluginStatus = status;
    expected = handle == CUR_PROCESS_HANDLE ? &current : handle == 0x123u ? &foreign : NULL;
    assert(TerminateProcessHook(handle) == 1234);
    assert(observedHandle == handle && originals == 1);
    assert(resets == (expected != NULL) && releases == (expected != NULL));
    assert(references == (handle == CUR_PROCESS_HANDLE));
    assert(signals == signalExpected);
    if (signalExpected) {
        assert(observedTimeout == 15000000000LL);
        assert((expected->customFlags & SignalOnExit) == 0);
    } else if (expected) {
        assert(expected->customFlags == flags);
    }
}
int main(void) {
    Vtable vtable = {release};
    current.autoObject.vtable = foreign.autoObject.vtable = &vtable;
    core.objectContext.currentProcess = &current;
    officialSVCs[0x76] = (void *)originalTerminate;
    run(0x456u, SignalOnExit, PLG_CFG_RUNNING, false);
    run(0x123u, SignalOnExit, PLG_CFG_RUNNING, true);
    run(0x123u, SignalOnExit, PLG_CFG_INHOME, true);
    run(CUR_PROCESS_HANDLE, SignalOnExit, PLG_CFG_RUNNING, true);
    run(0x123u, SignalOnExit, 0, false);
    run(0x123u, 0, PLG_CFG_RUNNING, false);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    generated = directory / 'terminate.c'
    generated.write_text(prefix + '\n' + source + '\n' + suffix, encoding='utf-8')
    output = directory / ('terminate.exe' if os.name == 'nt' else 'terminate')
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', str(generated), '-o', str(output)], check=True, env=environment)
    subprocess.run([str(output)], check=True, env=environment)
print('Forced termination and Y plugin cleanup: 6 cases passed')
