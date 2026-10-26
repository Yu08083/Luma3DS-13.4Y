import os
import shutil
import subprocess
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
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu_input.h"
#include "text.h"
#include "menu_capture_ip.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int64_t s64;
typedef uint64_t u64;
typedef int32_t Result;
enum { MENU_END, METHOD, MENU };
typedef struct Menu Menu;
typedef struct {
    const char *title;
    int action_type;
    union { Menu *menu; void (*method)(void); };
    bool (*visibility)(void);
} MenuItem;
struct Menu { const char *title; u32 nbItems; MenuItem items[32]; };
static Menu rosalinaMenu;
static bool isN3DS = true;
typedef struct { bool noNetwork; } RemotePlayStatus;
static bool RemotePlay_IsRunning(void) { return false; }
static void RemotePlay_GetStatus(RemotePlayStatus *out) { (void)out; assert(false); }
static bool Sleep__IsSleeping(void) { return false; }
#define KEY_A 1
#define KEY_B 2
#define KEY_RIGHT 16
#define KEY_LEFT 32
#define KEY_UP 64
#define KEY_DOWN 128
#define KEY_R 256
#define KEY_L 512
#define BUTTON_A KEY_A
#define BUTTON_B KEY_B
#define BUTTON_RIGHT KEY_RIGHT
#define BUTTON_LEFT KEY_LEFT
#define BUTTON_UP KEY_UP
#define BUTTON_DOWN KEY_DOWN
#define BUTTON_R1 KEY_R
#define BUTTON_L1 KEY_L
#define DIRECTIONAL_KEYS (KEY_RIGHT | KEY_LEFT | KEY_UP | KEY_DOWN)
#define HID_PAD mockHeldKeys()
#define MENU_MAX_ROWS 16
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240
#define SPACING_X 6
#define SPACING_Y 11
#define FORK_PACKAGE_TAG "v1.3"
#define COLOR_RED 0xF800
#define COLOR_WHITE 0xFFFF
#define GET_VERSION_REVISION(v) ((v) & 255)
#define GET_VERSION_MAJOR(v) ((v) >> 24)
#define GET_VERSION_MINOR(v) (((v) >> 16) & 255)
#define SYSCLOCK_ARM11 268123480
#define R_SUCCEEDED(v) ((v) >= 0)
#define R_FAILED(v) ((v) < 0)
static bool menuShouldExit, menuCloseRequested, terminationRequest;
static bool isHidInitialized = true;
static bool g_safeMode, miniSocEnabled;
static u32 g_accentColor, g_textColor, g_cursorColor;
static u16 g_menuBgColor;
static u32 menuScrollOffset;
static u32 testNow, queryCount, systemCount, strings, cursors, clears, flushes;
static bool framebufferCleared;
static u32 blankFlushes;
static u8 batteryLevel, batteryTemperature;
static float batteryVoltage = 4, batteryPercentage = 80;
static bool drawingRoot;
static u32 firstRootItem;
static u32 methodCalls;
typedef struct { u32 time, keys; } Event;
static Event events[64];
static u32 eventCount;
static u32 mockHeldKeys(void)
{
    assert(testNow < 2000);
    u32 held = 0;
    for (u32 i = 0; i < eventCount && events[i].time <= testNow; ++i)
        held = events[i].keys;
    return held;
}
static u32 scanHeldKeys(void) { return mockHeldKeys(); }
static u32 mockDown, mockLastHeld;
static void scanInputHook(void) { u32 held = mockHeldKeys(); mockDown = held & ~mockLastHeld; mockLastHeld = held; }
static u32 convertHidKeys(u32 keys) { return keys; }
static u32 hidKeysDown(void) { return mockDown; }
static u32 hidKeysDownRepeat(void) { return 0; }
static u32 hidKeysHeld(void) { return mockHeldKeys(); }
static uint64_t svcGetSystemTick(void) { return (uint64_t)testNow * (SYSCLOCK_ARM11 / 1000); }
static void svcSleepThread(int64_t ns) { testNow += (u32)(ns / 1000000); }
static void svcGetSystemInfo(s64 *out, u32 type, u32 value)
{
    (void)type;
    ++systemCount;
    *out = value == 0 ? (NEWER ? 0x0D040000 : 0x09010000) : value == 1 ? 42 : 1;
}
static void hidSetRepeatParameters(u32 a, u32 b) { (void)a; (void)b; }
static void Draw_Lock(void) {}
static void Draw_Unlock(void) {}
static void Draw_ClearFramebuffer(void) { ++clears; framebufferCleared = true; }
static void Draw_FlushFramebuffer(void) { ++flushes; if(framebufferCleared) ++blankFlushes; }
static void AccentColor_EnsureLoaded(void) {}
static void SDSpaceWarnIfLow(void) {}
static void MenuControls_Load(void) {}
static void MenuControls_Apply(void) {}
static const u16 themeTestPixels[320 * 240];
static const u16 *activeBackground;
static void Theme_EnsureLoaded(void) {}
static const u16 *Theme_GetBackground(void) { return themeTestPixels; }
static void Draw_SetMenuBackground(const u16 *pixels) { activeBackground = pixels; }
static u32 miniSocGetCachedHostId(void) { return 0; }
static u32 Draw_MeasureString(const char *text) { return Text_MeasureWidth(text); }
static const char *Lang_Translate(const char *text) { return text; }
static void Draw_DrawString(u32 x, u32 y, u32 color, const char *text)
{
    (void)color;
    ++strings;
    framebufferCleared = false;
    if (x == 10 && y == 10)
        drawingRoot = strcmp(text, "Root") == 0;
    if (drawingRoot && x == 30 && y == 30)
        firstRootItem = (u32)strtoul(text + 4, NULL, 10);
}
static void Draw_DrawFormattedString(u32 x, u32 y, u32 color, const char *fmt, ...)
{ (void)x; (void)y; (void)color; (void)fmt; ++strings; }
static void Draw_DrawCharacter(u32 x, u32 y, u32 color, char c)
{ (void)x; (void)y; (void)color; (void)c; ++cursors; }
static u32 rows;
static void Draw_DrawMenuRow(u32 y, u32 color, const char *text, u32 cursorColor, bool selected, char mark)
{
    ++rows;
    Draw_DrawString(30, y, color, text);
    Draw_DrawCharacter(10, y, cursorColor, selected ? '>' : ' ');
    if(mark) Draw_DrawCharacter(304, y, cursorColor, mark);
}
static void Draw_FillRect(u32 x, u32 y, u32 w, u32 h, u32 color)
{ (void)x; (void)y; (void)w; (void)h; (void)color; }
static u32 socGethostid(void) { return 0; }
static u32 gethostid(void) { return 0; }
static Result menuUpdateMcuInfo(void) { ++queryCount; return 0; }
static Result srvIsServiceRegistered(bool *registered, const char *name)
{ (void)name; *registered = true; return 0; }
static Result mcuHwcInit(void) { ++queryCount; return 0; }
static void mcuHwcExit(void) {}
static Result MCUHWC_GetBatteryLevel(u8 *level) { *level = 80; return 0; }
static u32 menuCountItems(const Menu *menu)
{
    u32 n = 0;
    while (n < 32 && menu->items[n].action_type != MENU_END) ++n;
    return n;
}
static bool menuItemIsHidden(const MenuItem *item)
{ return item->visibility && !item->visibility(); }
'''

main = r'''
static void resetCounts(void) { strings = cursors = clears = flushes = rows = 0; }
static bool hidden(void) { return false; }
static void method(void)
{
    assert(framebufferCleared && blankFlushes == 0);
    assert(activeBackground == NULL);
    ++methodCalls;
    testNow = 100;
}
static void setEvents(const Event *list, u32 count)
{
    if (count)
        memcpy(events, list, count * sizeof(Event));
    eventCount = count;
    testNow = 0;
    menuShouldExit = menuCloseRequested = terminationRequest = false;
    mockDown = mockLastHeld = 0;
}

