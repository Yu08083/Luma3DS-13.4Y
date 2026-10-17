import os
import re
import shutil
import struct
import subprocess
import zlib
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'tests/generated/online-bridge'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
memory = (module / 'source/plugin/memoryblock.c').read_text(encoding='utf-8')
bridge = memory[memory.index('static LightLock onlineBridgeLock'):memory.index('static void hashSwap')]
mount = memory[memory.index('Result     MemoryBlock__MountInProcess'):memory.index('Result MemoryBlock__SetSwapSettings')]
menu = (module / 'source/menus/online_bridge.c').read_text(encoding='utf-8')
menu = '\n'.join(line for line in menu.splitlines() if not line.startswith('#include'))
lang = (module / 'source/menus/lang.c').read_text(encoding='utf-8')
labels = lang[lang.index('typedef struct { const char *english;'):lang.index('\nconst char *Lang(u32 id)')]
for name in ('MemoryBlock__Free', 'MemoryBlock__ToSwapFile', 'MemoryBlock__UnmountFromProcess', 'MemoryBlock__MountInProcess'):
    body = memory[memory.index(name + '(void)'):]
    assert body.index('onlineBridgeClear();') < body.index('\n}')
menu_entry = (module / 'source/menus.c').read_text(encoding='utf-8')
assert 'RosalinaMenu_OnlineBridge' in menu_entry and '#include "menus/online_bridge.h"' in menu_entry

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "plugin/online_bridge.h"
typedef uint32_t u32;
typedef uint32_t Handle;
typedef int32_t Result;
typedef int LightLock;
typedef struct { const char *message; Result code; } Error;
typedef struct { u32 exeSize, heapVA, heapSize; } PluginHeader;
typedef struct { void *memblock; } MemoryBlock;
typedef struct { u32 target; Error error; PluginHeader header; MemoryBlock memblock; bool isMemPrivate; } Context;
static Context PluginLoaderCtx;
static u32 g_memBlockSize;
static unsigned lockDepth, flushes, mapCalls, failMap, unmapCalls;
static const void *flushed;
static size_t flushedSize;
#define R_FAILED(r) ((r) < 0)
#define R_SUCCEEDED(r) ((r) >= 0)
#define CUR_PROCESS_HANDLE 0xFFFF8001u
#define MAPEXFLAGS_PRIVATE 1
static void LightLock_Lock(LightLock *lock) { assert(*lock == 1 && !lockDepth); *lock = 0; ++lockDepth; }
static void LightLock_Unlock(LightLock *lock) { assert(*lock == 0 && lockDepth == 1); *lock = 1; --lockDepth; }
static void svcFlushDataCacheRange(const void *data, size_t size) { assert(lockDepth == 1); flushed = data; flushedSize = size; ++flushes; }
static Result svcMapProcessMemoryEx(u32 target, u32 address, u32 current, u32 source, u32 size, u32 flags) {
    assert(lockDepth == 0 && target == 17 && current == CUR_PROCESS_HANDLE && flags <= 1);
    assert(source && size && (address == 0x07000000 || address == PluginLoaderCtx.header.heapVA));
    ++mapCalls; return failMap == mapCalls ? -77 : 0;
}
static Result svcUnmapProcessMemoryEx(u32 target, u32 address, u32 size) {
    assert(lockDepth == 0 && target == 17 && size && (address == 0x07000000 || address == PluginLoaderCtx.header.heapVA));
    ++unmapCalls; return 0;
}
#define KEY_A 1
#define KEY_B 2
#define LANG_EN 0
#define LANG_JP 1
static unsigned g_language;
static bool g_langLoaded = true, menuShouldExit;
static void Lang_Load(void) { g_langLoaded = true; }
static u32 g_accentColor = 0xFFFF, g_textColor = 0x1234;
static char frames[24][2048];
static unsigned frameCount, inputIndex, drawDepth;
static u32 inputs[24];
static unsigned actions[24];
static OnlineBridgeMailbox *activeMailbox;
static unsigned width(const char *text) {
    unsigned size = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p)
        if (*p < 128) size += 6;
        else if ((*p & 0xC0) != 0x80) size += 8;
    return size;
}
static void Draw_Lock(void) { assert(!lockDepth && !drawDepth++); }
static void Draw_Unlock(void) { assert(drawDepth-- == 1); }
static void Draw_ClearFramebuffer(void) { assert(drawDepth == 1 && frameCount < 24); frames[frameCount][0] = 0; }
static void Draw_DrawString(int x, int y, u32 color, const char *text) {
    assert(drawDepth == 1 && x >= 0 && y >= 0 && y + 11 <= 240 && x + width(text) <= 320);
    assert(color == g_textColor || color == g_accentColor);
    size_t used = strlen(frames[frameCount]);
    assert(used + strlen(text) + 24 < sizeof(frames[frameCount]));
    snprintf(frames[frameCount] + used, sizeof(frames[frameCount]) - used, "%d:%d:%s\n", x, y, text);
}
static void Draw_DrawFormattedString(int x, int y, u32 color, const char *format, ...) {
    char text[256]; va_list args; va_start(args, format); vsnprintf(text, sizeof(text), format, args); va_end(args);
    Draw_DrawString(x, y, color, text);
}
static void Draw_FlushFramebuffer(void) { assert(drawDepth == 1); ++frameCount; }
static u32 waitInputWithTimeout(u32 timeout);
'''

suffix = r'''
static u32 waitInputWithTimeout(u32 timeout) {
    assert(timeout == 100 && inputIndex < 24 && !drawDepth && !lockDepth);
    unsigned i = inputIndex++;
    if(actions[i] == 1) { activeMailbox->state = 2; activeMailbox->acknowledgedSeq = activeMailbox->requestSeq; }
    if(actions[i] == 2) onlineBridgeClear();
    if(actions[i] == 3) activeMailbox->magic[0] ^= 1;
    return inputs[i];
}
static OnlineBridgeMailbox initial(void) {
    OnlineBridgeMailbox value = {0};
    value.magic[0] = 0x3635324C; value.magic[1] = 0x3158424D;
    value.abi = 1; value.bytes = sizeof(value);
    return value;
}
static void resetUi(unsigned language) {
    memset(inputs, 0, sizeof(inputs)); memset(actions, 0, sizeof(actions)); memset(frames, 0, sizeof(frames));
    frameCount = inputIndex = 0; menuShouldExit = false; g_language = language;
}
static void testFind(void) {
    size_t page = 4096, usable = 0x5000;
    uint8_t *region;
#ifdef _WIN32
    region = VirtualAlloc(NULL, usable + 2 * page, MEM_RESERVE, PAGE_NOACCESS);
    assert(region && VirtualAlloc(region + page, usable, MEM_COMMIT, PAGE_READWRITE));
#else
    region = mmap(NULL, usable + 2 * page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(region != MAP_FAILED && !mprotect(region + page, usable, PROT_READ | PROT_WRITE));
#endif
    uint8_t *end = region + page + usable;
    assert(!OnlineBridge_Find(NULL, 0x1000) && !OnlineBridge_Valid(NULL));
    for(uint32_t size = 0; size <= usable; size += 4) {
        uint8_t *base = end - size;
        memset(base, 0xA7, size);
        assert(!OnlineBridge_Find(base, size));
        if(size < 0x140) continue;
        uint32_t offset = (size - sizeof(OnlineBridgeMailbox)) & ~15u;
        OnlineBridgeMailbox value = initial(); memcpy(base + offset, &value, sizeof(value));
        assert(OnlineBridge_Find(base, size) == (void *)(base + offset));
        assert(!OnlineBridge_Find(base + 1, size - 1));
        for(size_t field = 0; field < 4; ++field) {
            ((uint32_t *)(base + offset))[field] ^= 1;
            assert(!OnlineBridge_Find(base, size));
            ((uint32_t *)(base + offset))[field] ^= 1;
        }
        uint8_t saved[64]; memcpy(saved, base + offset, sizeof(saved));
        assert(OnlineBridge_Find(base, size)); assert(!memcmp(saved, base + offset, sizeof(saved)));
    }
    memset(region + page, 0, usable);
    OnlineBridgeMailbox value = initial();
    memcpy(region + page + 0x100, &value, sizeof(value));
    assert(!OnlineBridge_Find(region + page, usable));
    memcpy(region + page + usable - 0x1000, &value, sizeof(value));
    assert(OnlineBridge_Find(region + page, usable) == (void *)(region + page + usable - 0x1000));
#ifdef _WIN32
    assert(VirtualFree(region, 0, MEM_RELEASE));
#else
    assert(!munmap(region, usable + 2 * page));
#endif
}
static void testApi(void) {
    struct { uint8_t before[16]; OnlineBridgeMailbox box; uint8_t after[16]; } guarded;
    memset(&guarded, 0xA5, sizeof(guarded)); guarded.box = initial();
    OnlineBridgeStatus status, original; memset(&status, 0x6B, sizeof(status)); original = status;
    assert(!MemoryBlock__OnlineBridgeGet(NULL));
    assert(!MemoryBlock__OnlineBridgeGet(&status) && !memcmp(&status, &original, sizeof(status)));
    assert(!MemoryBlock__OnlineBridgeSet(true) && flushes == 0);
    onlineBridge = &guarded.box;
    assert(MemoryBlock__OnlineBridgeGet(&status) && status.requestSeq == 0 && !status.requestedEnabled);
    guarded.box.requestSeq = guarded.box.acknowledgedSeq = UINT32_MAX;
    guarded.box.state = 2; guarded.box.error = 0x12345678;
    uint32_t reserved[7]; memcpy(reserved, guarded.box.reserved, sizeof(reserved));
    assert(MemoryBlock__OnlineBridgeSet(true));
    assert(guarded.box.requestSeq == 0 && guarded.box.requestedEnabled == 1 && guarded.box.acknowledgedSeq == UINT32_MAX);
    assert(guarded.box.state == 2 && guarded.box.error == 0x12345678 && !memcmp(reserved, guarded.box.reserved, sizeof(reserved)));
    assert(flushes == 1 && flushed == &guarded.box && flushedSize == 64);
    assert(MemoryBlock__OnlineBridgeGet(&status) && status.requestSeq != status.acknowledgedSeq);
    assert(MemoryBlock__OnlineBridgeSet(false) && guarded.box.requestSeq == 1 && !guarded.box.requestedEnabled);
    guarded.box.abi = 2; unsigned count = flushes;
    assert(!MemoryBlock__OnlineBridgeGet(&status) && !MemoryBlock__OnlineBridgeSet(true) && flushes == count);
    for(size_t n = 0; n < 16; ++n) assert(guarded.before[n] == 0xA5 && guarded.after[n] == 0xA5);
    onlineBridgeClear(); assert(!onlineBridge && !lockDepth && onlineBridgeLock == 1);
}
static void testMappings(void) {
    _Alignas(16) uint8_t image[0x2000]; memset(image, 0, sizeof(image));
    OnlineBridgeMailbox value = initial(); memcpy(image + sizeof(image) - 64, &value, 64);
    PluginLoaderCtx.target = 17; PluginLoaderCtx.memblock.memblock = image;
    PluginLoaderCtx.header.exeSize = sizeof(image); PluginLoaderCtx.header.heapVA = 0x06000000; PluginLoaderCtx.header.heapSize = 4096;
    g_memBlockSize = sizeof(image);
    assert(!MemoryBlock__MountInProcess() && onlineBridge == (void *)(image + sizeof(image) - 64));
    assert(!MemoryBlock__UnmountFromProcess() && !onlineBridge && unmapCalls == 2);
    assert(!MemoryBlock__MountInProcess() && onlineBridge);
    failMap = mapCalls + 1;
    assert(MemoryBlock__MountInProcess() == -77 && !onlineBridge);
    failMap = 0; assert(!MemoryBlock__MountInProcess() && onlineBridge);
    failMap = mapCalls + 2; assert(MemoryBlock__MountInProcess() == -77 && !onlineBridge);
    failMap = 0; g_memBlockSize = sizeof(image) - 1;
    assert(!MemoryBlock__MountInProcess() && !onlineBridge);
    g_memBlockSize = sizeof(image); image[sizeof(image) - 64] ^= 1;
    assert(!MemoryBlock__MountInProcess() && !onlineBridge);
}
static void testUi(void) {
    OnlineBridgeMailbox value = initial(); activeMailbox = &value; onlineBridge = &value;
    resetUi(LANG_EN); inputs[0] = KEY_A; inputs[1] = KEY_A; inputs[2] = KEY_B;
    RosalinaMenu_OnlineBridge();
    assert(frameCount == 3 && value.requestSeq == 2 && !value.requestedEnabled);
    assert(strstr(frames[0], "Requested: OFF") && !strstr(frames[0], "Close the menu to apply."));
    assert(strstr(frames[1], "Requested: ON") && strstr(frames[1], "Close the menu to apply."));
    assert(strstr(frames[2], "Requested: OFF") && strstr(frames[2], "Close the menu to apply."));
    value = initial(); resetUi(LANG_JP); inputs[0] = KEY_A; inputs[1] = KEY_B;
    RosalinaMenu_OnlineBridge();
    assert(frameCount == 2 && strstr(frames[0], "オンライン接続") && strstr(frames[0], "設定: 無効"));
    assert(strstr(frames[1], "設定: 有効") && strstr(frames[1], "メニューを閉じると適用します。"));
    value.state = 1; resetUi(LANG_JP); actions[0] = 1; inputs[1] = KEY_B;
    RosalinaMenu_OnlineBridge();
    assert(frameCount == 2 && strstr(frames[0], "接続中...") && strstr(frames[1], "75:接続状態: 有効"));
    assert(!strstr(frames[1], "メニューを閉じると適用します。"));
    value.state = 0xFFFFFFFF; value.error = 0xBADF00D;
    resetUi(LANG_EN); inputs[0] = KEY_B; RosalinaMenu_OnlineBridge();
    assert(strstr(frames[0], "75:State: Error") && strstr(frames[0], "0x0badf00d"));
    onlineBridgeClear(); unsigned count = flushes;
    resetUi(LANG_JP); inputs[0] = KEY_A; inputs[1] = KEY_B; RosalinaMenu_OnlineBridge();
    assert(frameCount == 1 && flushes == count && strstr(frames[0], "対応プラグインが読み込まれていません。"));
    onlineBridge = &value; resetUi(LANG_EN); actions[0] = 2; inputs[0] = KEY_A; inputs[1] = KEY_B;
    RosalinaMenu_OnlineBridge();
    assert(frameCount == 2 && flushes == count && strstr(frames[1], "Companion plugin is not loaded."));
    onlineBridge = &value; resetUi(LANG_EN); actions[0] = 3; inputs[1] = KEY_B;
    RosalinaMenu_OnlineBridge();
    assert(frameCount == 2 && strstr(frames[1], "Companion plugin is not loaded."));
    onlineBridgeClear(); resetUi(LANG_EN); menuShouldExit = true; RosalinaMenu_OnlineBridge(); assert(!frameCount);
    assert(!drawDepth && !lockDepth && onlineBridgeLock == 1);
}
int main(void) {
    assert(sizeof(OnlineBridgeMailbox) == 64);
    testFind(); testApi(); testMappings(); testUi();
    puts("Online bridge: actual footer search, guard pages, invalid headers, canaries, locks, flushes, mappings, seq wrap, EN/JP immediate toggle and pending/apply UI passed");
}
'''
source = generated / 'bridge.c'
source.write_text(prefix + bridge + mount + labels + menu + suffix, encoding='utf-8')
binary = generated / ('bridge.exe' if os.name == 'nt' else 'bridge')
subprocess.run([compiler, '-D_DEFAULT_SOURCE', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-pointer-to-int-cast', '-O2', '-I', str(module / 'include'), str(source), '-o', str(binary)], check=True, env=env)
subprocess.run([str(binary)], check=True, env=env)

draw_source = (module / 'source/draw.c').read_text(encoding='utf-8')
renderer = draw_source[draw_source.index('static u32 drawDirtyFirst'):draw_source.index('u32 Draw_DrawFormattedString(')]
render_prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "text.h"
#include "font.h"
#include "plugin/online_bridge.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240
#define SPACING_X 6
#define SPACING_Y 11
#define GUARD 64
#define KEY_A 1
#define KEY_B 2
#define LANG_EN 0
#define LANG_JP 1
static u16 testFramebuffer[320 * 240 + GUARD * 2];
#define FB_BOTTOM_VRAM_ADDR (testFramebuffer + GUARD)
static u16 g_menuBgColor, g_accentColor = 0xC4FF, g_textColor = 0xFFFF;
static bool g_langLoaded = true, menuShouldExit;
static unsigned g_language, frameNumber, inputNumber;
static OnlineBridgeStatus renderStatus;
static bool renderLoaded = true;
static void Lang_Load(void) { g_langLoaded = true; }
'''
render_adapter = r'''
static void Draw_Lock(void) {}
static void Draw_Unlock(void) {}
static void Draw_ClearFramebuffer(void) { Draw_FillRect(0, 0, 320, 240, g_menuBgColor); }
static void Draw_FlushFramebuffer(void) {
    char name[80]; snprintf(name, sizeof(name), "online-bridge-%s-%u.rgb565", g_language == LANG_JP ? "jp" : "en", frameNumber++);
    FILE *file = fopen(name, "wb"); assert(file);
    assert(fwrite(testFramebuffer + GUARD, sizeof(u16), 320 * 240, file) == 320 * 240); fclose(file);
}
static void Draw_DrawFormattedString(int x, int y, u32 color, const char *format, ...) {
    char text[256]; va_list args; va_start(args, format); vsnprintf(text, sizeof(text), format, args); va_end(args);
    assert(x + Draw_MeasureString(text) <= 320); Draw_DrawString(x, y, color, text);
}
bool MemoryBlock__OnlineBridgeGet(OnlineBridgeStatus *status) { *status = renderStatus; return renderLoaded; }
bool MemoryBlock__OnlineBridgeSet(bool enabled) { renderStatus.requestedEnabled = enabled; ++renderStatus.requestSeq; return true; }
static u32 waitInputWithTimeout(u32 timeout) { assert(timeout == 100); return inputNumber++ == 0 ? KEY_A : KEY_B; }
'''
render_main = r'''
int main(void) {
    for(size_t n = 0; n < sizeof(testFramebuffer) / sizeof(testFramebuffer[0]); ++n) testFramebuffer[n] = 0xA55A;
    for(g_language = LANG_EN; g_language <= LANG_JP; ++g_language) {
        memset(&renderStatus, 0, sizeof(renderStatus)); inputNumber = frameNumber = 0;
        RosalinaMenu_OnlineBridge();
        renderLoaded = false; inputNumber = 1; RosalinaMenu_OnlineBridge();
        renderLoaded = true; renderStatus.state = 3; renderStatus.error = 0xD8A0A01E; inputNumber = 1; RosalinaMenu_OnlineBridge();
        assert(frameNumber == 4);
    }
    for(size_t n = 0; n < GUARD; ++n) assert(testFramebuffer[n] == 0xA55A && testFramebuffer[GUARD + 320 * 240 + n] == 0xA55A);
    puts("Online bridge: native renderer EN/JP off, pending, unavailable and error frames passed");
}
'''
render_path = generated / 'bridge-render.c'
render_path.write_text(render_prefix + labels + renderer + render_adapter + menu + render_main, encoding='utf-8')
render_binary = generated / ('bridge-render.exe' if os.name == 'nt' else 'bridge-render')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-I', str(module / 'include'), str(render_path), str(module / 'source/text.c'), '-o', str(render_binary)], check=True, env=env)
subprocess.run([str(render_binary)], cwd=generated, check=True, env=env)
def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
for path in generated.glob('online-bridge-*.rgb565'):
    data = path.read_bytes()
    rows = bytearray()
    for y in range(240):
        row = bytearray()
        for x in range(320):
            value = struct.unpack_from('<H', data, (x * 240 + 239 - y) * 2)[0]
            row.extend(bytes(((value >> 11) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31)) * 3)
        rows.extend((b'\0' + row) * 3)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 960, 720, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')
    path.with_suffix('.png').write_bytes(png)
