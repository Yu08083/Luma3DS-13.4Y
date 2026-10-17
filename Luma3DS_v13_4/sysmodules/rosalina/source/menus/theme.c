#include <3ds.h>
#include "menus/theme.h"
#include "menus/lang.h"
#include "theme_image.h"
#include "text.h"
#include "menu.h"
#include "draw.h"
#include "ifile.h"
#include "csvc.h"
#include "memory.h"

#define THEME_DIRECTORY "/3ds/luma/RosaliaTheme"
#define THEME_CONFIG "/luma/rosalina_theme.cfg"
#define THEME_NAME_SIZE 128
#define THEME_MAX_FILES 128
#define THEME_ROWS 14
#define THEME_PIXEL_SIZE 0x26000
#define THEME_ADDRESS_A 0x0D100000u
#define THEME_ADDRESS_B 0x0D180000u

static bool themeLoaded;
static u16 *themePixels;
static char themeSelected[THEME_NAME_SIZE];
static char themeNames[THEME_MAX_FILES][THEME_NAME_SIZE];
static FS_DirectoryEntry themeEntries[4];

static bool themeNameValid(const char *name)
{
    u32 length = strlen(name);
    if(!length || length >= THEME_NAME_SIZE) return false;
    for(u32 i = 0; i < length; ++i)
        if((u8)name[i] < 32 || name[i] == '/' || name[i] == '\\' || name[i] == ':') return false;
    const char *extension = NULL;
    for(const char *p = name; *p; ++p) if(*p == '.') extension = p;
    if(!extension || extension == name) return false;
    char suffix[6];
    u32 n = strlen(extension);
    if(n >= sizeof(suffix)) return false;
    for(u32 i = 0; i <= n; ++i)
        suffix[i] = extension[i] >= 'A' && extension[i] <= 'Z' ? extension[i] + ('a' - 'A') : extension[i];
    return !strcmp(suffix, ".png") || !strcmp(suffix, ".bmp") || !strcmp(suffix, ".jpg") || !strcmp(suffix, ".jpeg");
}

static u32 themeList(void)
{
    FS_Archive archive;
    Handle directory;
    u32 count = 0;
    if(R_FAILED(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) return 0;
    FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, THEME_DIRECTORY), 0);
    if(R_SUCCEEDED(FSUSER_OpenDirectory(&directory, archive, fsMakePath(PATH_ASCII, THEME_DIRECTORY))))
    {
        u32 read = 0;
        while(count < THEME_MAX_FILES && R_SUCCEEDED(FSDIR_Read(directory, &read, 4, themeEntries)) && read)
            for(u32 i = 0; i < read && count < THEME_MAX_FILES; ++i)
            {
                if(themeEntries[i].attributes & 1) continue;
                int length = utf16_to_utf8((u8 *)themeNames[count], themeEntries[i].name, THEME_NAME_SIZE - 1);
                if(length <= 0 || length >= THEME_NAME_SIZE) continue;
                themeNames[count][length] = 0;
                if(themeNameValid(themeNames[count])) ++count;
            }
        FSDIR_Close(directory);
    }
    FSUSER_CloseArchive(archive);
    for(u32 i = 1; i < count; ++i)
    {
        char name[THEME_NAME_SIZE];
        strcpy(name, themeNames[i]);
        u32 j = i;
        while(j && strcmp(themeNames[j - 1], name) > 0)
        {
            strcpy(themeNames[j], themeNames[j - 1]);
            --j;
        }
        strcpy(themeNames[j], name);
    }
    return count;
}

static bool themeRead(void *context, u32 offset, void *buffer, u32 size)
{
    IFile *file = context;
    file->pos = offset;
    u64 read = 0;
    return R_SUCCEEDED(IFile_Read(file, &read, buffer, size)) && read == size;
}

static bool themeCancelled(void *context)
{
    (void)context;
    return menuShouldExit || (HID_PAD & KEY_B);
}

static void themeFree(u16 *pixels, u32 size)
{
    if(pixels)
    {
        u32 unused;
        svcControlMemory(&unused, (u32)pixels, 0, size, MEMOP_FREE, 0);
    }
}

