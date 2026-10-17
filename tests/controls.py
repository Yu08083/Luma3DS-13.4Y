import os
import re
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
#include <stdio.h>
#include <string.h>
#include "menu_input.h"
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint64_t u64;
typedef int32_t Result;
typedef struct { u32 handle; u64 pos; } IFile;
#define KEY_A 1
#define KEY_B 2
#define KEY_X 4
#define KEY_UP 8
#define KEY_DOWN 16
#define KEY_LEFT 32
#define KEY_RIGHT 64
#define KEY_SELECT 128
#define KEY_TOUCH 256
#define BUTTON_A KEY_A
#define BUTTON_B KEY_B
#define BUTTON_X KEY_X
#define BUTTON_UP KEY_UP
#define BUTTON_DOWN KEY_DOWN
#define BUTTON_LEFT KEY_LEFT
#define BUTTON_RIGHT KEY_RIGHT
#define ARCHIVE_SDMC 0
#define PATH_EMPTY 0
#define PATH_ASCII 1
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define FS_WRITE_FLUSH 1
#define COLOR_RED 1
#define RGB565(r, g, b) (((r) << 11) | ((g) << 5) | (b))
#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0
#define fsMakePath(type, path) (path)
#define R_FAILED(result) ((result) < 0)
#define R_SUCCEEDED(result) ((result) >= 0)
static bool menuShouldExit, terminationRequest;
static bool hasFile, failWrite;
static char disk[64];
static u32 keyList[16], keyCount, keyIndex, saves;
static void hidSetRepeatParameters(u32 delay, u32 interval) { assert(delay >= 100 && interval >= 25); }
static Result IFile_Open(IFile *file, u32 archive, const char *empty, const char *path, u32 mode)
{
    (void)archive; (void)empty;
    assert(strcmp(path, "/luma/menu.cfg") == 0 || strcmp(path, "/luma/colors.cfg") == 0);
    file->handle = 1;
    return mode == FS_OPEN_READ && !hasFile ? -1 : 0;
}
static Result IFile_Read(IFile *file, u64 *read, void *dst, u32 size)
{
    (void)file;
    *read = strlen(disk) < size ? strlen(disk) : size;
    memcpy(dst, disk, (size_t)*read);
    return 0;
}
static Result IFile_Write(IFile *file, u64 *written, const void *src, u32 size, u32 flags)
{
    (void)file;
    assert(flags == FS_WRITE_FLUSH && size < sizeof(disk));
    if(failWrite) { *written = 0; return -1; }
    memcpy(disk, src, size);
    *written = size;
    return 0;
}
static Result FSFILE_SetSize(u32 handle, u64 size)
{ assert(handle == 1 && size < sizeof(disk)); disk[size] = 0; hasFile = true; ++saves; return 0; }
static void IFile_Close(IFile *file) { (void)file; }
static void Draw_Lock(void) {}
static void Draw_Unlock(void) {}
static void Draw_ClearFramebuffer(void) {}
static void Draw_FlushFramebuffer(void) {}
static void Draw_DrawString(u32 x, u32 y, u32 color, const char *text) { (void)x; (void)y; (void)color; (void)text; }
static void Draw_DrawFormattedString(u32 x, u32 y, u32 color, const char *text, ...) { (void)x; (void)y; (void)color; (void)text; }
static void Draw_DrawCharacter(u32 x, u32 y, u32 color, char c) { (void)x; (void)y; (void)color; (void)c; }
static const char *Lang_Translate(const char *text) { return text; }
static u32 waitInputWithTimeout(u32 timeout)
{ assert(timeout == 100 && keyIndex < keyCount); return keyList[keyIndex++]; }
#if NEWER
typedef struct { u16 px, py; } touchPosition;
#define PRV_Y 182
#define HUE_X 10
#define HUE_Y 35
#define HUE_W 300
#define HUE_H 16
#define SV_X 10
#define SV_Y 58
#define SV_W 200
#define SV_H 118
#define PRE_X 280
#define PRE_Y 58
#define PRE_W 30
#define PRE_N 9
#define PRE_H 13
#define STR_TOUCH_BAR_BOX_SWATCHES_TO_PICK 0
#define SPACING_Y 11
static void hidScanInput(void) { assert(++keyIndex < keyCount); }
static u32 hidKeysDown(void) { return keyList[keyIndex]; }
static u32 hidKeysHeld(void) { return 0; }
static void hidTouchRead(touchPosition *pos) { memset(pos, 0, sizeof(*pos)); }
static void svcSleepThread(int64_t ns) { assert(ns == 20000000); }
static const char *Lang(u32 id) { (void)id; return ""; }
static void AccentDrawBase(u32 h, u32 s, u32 v) { assert(h < 360 && s <= 255 && v <= 255); }
static void SdmcDeleteFile(const char *path)
{ assert(strcmp(path, "/luma/colors.cfg") == 0); memset(disk, 0, sizeof(disk)); hasFile = false; }
static const u16 g_presetHSV[PRE_N][3] = {{0}};
static u32 themeResets;
static bool Theme_Disable(void) { ++themeResets; return true; }
#endif
'''

main = r'''
static void keys(const u32 *list, u32 count)
{
    memcpy(keyList, list, count * sizeof(u32));
    keyCount = count; keyIndex = 0;
    RosalinaMenu_Controls();
    assert(keyIndex == count);
}
int main(void)
{
    MenuControls_Load();
    assert(g_menuRepeatDelay == 220 && g_menuRepeatInterval == 70 && g_menuTransitions);
    controlsLoaded = false; hasFile = true; strcpy(disk, "150 40 0\n");
    MenuControls_Load();
    assert(g_menuRepeatDelay == 150 && g_menuRepeatInterval == 40 && !g_menuTransitions);
    controlsLoaded = false; strcpy(disk, "999999999 999999999 0\n");
    MenuControls_Load();
    assert(g_menuRepeatDelay == 150 && g_menuRepeatInterval == 40 && !g_menuTransitions);
    const u32 change[] = { KEY_RIGHT, KEY_DOWN, KEY_RIGHT, KEY_DOWN, KEY_RIGHT, KEY_A };
    keys(change, sizeof(change) / sizeof(change[0]));
    assert(g_menuRepeatDelay == 220 && g_menuRepeatInterval == 55 && g_menuTransitions);
    assert(strcmp(disk, "220 55 1\n") == 0 && saves == 1);
    const u32 cancel[] = { KEY_LEFT, KEY_B };
    keys(cancel, 2);
    assert(g_menuRepeatDelay == 220 && g_menuRepeatInterval == 55 && saves == 1);
    failWrite = true;
    const u32 fail[] = { KEY_RIGHT, KEY_A, KEY_B };
    keys(fail, 3);
    assert(g_menuRepeatDelay == 220 && g_menuRepeatInterval == 55 && saves == 1);
    failWrite = false;
    const u32 reset[] = { KEY_X, KEY_A };
    keys(reset, 2);
    assert(g_menuRepeatDelay == 220 && g_menuRepeatInterval == 70 && g_menuTransitions && saves == 2);
    MenuInput input;
    MenuInput_SetTiming(100, 25);
    MenuInput_Reset(&input, 0);
    assert(MenuInput_Update(&input, KEY_DOWN, 0, KEY_DOWN) == KEY_DOWN);
    assert(MenuInput_Update(&input, KEY_DOWN, 99, KEY_DOWN) == 0);
    assert(MenuInput_Update(&input, KEY_DOWN, 100, KEY_DOWN) == KEY_DOWN);
    assert(MenuInput_Update(&input, KEY_DOWN, 124, KEY_DOWN) == 0);
    assert(MenuInput_Update(&input, KEY_DOWN, 125, KEY_DOWN) == KEY_DOWN);
    g_colorsLoaded = false;
    strcpy(disk, "180 255 255\n0 0 255\n180 255 255\n0 0 0\n");
    AccentColor_EnsureLoaded();
    assert(g_cursorColor == g_accentColor);
#if NEWER
    assert(g_colH[0] == 264 && g_colS[0] == 99 && g_colH[1] == 264);
#endif
    g_colorsLoaded = false;
    strcpy(disk, "300 128 200\n0 0 255\n180 255 255\n0 0 0\n");
    AccentColor_EnsureLoaded();
    assert(g_cursorColor == g_accentColor && g_colH[2] == 300);
    g_colorsLoaded = false;
    strcpy(disk, "300 128 200\n60 200 150\n180 255 255\n90 150 100\n2\n");
    AccentColor_EnsureLoaded();
    assert(g_cursorColor != g_accentColor && g_colH[2] == 180);
#if NEWER
    g_textColor = 0xFFFF;
    AccentColor_Refresh();
    assert(g_textColor == AccentHsv(60, 200, 150));
    assert(g_accentColor == AccentHsv(300, 128, 200));
    assert(g_menuBgColor == AccentHsv(90, 150, 100));
#endif
#if NEWER
    const u32 colorCancel[] = { KEY_X, KEY_B };
    memcpy(keyList, colorCancel, sizeof(colorCancel));
    keyCount = 2; keyIndex = UINT32_MAX;
    RosalinaMenu_AccentColor();
    assert(g_colH[0] == 300 && g_colH[2] == 180 && saves == 2);
    assert(g_colH[1] == 60 && g_colH[3] == 90 && g_menuBgColor == AccentHsv(90, 150, 100));
    assert(themeResets == 0);
    const u32 colorReset[] = { KEY_X, KEY_A };
    memcpy(keyList, colorReset, sizeof(colorReset));
    keyIndex = UINT32_MAX;
    RosalinaMenu_AccentColor();
    for(u32 i = 0; i < 3; ++i)
        assert(g_colH[i] == 264 && g_colS[i] == 99 && g_colV[i] == 255);
    assert(g_colH[3] == 0 && g_colS[3] == 0 && g_colV[3] == 0);
    assert(themeResets == 0);
    assert(g_accentColor == COLOR_TITLE && g_cursorColor == COLOR_TITLE && g_textColor == COLOR_TITLE && g_menuBgColor == 0);
    assert(strcmp(disk, "264 99 255\n264 99 255\n264 99 255\n0 0 0\n2\n") == 0);
    hasFile = true;
    g_colorsLoaded = false; g_colH[0] = 0;
    AccentColor_EnsureLoaded();
    assert(g_colH[0] == 264 && g_accentColor == COLOR_TITLE);
#endif
    puts("Menu timing settings load/save/reset/cancel, invalid input, failed writes and configured repeats: OK");
    return 0;
}
'''

for version in ('Luma3DS_v13_4',):
    base = root / version / 'sysmodules/rosalina'
    code = re.sub(r'^#include[^\n]*\n', '', (base / 'source/menus/controls.c').read_text(encoding='utf-8'), flags=re.M)
    colors = (base / 'source/menus.c').read_text(encoding='utf-8')
    globals_code = colors[colors.index('u32 g_accentColor ='):colors.index('#define HUE_X')]
    hsv_code = colors[colors.index('static u16 AccentHsv('):colors.index('static void AccentPixel(')]
    apply_code = colors[colors.index('static void ColorsApply('):colors.index('static void SdmcDeleteFile(')]
    color_menu = colors[colors.index('static void ColorsSave('):colors.index('static u64 Checker_GetForegroundTitleId(')] if version == 'Luma3DS_v13_4' else ''
    title = re.search(r'^#define COLOR_TITLE (.+)$', (base / 'include/draw.h').read_text(encoding='utf-8'), re.M)[1]
    target = generated / (version + '_controls.c')
    target.write_text('#define NEWER ' + str(int(version == 'Luma3DS_v13_4')) + '\n#define COLOR_TITLE ' + title + '\n' + prefix + globals_code + hsv_code + apply_code + color_menu + code + main, encoding='utf-8', newline='\n')
    exe = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-variable', '-O2', '-I' + str(base / 'include'), str(target), str(base / 'source/menu_input.c'), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
