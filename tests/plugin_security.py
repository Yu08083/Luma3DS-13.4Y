import os
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'tests/generated/plugin-security'
(generated / '3ds').mkdir(parents=True, exist_ok=True)
(generated / '3ds/types.h').write_text('#include <stdint.h>\ntypedef uint8_t u8;\ntypedef uint32_t u32;\ntypedef uint64_t u64;\n', encoding='utf-8')
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt': compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
prefix = r'''
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sha256.h"
#include "plugin/payload_vm.h"
#include "plugin/online_bridge.h"
typedef int32_t Result;
typedef int LightLock;
static void LightLock_Lock(LightLock *lock) { assert(*lock == 1); *lock = 0; }
static void LightLock_Unlock(LightLock *lock) { assert(*lock == 0); *lock = 1; }
typedef struct { u64 pos; } IFile;
typedef struct { void *memblock; } MemoryBlock;
typedef struct { MemoryBlock memblock; bool isSwapFunctionset; u32 swapLoadChecksum; } Context;
typedef Context PluginLoaderContext;
static Context PluginLoaderCtx;
static unsigned char disk[512];
static u64 diskSize;
static bool shortRead, failWrite;
static int reboots, opens;
#define R_FAILED(r) ((r) < 0)
#define CUR_PROCESS_HANDLE 0xFFFF8001u
#define ARCHIVE_SDMC 0
#define PATH_EMPTY 0
#define PATH_ASCII 0
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define FS_OPEN_RWC 7
#define FS_WRITE_FLUSH 1
#define fsMakePath(t,d) (d)
static Result IFile_Open(IFile *f, int a, const char *b, const char *c, int flags) {
    (void)a; (void)b; (void)flags; assert(!strcmp(c, "/luma/plugins/.swap")); f->pos = 0; ++opens; return 0;
}
static void IFile_Close(IFile *f) { (void)f; --opens; }
static Result IFile_Write(IFile *f, u64 *total, void *buffer, u32 size, int flags) {
    (void)f; assert(flags == FS_WRITE_FLUSH); *total = failWrite ? size - 1 : size;
    memcpy(disk, buffer, *total); diskSize = *total; return failWrite ? -1 : 0;
}
static Result IFile_Read(IFile *f, u64 *total, void *buffer, u32 size) {
    (void)f; *total = shortRead ? size - 1 : size; memcpy(buffer, disk, *total); return 0;
}
static Result IFile_GetSize(IFile *f, u64 *size) { (void)f; *size = diskSize; return 0; }
static Result IFile_SetSize(IFile *f, u64 size) { (void)f; diskSize = size; return 0; }
static void PluginLoader__Error(const char *message, Result result) { (void)message; assert(result < 0); }
static void svcKernelSetState(int state) { assert(state == 7); ++reboots; }
static void svcFlushDataCacheRange(const void *data, u32 size) { (void)data; (void)size; }
static Result svcFlushProcessDataCache(u32 process, u32 address, u32 size) { assert(process == CUR_PROCESS_HANDLE && address && size == 64); return 0; }
static void svcFlushEntireDataCache(void) {}
'''
loader = (module / 'source/plugin/file_loader.c').read_text()
memory = (module / 'source/plugin/memoryblock.c').read_text()
globals_ = memory[memory.index('u32  g_loadSaveSwapArgs'):memory.index('Result     MemoryBlock__SetSize')]
globals_ = globals_.replace('(u32)onlineBridge', '(u32)(uintptr_t)onlineBridge')
swap = memory[memory.index('static Result swapFailure'):memory.index('Result     MemoryBlock__MountInProcess')]
settings = memory[memory.index('Result MemoryBlock__SetSwapSettings'):memory.index('PluginHeader* MemoryBlock__GetMappedPluginHeader')]
path_helpers = loader[loader.index('static bool PluginPathCanAppend'):loader.index('static Result   EnumeratePluginFiles')]
commands = (module / 'source/plugin/plgloader.c').read_text()
swap_command = commands[commands.index('        case 12:'):commands.index('        case 13:')]
swap_command = swap_command.replace('MemoryBlock__SetSwapSettings(', 'ipcSwapSettings(').replace('MemoryBlock__ResetSwapSettings(', 'ipcReset(')
command_prefix = r'''
static struct { char path[256]; u8 guard[16]; } pathStorage;
#define g_path pathStorage.path
#define IPC_BUFFER_R 2
#define IPC_Desc_Buffer(n,d) (((n) << 4) | (d))
#define IPC_MakeHeader(c,n,t) (((c) << 16) | ((n) << 6) | (t))
#define MAKERESULT(...) (-1)
static bool isN3DS = true;
static unsigned ipcCalls;
static bool ipcFail;
static u32 ipcArgs[4];
static Result ipcSwapSettings(u32 *code, bool isLoad, u32 *arguments)
{
    assert((u32)(uintptr_t)code == (isLoad ? 0xA0000200u : 0xA0000100u));
    assert(arguments == ipcArgs);
    assert(isLoad == (ipcCalls == 1));
    ++ipcCalls;
    return ipcFail ? -1 : 0;
}
static void ipcReset(void) { PluginLoaderCtx.isSwapFunctionset = false; }
static void error(uintptr_t *command, u32 result) { command[1] = result; }
static void svcInvalidateEntireInstructionCache(void) {}
'''
command_function = r'''
static void handleSwapCommand(uintptr_t *cmdbuf)
{
    Context *ctx = &PluginLoaderCtx;
    switch(cmdbuf[0] >> 16)
    {
'''
command_suffix = r'''
    }
}
static void verifyPathGuard(void)
{
    for(unsigned i = 0; i < sizeof(pathStorage.guard); ++i) assert(pathStorage.guard[i] == 0xA5);
}
static void testPluginPaths(void)
{
    const char *directory = "/luma/plugins/0004000000155100/";
    char name[256];
    memset(&pathStorage, 0xA5, sizeof(pathStorage));
    strcpy(g_path, directory);
    u32 capacity = sizeof(g_path) - strlen(g_path) - 1;
    memset(name, 'A', capacity); name[capacity] = 0;
    assert(PluginPathCanAppend(capacity) && AppendPluginPath(name));
    assert(strlen(g_path) == sizeof(g_path) - 1); verifyPathGuard();
    strcpy(g_path, directory); name[capacity] = 'A'; name[capacity + 1] = 0;
    assert(!PluginPathCanAppend(capacity + 1) && !AppendPluginPath(name));
    assert(!strcmp(g_path, directory)); verifyPathGuard();
    memset(name, 'B', sizeof(name) - 1); name[sizeof(name) - 1] = 0;
    assert(!PluginPathCanAppend(strlen(name)) && !AppendPluginPath(name));
    assert(!strcmp(g_path, directory)); verifyPathGuard();
    for(u32 i = 0; i < 73; ++i) memcpy(name + i * 3, "\xE3\x81\x82", 3);
    memcpy(name + 73 * 3, "A.3gx", 6);
    assert(strlen(name) == capacity && PluginPathCanAppend(strlen(name)) && AppendPluginPath(name));
    verifyPathGuard(); strcpy(g_path, directory);
    memcpy(name + 73 * 3, "\xE3\x81\x82" "A.3gx", 9);
    assert(!PluginPathCanAppend(strlen(name)) && !AppendPluginPath(name));
    assert(!strcmp(g_path, directory)); verifyPathGuard();
}
static void testIgnoredSwapPaths(void)
{
    const char *paths[] = {"/boot.firm", "/private/movable.sed", "/luma/plugins/custom.swap", ""};
    for(unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
    {
        uintptr_t command[] = {IPC_MakeHeader(12, 2, 4), 0xA0000100u, 0xA0000200u,
            IPC_Desc_Buffer(16, IPC_BUFFER_R), (uintptr_t)ipcArgs,
            IPC_Desc_Buffer(strlen(paths[i]) + 1, IPC_BUFFER_R), (uintptr_t)paths[i]};
        ipcCalls = 0; handleSwapCommand(command);
        assert(command[0] == IPC_MakeHeader(12, 1, 0) && command[1] == 0 && ipcCalls == 2);
        assert(!strcmp(g_swapFileName, "/luma/plugins/.swap"));
    }
    char unterminated[256]; memset(unterminated, 'X', sizeof(unterminated));
    uintptr_t command[] = {IPC_MakeHeader(12, 2, 4), 0xA0000100u, 0xA0000200u,
        IPC_Desc_Buffer(16, IPC_BUFFER_R), (uintptr_t)ipcArgs,
        IPC_Desc_Buffer(sizeof(unterminated), IPC_BUFFER_R), (uintptr_t)unterminated};
    ipcCalls = 0; handleSwapCommand(command);
    assert(command[1] == 0 && ipcCalls == 2 && !strcmp(g_swapFileName, "/luma/plugins/.swap"));
    command[0] = IPC_MakeHeader(12, 2, 4); command[1] = 0xA0000100u; command[6] = 0;
    ipcCalls = 0; handleSwapCommand(command);
    assert((int32_t)command[1] < 0 && !ipcCalls);
    command[0] = IPC_MakeHeader(12, 2, 4); command[1] = 0xA0000100u; command[6] = (uintptr_t)unterminated;
    ipcCalls = 0; ipcFail = true; handleSwapCommand(command); ipcFail = false;
    assert((int32_t)command[1] < 0 && ipcCalls == 1 && !PluginLoaderCtx.isSwapFunctionset);
}
'''
tests = r'''
int main(void)
{
    testPluginPaths();
    testIgnoredSwapPaths();
    u8 buffer[128] = {0};
    g_memBlockSize = sizeof(buffer);
    PluginLoaderCtx.memblock.memblock = buffer;
    u32 code[32] = {0xE92D40C0,0xE3A07000,0xE4906004,0xE0877006,0xE1500001,0x1AFFFFFB,0xE1A00007,0xE8BD80C0,0xE320F000};
    u32 args[4] = {0};
    MemoryBlock__ResetSwapSettings();
    assert(!MemoryBlock__SetSwapSettings(code, false, args));
    assert(!PluginLoaderCtx.isSwapFunctionset);
    assert(!MemoryBlock__SetSwapSettings(code, true, args) && PluginLoaderCtx.isSwapFunctionset);
    for(u32 i = 0; i < sizeof(buffer); ++i) buffer[i] = i;
    assert(!MemoryBlock__ToSwapFile() && opens == 0 && swapDigestValid);
    memset(buffer, 0, sizeof(buffer));
    assert(!MemoryBlock__FromSwapFile() && opens == 0);
    for(u32 i = 0; i < sizeof(buffer); ++i) assert(buffer[i] == i);
    disk[10] ^= 1;
    assert(MemoryBlock__FromSwapFile() < 0 && opens == 0 && reboots == 1);
    disk[10] ^= 1; shortRead = true;
    assert(MemoryBlock__FromSwapFile() < 0 && opens == 0 && reboots == 2);
    shortRead = false; --diskSize;
    assert(MemoryBlock__FromSwapFile() < 0 && opens == 0 && reboots == 3);
    failWrite = true;
    assert(MemoryBlock__ToSwapFile() < 0 && opens == 0 && !swapDigestValid);
    failWrite = false;
    assert(MemoryBlock__FromSwapFile() < 0 && opens == 0);
    memset(code, 0, sizeof(code));
    assert(MemoryBlock__SetSwapSettings(code, false, args) < 0);
    MemoryBlock__ResetSwapSettings();
    assert(!swapDigestValid && !PluginLoaderCtx.isSwapFunctionset);
    puts("Plugin security: bounded UTF-8 plugin paths, fixed swap path and IPC forwarding, atomic swap settings, tamper, short-read and failed-write rejection passed");
}
'''
source = generated / 'security.c'
source.write_text(prefix + globals_ + swap + settings + command_prefix + path_helpers + command_function + swap_command + command_suffix + tests, encoding='utf-8')
binary = generated / ('security.exe' if os.name == 'nt' else 'security')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-I', str(generated), '-I', str(module / 'include'), str(source), str(module / 'source/sha256.c'), str(module / 'source/plugin/payload_vm.c'), '-o', str(binary)], check=True, env=env)
subprocess.run([str(binary)], check=True, env=env)
assert 'PA_FROM_VA_PTR' not in (module / 'source/plugin/3gx.c').read_text()
assert 'PA_FROM_VA_PTR(isLoad' not in memory
assert '!PluginPathCanAppend((u32)units)' in loader and '!AppendPluginPath(chosen)' in loader
assert 'strcat(g_path, chosen)' not in loader
assert 'g_swapFileName' not in swap_command
assert 'const char g_swapFileName[] = "/luma/plugins/.swap";' in memory