static u16 *themeLoadImage(const char *name)
{
    if(!themeNameValid(name)) return NULL;
    u16 path[256];
    int prefix = utf8_to_utf16(path, (const u8 *)THEME_DIRECTORY "/", 255);
    if(prefix <= 0) return NULL;
    int suffix = utf8_to_utf16(path + prefix, (const u8 *)name, 255 - prefix);
    if(suffix <= 0) return NULL;
    path[prefix + suffix] = 0;
    IFile file;
    if(R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_UTF16, path), FS_OPEN_READ))) return NULL;
    u64 size;
    u16 *candidate = NULL;
    if(R_SUCCEEDED(IFile_GetSize(&file, &size)) && size >= 54 && size <= THEME_IMAGE_MAX_FILE_SIZE)
    {
        u32 address = (u32)themePixels == THEME_ADDRESS_A ? THEME_ADDRESS_B : THEME_ADDRESS_A;
        u32 allocated;
        if(R_SUCCEEDED(svcControlMemoryEx(&allocated, address, 0, THEME_PIXEL_SIZE, MEMOP_ALLOC | MEMOP_REGION_SYSTEM, MEMPERM_READWRITE, true)))
        {
            candidate = (u16 *)address;
            bool valid = false;
            if(R_SUCCEEDED(svcControlMemoryEx(&allocated, address + THEME_PIXEL_SIZE, 0, THEME_IMAGE_WORK_SIZE, MEMOP_ALLOC | MEMOP_REGION_SYSTEM, MEMPERM_READWRITE, true)))
            {
                ThemeImageSource source = { .context = &file, .size = (u32)size, .read = themeRead, .cancelled = themeCancelled };
                valid = ThemeImage_Decode(&source, candidate, (u8 *)candidate + THEME_PIXEL_SIZE, THEME_IMAGE_WORK_SIZE);
                u32 unused;
                svcControlMemory(&unused, address + THEME_PIXEL_SIZE, 0, THEME_IMAGE_WORK_SIZE, MEMOP_FREE, 0);
            }
            if(!valid)
            {
                themeFree(candidate, THEME_PIXEL_SIZE);
                candidate = NULL;
            }
        }
    }
    IFile_Close(&file);
    return candidate;
}

static bool themeSave(const char *name)
{
    IFile file;
    if(R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, THEME_CONFIG), FS_OPEN_CREATE | FS_OPEN_WRITE))) return false;
    u32 length = strlen(name);
    u64 written = 0;
    Result result = IFile_Write(&file, &written, (void *)name, length, FS_WRITE_FLUSH);
    if(R_SUCCEEDED(result) && written == length) result = FSFILE_SetSize(file.handle, length);
    IFile_Close(&file);
    return R_SUCCEEDED(result) && written == length;
}

void Theme_EnsureLoaded(void)
{
    if(themeLoaded || g_safeMode) return;
    themeLoaded = true;
    IFile file;
    if(R_SUCCEEDED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, THEME_CONFIG), FS_OPEN_READ)))
    {
        u64 size = 0, read = 0;
        if(R_SUCCEEDED(IFile_GetSize(&file, &size)) && size < THEME_NAME_SIZE && R_SUCCEEDED(IFile_Read(&file, &read, themeSelected, (u32)size)) && read == size)
            themeSelected[size] = 0;
        else themeSelected[0] = 0;
        IFile_Close(&file);
        if(themeNameValid(themeSelected)) themePixels = themeLoadImage(themeSelected);
        else themeSelected[0] = 0;
    }
}

const u16 *Theme_GetBackground(void)
{
    return g_safeMode ? NULL : themePixels;
}

bool Theme_Disable(void)
{
    if(!themeSave("")) return false;
    themeFree(themePixels, THEME_PIXEL_SIZE);
    themePixels = NULL;
    themeSelected[0] = 0;
    themeLoaded = true;
    return true;
}

