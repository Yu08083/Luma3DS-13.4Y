import os
import shutil
import subprocess
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated'
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "online_id_record.h"
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t Result;
typedef int LightLock;
typedef struct { u64 pos; } IFile;
typedef struct { int type; const void *data; } Path;
#define ARCHIVE_SDMC 0
#define PATH_EMPTY 0
#define PATH_ASCII 1
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define FS_WRITE_FLUSH 1
#define R_FAILED(r) ((r) < 0)
#define R_SUCCEEDED(r) ((r) >= 0)
#define fsMakePath(t,d) ((Path){t,d})
#define KEY_A 1
#define KEY_B 2
static bool g_safeMode, menuShouldExit, exists, failOpen, failWrite, failRandom, sameRandom;
static unsigned char disk[48];
static size_t diskSize, partial;
static int psCalls, randomCalls, psExits, lockDepth, buttonIndex;
static u64 entropy = 0x5ac38e01ab27cd19ULL;
static u32 buttons[4];
static u32 g_accentColor, g_textColor;
static char drawn[4096];
#define terminationRequest menuShouldExit
static void LightLock_Lock(LightLock *lock) { assert(*lock == 1); *lock = 0; assert(++lockDepth == 1); }
static void LightLock_Unlock(LightLock *lock) { assert(*lock == 0); *lock = 1; assert(--lockDepth == 0); }
static Result psInit(void) { ++psCalls; return failRandom ? -1 : 0; }
static void psExit(void) { ++psExits; }
static Result PS_GenerateRandomBytes(void *out, size_t size) {
    assert(size == 8); ++randomCalls;
    if(!sameRandom) entropy += 0x714981071849817ULL;
    memcpy(out, &entropy, size); return 0;
}
static Result IFile_Open(IFile *f, int archive, Path empty, Path path, u32 flags) {
    (void)archive; (void)empty; assert(!strcmp(path.data, ONLINE_ID_PATH));
    if(failOpen || (!exists && !(flags & FS_OPEN_CREATE))) return -1;
    if(flags & FS_OPEN_CREATE) exists = true;
    f->pos = 0; return 0;
}
static Result IFile_Read(IFile *f, u64 *read, void *out, u32 size) {
    *read = f->pos >= diskSize ? 0 : size < diskSize - f->pos ? size : diskSize - f->pos;
    memcpy(out, disk + f->pos, *read); f->pos += *read; return 0;
}
static Result IFile_Write(IFile *f, u64 *written, const void *in, u32 size, u32 flags) {
    assert(flags == FS_WRITE_FLUSH); assert(f->pos + size <= sizeof(disk));
    if(failWrite) { *written = 0; return -1; }
    *written = partial ? partial : size;
    memcpy(disk + f->pos, in, *written);
    if(diskSize < f->pos + *written) diskSize = f->pos + *written;
    return 0;
}
static void IFile_Close(IFile *f) { (void)f; }
static void Draw_Lock(void) {}
static void Draw_Unlock(void) {}
static void Draw_ClearFramebuffer(void) { drawn[0] = 0; }
static void Draw_FlushFramebuffer(void) {}
static void Draw_DrawString(int x, int y, u32 color, const char *text) {
    (void)x; (void)y; (void)color; strcat(drawn, text); strcat(drawn, "\n");
}
static const char *Lang_Translate(const char *text) { return text; }
static u32 waitInput(void) { assert(buttonIndex < 4); return buttons[buttonIndex++]; }
'''
suffix = r'''
static OnlineIdRecord active(void) {
    OnlineIdRecord records[2]; onlineIdRead(records);
    int selected = OnlineId_Select(records); assert(selected >= 0); return records[selected];
}
int main(void) {
    assert(sizeof(OnlineIdRecord) == 24);
    assert(OnlineId_Ensure());
    OnlineIdRecord a = active(); assert(a.generation == 1 && a.value);
    assert(psCalls == 1 && randomCalls == 1 && psExits == 1 && diskSize == 24);
    assert(OnlineId_Ensure() && psCalls == 1 && active().value == a.value);
    assert(OnlineId_Regenerate());
    OnlineIdRecord b = active(); assert(b.generation == 2 && b.value != a.value && diskSize == 48);
    failRandom = true; assert(!OnlineId_Regenerate() && active().value == b.value); failRandom = false;
    sameRandom = true; int calls = randomCalls;
    assert(!OnlineId_Regenerate() && randomCalls == calls + 4 && active().value == b.value);
    sameRandom = false;
    failWrite = true; assert(!OnlineId_Regenerate() && active().value == b.value); failWrite = false;
    partial = 8; assert(!OnlineId_Regenerate() && active().value == b.value); partial = 0;
    assert(OnlineId_Regenerate());
    OnlineIdRecord c = active(); assert(c.generation == 3 && c.value != b.value);
    disk[8] ^= 1; assert(active().value == b.value); disk[8] ^= 1;
    for(size_t i = 0; i < sizeof(c); ++i) {
        OnlineIdRecord copy = c; ((unsigned char *)&copy)[i] ^= 1; assert(!OnlineId_Valid(&copy));
    }
    OnlineIdRecord records[2] = {c, b};
    records[0].generation = 0xffffffffu; records[0].checksum = OnlineId_Checksum(&records[0]);
    records[1].generation = 0; records[1].checksum = OnlineId_Checksum(&records[1]);
    assert(OnlineId_Select(records) == 1);
    g_safeMode = true; calls = psCalls; assert(!OnlineId_Ensure() && !OnlineId_Regenerate() && psCalls == calls); g_safeMode = false;
    failOpen = true; assert(!OnlineId_Regenerate()); failOpen = false;
    buttons[0] = KEY_A; buttons[1] = KEY_B; RosalinaMenu_OnlineId();
    assert(strstr(drawn, "Online ID regenerated."));
    assert(!strstr(drawn, "0x") && !strstr(drawn, "value") && !strstr(drawn, "level256_id"));
    assert(active().generation == 4 && lockDepth == 0);
    puts("Online ID: generation, reuse, rotation, entropy failures, torn writes, checksums, wraparound, hidden UI passed");
}
'''
for version in ['Luma3DS_v13_4']:
    module = root / version / 'sysmodules/rosalina'
    menu = (module / 'source/menus.c').read_text(encoding='utf-8').split('Menu rosalinaMenu = {', 1)[1].split('\n};', 1)[0]
    source = (module / 'source/menus/online_id.c').read_text(encoding='utf-8')
    source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
    path = generated / f'online-id-{version}.c'
    path.write_text(prefix + source + suffix, encoding='utf-8')
    output = path.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(module / 'include'), str(path), '-o', str(output)], check=True, env=env)
    subprocess.run([str(output)], check=True, env=env)
