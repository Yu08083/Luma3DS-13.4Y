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
#include <stdlib.h>
#include <string.h>
#include "theme_image.h"
#include "text.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t Result;
typedef u32 FS_Archive;
typedef u32 Handle;
typedef struct { u16 name[256]; u32 attributes; } FS_DirectoryEntry;
typedef struct { u32 handle; u64 pos; } IFile;
typedef struct { u32 type; const void *data; } Path;
#define KEY_A 1
#define KEY_B 2
#define KEY_X 4
#define KEY_Y 8
#define KEY_UP 16
#define KEY_DOWN 32
#define KEY_LEFT 64
#define KEY_RIGHT 128
#define KEY_L 256
#define KEY_R 512
#define BUTTON_A KEY_A
#define BUTTON_B KEY_B
#define BUTTON_X KEY_X
#define BUTTON_Y KEY_Y
#define BUTTON_UP KEY_UP
#define BUTTON_DOWN KEY_DOWN
#define BUTTON_LEFT KEY_LEFT
#define BUTTON_RIGHT KEY_RIGHT
#define BUTTON_L1 KEY_L
#define BUTTON_R1 KEY_R
#define HID_PAD held
#define ARCHIVE_SDMC 0
#define PATH_EMPTY 0
#define PATH_ASCII 1
#define PATH_UTF16 2
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define FS_WRITE_FLUSH 1
#define MEMOP_ALLOC 1
#define MEMOP_FREE 2
#define MEMOP_REGION_SYSTEM 4
#define MEMPERM_READWRITE 3
#define R_FAILED(r) ((r) < 0)
#define R_SUCCEEDED(r) ((r) >= 0)
#define fsMakePath(t, d) ((Path){t, d})
#define SPACING_Y 11
#define COLOR_RED 1
static bool menuShouldExit, terminationRequest, g_safeMode;
static bool hasConfig, hasTemporary, hasBackup, failWrite, failDecode, failAllocation, failRead, pngImage, failFirstSlot, failRename;
static char config[128], temporaryConfig[128], backupConfig[128];
static u32 held, opens, decodes, clears, rows, allocated, freed, keyIndex, keyCount, imageReads, allocationBytes, allocatedBytes, freedBytes;
static u32 keys[32];
static u16 g_menuBgColor;
static u32 g_accentColor, g_textColor, g_cursorColor;
static u32 colorRefreshes;
void AccentColor_Refresh(void) { ++colorRefreshes; }
static u64 memoryA[0x36000 / 8], memoryB[0x36000 / 8];
static const char *directoryNames[] = {"z.png", "not.txt", "a.JPG", "日本語.bmp"};
static u32 directoryOffset;
static int utf8_to_utf16(u16 *dst, const u8 *src, u32 limit)
{
    const char *cursor = (const char *)src;
    u32 count = 0;
    while(*cursor && count < limit) dst[count++] = (u16)Text_Decode(&cursor);
    return *cursor ? -1 : (int)count;
}
static int utf16_to_utf8(u8 *dst, const u16 *src, u32 limit)
{
    u32 count = 0;
    while(*src)
    {
        u32 value = *src++, length = value < 128 ? 1 : value < 2048 ? 2 : 3;
        if(count + length > limit) return -1;
        if(length == 1) dst[count++] = (u8)value;
        else if(length == 2) { dst[count++] = 0xC0 | (value >> 6); dst[count++] = 0x80 | (value & 63); }
        else { dst[count++] = 0xE0 | (value >> 12); dst[count++] = 0x80 | ((value >> 6) & 63); dst[count++] = 0x80 | (value & 63); }
    }
    return (int)count;
}
static Result FSUSER_OpenArchive(FS_Archive *archive, u32 type, Path path) { (void)type; (void)path; *archive = 1; return 0; }
static Result FSUSER_CloseArchive(FS_Archive archive) { assert(archive == 1); return 0; }
static char *configuration(u32 handle) { assert(handle >= 1 && handle <= 3); return handle == 1 ? config : handle == 2 ? temporaryConfig : backupConfig; }
static u32 configHandle(const char *path)
{
    if(!strcmp(path, "/luma/rosalina_theme.cfg")) return 1;
    if(!strcmp(path, "/luma/rosalina_theme.cfg.tmp")) return 2;
    assert(!strcmp(path, "/luma/rosalina_theme.cfg.bak")); return 3;
}
static bool *configExists(u32 handle) { return handle == 1 ? &hasConfig : handle == 2 ? &hasTemporary : &hasBackup; }
static Result FSUSER_DeleteFile(FS_Archive archive, Path path)
{ assert(archive == 1 && path.type == PATH_ASCII); *configExists(configHandle(path.data)) = false; return 0; }
static Result FSUSER_RenameFile(FS_Archive fromArchive, Path from, FS_Archive toArchive, Path to)
{
    assert(fromArchive == 1 && toArchive == 1 && from.type == PATH_ASCII && to.type == PATH_ASCII);
    u32 source = configHandle(from.data), target = configHandle(to.data);
    if((failRename && source == 2) || !*configExists(source) || *configExists(target)) return -1;
    memmove(configuration(target), configuration(source), strlen(configuration(source)) + 1); *configExists(source) = false; *configExists(target) = true; return 0;
}
static Result FSUSER_CreateDirectory(FS_Archive archive, Path path, u32 attributes) { (void)attributes; assert(archive == 1 && path.type == PATH_ASCII); return 0; }
static Result FSUSER_OpenDirectory(Handle *directory, FS_Archive archive, Path path) { (void)path; assert(archive == 1); *directory = 1; directoryOffset = 0; return 0; }
static Result FSDIR_Read(Handle directory, u32 *read, u32 count, FS_DirectoryEntry *entries)
{
    assert(directory == 1 && count == 4);
    *read = 0;
    while(directoryOffset < 4 && *read < count)
    {
        FS_DirectoryEntry *entry = entries + (*read)++;
        int length = utf8_to_utf16(entry->name, (const u8 *)directoryNames[directoryOffset++], 255);
        assert(length > 0); entry->name[length] = 0; entry->attributes = 0;
    }
    return 0;
}
static Result FSDIR_Close(Handle directory) { assert(directory == 1); return 0; }
static Result IFile_Open(IFile *file, u32 archive, Path empty, Path path, u32 flags)
{
    (void)archive; (void)empty;
    file->pos = 0;
    if(path.type == PATH_ASCII)
    {
        file->handle = configHandle(path.data);
        return flags == FS_OPEN_READ && !*configExists(file->handle) ? -1 : 0;
    }
    assert(path.type == PATH_UTF16);
    const u16 *name = path.data;
    assert(name[0] == '/');
    char utf8[256]; int length = utf16_to_utf8((u8 *)utf8, name, sizeof(utf8) - 1);
    assert(length > 0); utf8[length] = 0;
    assert(!strcmp(utf8, "/3ds/luma/RosaliaTheme/a.JPG") || !strcmp(utf8, "/3ds/luma/RosaliaTheme/z.png") || !strcmp(utf8, "/3ds/luma/RosaliaTheme/日本語.bmp"));
    file->handle = 4; ++opens;
    return 0;
}
static Result IFile_GetSize(IFile *file, u64 *size) { *size = file->handle < 4 ? strlen(configuration(file->handle)) : 100; return 0; }
static Result IFile_Read(IFile *file, u64 *read, void *buffer, u32 size)
{
    *read = size;
    if(failRead) { *read = 0; return -1; }
    if(file->handle < 4) memcpy(buffer, configuration(file->handle), size);
    else
    {
        ++imageReads; memset(buffer, 0, size);
        if(pngImage && !file->pos && size >= 8) memcpy(buffer, "\x89PNG\r\n\x1A\n", 8);
    }
    return 0;
}
static Result IFile_Write(IFile *file, u64 *written, void *buffer, u32 size, u32 flags)
{
    assert(file->handle == 2 && flags == FS_WRITE_FLUSH && size < sizeof(config));
    if(failWrite) { memcpy(temporaryConfig, buffer, size / 2); *written = size / 2; return -1; }
    memcpy(temporaryConfig, buffer, size); *written = size; hasTemporary = true; return 0;
}
static Result FSFILE_SetSize(u32 handle, u64 size) { assert(handle == 2 && size < sizeof(config)); configuration(handle)[size] = 0; return 0; }
static void IFile_Close(IFile *file) { (void)file; }
static Result svcControlMemoryEx(u32 *result, uintptr_t address, u32 zero, u32 size, u32 operation, u32 permissions, bool unsafe)
{
    (void)zero; (void)permissions; assert(unsafe && operation == (MEMOP_ALLOC | MEMOP_REGION_SYSTEM) && (size == 0x26000 || size == 0x36000));
    assert(address == (uintptr_t)memoryA || address == (uintptr_t)memoryB || address == (uintptr_t)memoryA + 0x26000 || address == (uintptr_t)memoryB + 0x26000);
    if(failAllocation || (failFirstSlot && address == (uintptr_t)memoryA)) return -1;
    ++allocated; allocationBytes = size; allocatedBytes += size; *result = 1; return 0;
}
static Result svcControlMemory(u32 *result, uintptr_t address, u32 zero, u32 size, u32 operation, u32 permissions)
{
    (void)address; (void)zero; (void)permissions; assert(operation == MEMOP_FREE && size); ++freed; freedBytes += size; *result = 0; return 0;
}
bool ThemeImage_Decode(const ThemeImageSource *source, uint16_t *pixels, void *work, size_t size)
{
    (void)pixels; assert(size == (pngImage ? THEME_IMAGE_WORK_SIZE : 0)); assert((work != NULL) == pngImage); ++decodes;
    u8 data[10]; assert(source->read(source->context, 0, data, sizeof(data)));
    return !failDecode && !source->cancelled(source->context);
}
static void Draw_Lock(void) {}
static void Draw_Unlock(void) {}
static void Draw_ClearFramebuffer(void) { ++clears; }
static void Draw_FlushFramebuffer(void) {}
static void Draw_FillRect(u32 x, u32 y, u32 w, u32 h, u32 c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
static void Draw_DrawString(u32 x, u32 y, u32 color, const char *text) { (void)x; (void)y; (void)color; (void)text; }
static void Draw_DrawMenuRow(u32 y, u32 c, const char *text, u32 cursor, bool selected, char mark) { (void)y; (void)c; (void)cursor; (void)selected; (void)mark; assert(strlen(text) < 128); ++rows; }
static const char *Lang_Translate(const char *text) { return text; }
static u32 waitInputWithTimeout(u32 timeout) { assert(timeout == 100 && keyIndex < keyCount); return keys[keyIndex++]; }
'''
main = r'''
static void choose(const u32 *input, u32 count)
{
    memcpy(keys, input, count * sizeof(u32)); keyIndex = 0; keyCount = count;
    RosalinaMenu_Theme();
    assert(keyIndex == count);
}
int main(void)
{
    assert(themeNameValid("日本語.PNG") && themeNameValid("x.jpeg"));
    assert(!themeNameValid("../x.png") && !themeNameValid("x\\a.png") && !themeNameValid(".png") && !themeNameValid("x.txt"));
    Theme_EnsureLoaded();
    assert(Theme_GetBackground() == NULL && opens == 0);
    assert(themeList() == 3 && strcmp(themeNames[0], "a.JPG") == 0 && strcmp(themeNames[2], "日本語.bmp") == 0);
    const u32 select[] = { KEY_DOWN, KEY_DOWN, KEY_A };
    choose(select, 3);
    assert(strcmp(config, "日本語.bmp") == 0 && themePixels == (u16 *)memoryA && decodes == 1);
    Theme_EnsureLoaded(); Theme_EnsureLoaded();
    assert(decodes == 1);
    assert(allocationBytes == 0x26000 && imageReads == 1);
    failDecode = true;
    const u32 bad[] = { KEY_UP, KEY_A, KEY_B };
    choose(bad, 3);
    assert(themePixels == (u16 *)memoryA && strcmp(config, "日本語.bmp") == 0);
    failDecode = false; failWrite = true;
    choose(bad, 3);
    assert(themePixels == (u16 *)memoryA && strcmp(themeSelected, "日本語.bmp") == 0);
    failWrite = false; failAllocation = true;
    choose(bad, 3);
    assert(themePixels == (u16 *)memoryA);
    failAllocation = false; failRename = true;
    choose(bad, 3);
    assert(themePixels == (u16 *)memoryA && hasConfig && !hasBackup && !strcmp(config, "日本語.bmp"));
    failRename = false;
    failAllocation = false;
    held = KEY_B;
    choose(bad, 3);
    assert(themePixels == (u16 *)memoryA && strcmp(config, "日本語.bmp") == 0);
    held = 0;
    const u32 different[] = { KEY_UP, KEY_A };
    choose(different, 2);
    assert(themePixels == (u16 *)memoryB && strcmp(config, "z.png") == 0);
    g_safeMode = true;
    assert(Theme_GetBackground() == NULL);
    g_safeMode = false;
    u32 oldClears = clears;
    const u32 navigate[] = { KEY_DOWN, KEY_UP, 0, 0, KEY_B };
    u32 oldRows = rows, oldOpens = opens, oldDecodes = decodes;
    choose(navigate, 5);
    assert(clears == oldClears + 1 && rows == oldRows + 7 && opens == oldOpens && decodes == oldDecodes);
    failWrite = true;
    assert(!Theme_Disable() && themePixels == (u16 *)memoryB);
    failWrite = false;
    const u32 off[] = { KEY_X };
    g_accentColor = 0xA11C; g_textColor = 0xB22D; g_cursorColor = 0xC33E; g_menuBgColor = 0x1234;
    u32 previousRefreshes = colorRefreshes;
    choose(off, 1);
    assert(themePixels == NULL && config[0] == 0 && themeSelected[0] == 0);
    assert(g_accentColor == 0xA11C && g_textColor == 0xB22D && g_cursorColor == 0xC33E && g_menuBgColor == 0x1234);
#if NEWER
    assert(colorRefreshes == previousRefreshes + 1);
#else
    (void)previousRefreshes;
#endif
    assert(allocated >= 3 && freedBytes == allocatedBytes);
    strcpy(config, "日本語.bmp"); hasConfig = true; themeLoaded = false; failRead = true;
    Theme_EnsureLoaded(); assert(!themeLoaded && !themePixels);
    failRead = false; failAllocation = true;
    Theme_EnsureLoaded(); assert(!themeLoaded && !themePixels);
    failAllocation = false; failFirstSlot = true; pngImage = true;
    Theme_EnsureLoaded(); assert(themeLoaded && themePixels == (u16 *)memoryB && allocationBytes == 0x36000);
    failFirstSlot = false; pngImage = false;
    assert(Theme_Disable());
    strcpy(backupConfig, "z.png"); hasBackup = true; hasConfig = false; themeLoaded = false;
    Theme_EnsureLoaded(); assert(themeLoaded && themePixels == (u16 *)memoryA && !strcmp(themeSelected, "z.png"));
    assert(Theme_Disable() && !hasBackup && hasConfig && !config[0]);
    assert(freedBytes == allocatedBytes);
    puts("Theme UTF-8 file listing/sorting, selection, cached loading, safe mode, failed loads/saves, incremental rows and disable: OK");
    return 0;
}
'''
for version in ('Luma3DS_v13_4',):
    base = root / version / 'sysmodules/rosalina'
    code = (base / 'source/menus/theme.c').read_text(encoding='utf-8')
    code = re.sub(r'^#include[^\n]*\n', '', code, flags=re.M)
    code = code.replace('#define THEME_ADDRESS_A 0x0D100000u', '#define THEME_ADDRESS_A ((uintptr_t)memoryA)').replace('#define THEME_ADDRESS_B 0x0D180000u', '#define THEME_ADDRESS_B ((uintptr_t)memoryB)')
    code = code.replace('(u32)themePixels', '(uintptr_t)themePixels').replace('(u32)pixels', '(uintptr_t)pixels').replace('u32 address =', 'uintptr_t address =')
    target = generated / (version + '_theme_menu.c')
    target.write_text('#define NEWER ' + str(int(version == 'Luma3DS_v13_4')) + '\n' + prefix + code + main, encoding='utf-8', newline='\n')
    exe = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-variable', '-O2', '-I' + str(base / 'include'), str(target), str(base / 'source/text.c'), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