static void themeDrawName(u32 y, const char *name, bool selected, char mark)
{
    char display[THEME_NAME_SIZE];
    const char *cursor = name;
    u32 length = 0, width = 0;
    while(*cursor)
    {
        const char *start = cursor;
        u32 codepoint = Text_Decode(&cursor);
        u32 nextWidth = Text_CharacterWidth(codepoint);
        if(width + nextWidth > 264) break;
        u32 bytes = (u32)(cursor - start);
        memcpy(display + length, start, bytes);
        length += bytes;
        width += nextWidth;
    }
    display[length] = 0;
    Draw_DrawMenuRow(y, g_textColor, display, g_cursorColor, selected, mark);
}

void RosalinaMenu_Theme(void)
{
    Theme_EnsureLoaded();
    u32 count = themeList(), selected = 0, scroll = 0;
    for(u32 i = 0; i < count; ++i) if(!strcmp(themeSelected, themeNames[i])) selected = i;
    bool redraw = true, full = true;
    const char *error = NULL;
    while(!menuShouldExit)
    {
        if(selected < scroll) scroll = selected;
        if(selected >= scroll + THEME_ROWS) scroll = selected - THEME_ROWS + 1;
        if(redraw)
        {
            Draw_Lock();
            if(full)
            {
                Draw_ClearFramebuffer();
                Draw_DrawString(10, 10, g_accentColor, Lang_Translate("Background theme"));
                Draw_DrawString(10, 220, g_textColor, Lang_Translate("A: apply  X: off  Y: refresh  B: back"));
            }
            if(count)
            {
                for(u32 i = scroll; i < count && i < scroll + THEME_ROWS; ++i)
                {
                    char mark = 0;
                    if(i == scroll && scroll) mark = '^';
                    if(i == scroll + THEME_ROWS - 1 && i + 1 < count) mark = 'v';
                    themeDrawName(30 + (i - scroll) * SPACING_Y, themeNames[i], i == selected, mark);
                }
            }
            else
            {
                Draw_DrawString(10, 40, g_textColor, Lang_Translate("No theme images found."));
                Draw_DrawString(10, 65, g_textColor, "SD:/3ds/luma/RosaliaTheme/");
                Draw_DrawString(10, 87, g_textColor, "PNG / JPG / BMP (320 x 240)");
            }
            Draw_FillRect(0, 188, 320, 27, g_menuBgColor);
            if(error) Draw_DrawString(10, 188, COLOR_RED, Lang_Translate(error));
            Draw_FlushFramebuffer();
            Draw_Unlock();
            redraw = false;
            full = false;
        }
        u32 pressed = waitInputWithTimeout(100);
        if(pressed & KEY_B) break;
        if(pressed & KEY_X)
        {
            if(Theme_Disable()) return;
            error = "Unable to save settings.";
            redraw = true;
        }
        else if(pressed & KEY_A && count)
        {
            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_DrawString(10, 10, g_accentColor, Lang_Translate("Background theme"));
            Draw_DrawString(10, 40, g_textColor, Lang_Translate("Loading image...  B: cancel"));
            Draw_FlushFramebuffer();
            Draw_Unlock();
            u16 *pixels = themeLoadImage(themeNames[selected]);
            if(pixels && themeSave(themeNames[selected]))
            {
                themeFree(themePixels, THEME_PIXEL_SIZE);
                themePixels = pixels;
                strcpy(themeSelected, themeNames[selected]);
                return;
            }
            error = pixels ? "Unable to save settings." : "Unable to load image.\nPNG / BMP / baseline JPEG, up to 1024 x 1024.";
            themeFree(pixels, THEME_PIXEL_SIZE);
            redraw = full = true;
        }
        else if(pressed & KEY_Y)
        {
            count = themeList();
            selected = scroll = 0;
            error = NULL;
            redraw = full = true;
        }
        else if(count && (pressed & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R)))
        {
            if(pressed & KEY_UP) selected = selected ? selected - 1 : count - 1;
            else if(pressed & KEY_DOWN) selected = (selected + 1) % count;
            else if(pressed & (KEY_LEFT | KEY_L)) selected = selected > THEME_ROWS ? selected - THEME_ROWS : 0;
            else selected = selected + THEME_ROWS < count ? selected + THEME_ROWS : count - 1;
            error = NULL;
            redraw = true;
        }
    }
}