int main(void)
{
    MenuInput input;
    MenuInput_Reset(&input, KEY_L | KEY_DOWN);
    assert(MenuInput_Update(&input, KEY_L | KEY_DOWN, 0, DIRECTIONAL_KEYS) == 0);
    assert(MenuInput_Update(&input, KEY_L | KEY_DOWN, 1000, DIRECTIONAL_KEYS) == 0);
    assert(MenuInput_Update(&input, 0, 1001, DIRECTIONAL_KEYS) == 0);
    assert(MenuInput_Update(&input, KEY_DOWN, 1002, DIRECTIONAL_KEYS) == KEY_DOWN);
    assert(MenuInput_Update(&input, KEY_DOWN, 1221, DIRECTIONAL_KEYS) == 0);
    assert(MenuInput_Update(&input, KEY_DOWN, 1222, DIRECTIONAL_KEYS) == KEY_DOWN);
    assert(MenuInput_Update(&input, KEY_DOWN, 1291, DIRECTIONAL_KEYS) == 0);
    assert(MenuInput_Update(&input, KEY_DOWN, 1292, DIRECTIONAL_KEYS) == KEY_DOWN);
    assert(MenuInput_Update(&input, KEY_DOWN | KEY_A, 1300, DIRECTIONAL_KEYS) == KEY_A);
    assert((MenuInput_Update(&input, KEY_DOWN | KEY_A, 1400, DIRECTIONAL_KEYS) & KEY_A) == 0);
    MenuInput_Reset(&input, KEY_B);
    assert(MenuInput_Update(&input, KEY_B, 1500, DIRECTIONAL_KEYS) == 0);
    MenuInput_Reset(&input, 0);
    assert(MenuInput_Update(&input, KEY_UP, UINT32_MAX - 100, DIRECTIONAL_KEYS) == KEY_UP);
    assert(MenuInput_Update(&input, KEY_UP, 119, DIRECTIONAL_KEYS) == KEY_UP);
    const Event combo[] = { {8, KEY_A}, {12, KEY_A | KEY_B}, {16, KEY_B}, {20, 0} };
    setEvents(combo, sizeof(combo) / sizeof(combo[0]));
    assert(waitComboWithTimeout(50) == (KEY_A | KEY_B));
    assert(testNow == 20);
    setEvents(NULL, 0);
    assert(waitComboWithTimeout(12) == 0 && testNow >= 12);
#if NEWER
    const Event timely[] = { {8, KEY_A}, {16, 0} };
    setEvents(timely, sizeof(timely) / sizeof(timely[0]));
    assert(waitInputWithTimeout(50) == KEY_A && testNow == 8);
    setEvents(NULL, 0);
    assert(waitInputWithTimeout(12) == 0 && testNow >= 12 && testNow <= 16);
    u32 held = UINT32_MAX;
    isHidInitialized = false;
    assert(waitInputWithTimeoutEx(&held, 0) == 0 && held == 0);
    isHidInitialized = true;
#endif
#if !NEWER
    const Event initialHeld[] = { {0, KEY_A}, {8, 0}, {16, KEY_DOWN}, {32, 0} };
    setEvents(initialHeld, sizeof(initialHeld) / sizeof(initialHeld[0]));
    assert(waitInputWithTimeout(50) == KEY_DOWN);
    assert(testNow >= 16 && testNow < 32);
    const Event heldDown[] = { {8, KEY_DOWN}, {400, 0} };
    setEvents(heldDown, sizeof(heldDown) / sizeof(heldDown[0]));
    menuResetInput();
    assert(waitInputWithTimeout(50) == KEY_DOWN && testNow == 8);
    assert(waitInputWithTimeout(300) == KEY_DOWN && testNow == 228);
#endif

    Menu root = { .title = "Root", .nbItems = 24 };
    char titles[24][16];
    for (u32 i = 0; i < 24; ++i)
    {
        snprintf(titles[i], sizeof(titles[i]), "Item%u", i);
        root.items[i] = (MenuItem){ .title = titles[i], .action_type = METHOD };
    }
    MenuDrawState state = {0};
    testNow = menuScrollOffset = queryCount = systemCount = 0;
    menuDraw(&root, 0, false, &state);
    assert(queryCount == 0 && systemCount == 3 && clears == 1);
    assert(activeBackground == NULL);
    assert(strcmp(menuVersionString, NEWER ? FORK_RELEASE_VERSION : "v9.1") == 0);
    resetCounts();
    menuDraw(&root, 1, false, &state);
    assert(cursors == 2 && strings == 0 && clears == 0 && flushes == 1 && queryCount == 0 && systemCount == 3);
    resetCounts();
    menuDraw(&root, 1, false, &state);
    assert(cursors == 0 && strings == 0 && clears == 0 && flushes == 0);
    menuDraw(&root, 1, true, &state);
    assert(queryCount == 1);
    testNow = 999;
    menuDraw(&root, 2, true, &state);
    assert(queryCount == 1);
    testNow = 1000;
    menuDraw(&root, 2, true, &state);
    assert(queryCount == 2);
    testNow = 1500;
    resetCounts();
    menuDraw(&root, 23, false, &state);
    assert(menuScrollOffset == 8 && clears == 0 && rows == 16 && flushes == 1 && queryCount == 2);
    resetCounts();
    menuDraw(&root, 0, false, &state);
    assert(menuScrollOffset == 0 && clears == 0 && rows == 16 && flushes == 1);
    assert(menuMoveSelection(&root, 23, 1, true) == 0);
    assert(menuMoveSelection(&root, 0, -1, true) == 23);
    assert(menuMoveSelection(&root, 0, -16, false) == 0);
    assert(menuMoveSelection(&root, 20, 16, false) == 23);
    assert(menuMoveSelection(&root, 0, 16, false) == 16);
#if NEWER
    root.items[1].visibility = hidden;
    assert(menuMoveSelection(&root, 0, 1, true) == 2);
    root.items[1].visibility = NULL;
#endif
    Menu child = { .title = "Child", .nbItems = 2, .items = { { .title = "One", .action_type = METHOD }, { .title = "Two", .action_type = METHOD } } };
    root.items[18].action_type = MENU;
    root.items[18].menu = &child;
    const Event history[] = { {8, KEY_R}, {16, 0}, {24, KEY_DOWN}, {32, 0}, {40, KEY_DOWN}, {48, 0}, {64, KEY_A}, {72, 0}, {96, KEY_DOWN}, {104, 0}, {128, KEY_B}, {136, 0}, {160, KEY_B}, {168, 0} };
    setEvents(history, sizeof(history) / sizeof(history[0]));
    menuShow(&root);
    assert(firstRootItem == 3);
    root.items[1].method = method;
    const Event returning[] = { {8, KEY_DOWN}, {16, 0}, {40, KEY_A}, {48, 0}, {100, KEY_B}, {140, 0}, {200, KEY_DOWN}, {208, 0}, {240, KEY_B}, {248, 0} };
    setEvents(returning, sizeof(returning) / sizeof(returning[0]));
    resetCounts();
    menuShow(&root);
    assert(methodCalls == 1 && testNow >= 248);
    puts("Menu repeat, paging, selection/scroll restore, held-button suppression, incremental drawing, status throttling: OK");
    return 0;
}
'''

for version in ('Luma3DS_v13_4',):
    newer = version == 'Luma3DS_v13_4'
    base = root / version / 'sysmodules/rosalina'
    source = (base / 'source/menu.c').read_text(encoding='utf-8')
    code = source[source.index('typedef struct {\n    Menu *menu;'):]
    if not newer:
        code = code[:code.index('\nstatic const char *__press_b_to_close')]
    target = generated / ('navigation_' + version + '.c')
    waits = source[source.index('static u32 waitStableButtons('):source.index('static MyThread menuThread;')]
    if newer:
        waits = source[source.index('static u32 waitInputTimed('):source.index('static u32 scanHeldKeys(')] + waits
    release = (base / 'include/menu.h').read_text(encoding='utf8').split('#define FORK_RELEASE_VERSION ', 1)[1].splitlines()[0]
    target.write_text('#define NEWER ' + str(int(newer)) + '\n#define FORK_RELEASE_VERSION ' + release + '\n#define FORK_DISPLAY_VERSION "' + ('v13.4Y' if newer else 'v9.1') + '"\n' + prefix + waits + code + main, encoding='utf-8', newline='\n')
    executable = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-variable', '-O2', '-I' + str(base / 'include'), str(target), str(base / 'source/menu_input.c'), str(base / 'source/text.c'), str(base / 'source/menu_capture_ip.c'), '-o', str(executable)], check=True, env=env)
    subprocess.run([str(executable)], check=True, env=env)
    print(version, 'navigation and rendering verified')
