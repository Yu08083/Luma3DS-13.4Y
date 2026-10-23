#include <3ds.h>
#include "3gx.h"
#include "menus/lang.h"
#include "ifile.h"
#include "utils.h" 
#include "plgloader.h"
#include "fmt.h"
#include "menu.h"
#include "menus.h"
#include "draw.h"
#include "memory.h"
#include "sleep.h"

#define MEMPERM_RW (MEMPERM_READ | MEMPERM_WRITE)
#define MemBlockSize (5*1024*1024)  

typedef struct
{
    Result          code;
    const char *    message;
}   Error;

typedef struct
{
    bool    isEnabled;
    bool    noFlash;
    u32     titleid;
    char    path[256];
    u32     config[32];
}   PluginLoadParameters;

#define HeaderMagic (0x24584733)  

typedef struct
{
    u32             magic;
    u32             version;
    u32             heapVA;
    u32             heapSize;
    u32             pluginSize;
    const char*     pluginPathPA;
    u32             isDefaultPlugin;
    u32             reserved[25];
    u32             config[32];
}   PluginHeader;

static bool         g_isEnabled;
static u8  *        g_memBlock;
static char         g_path[256];
static Handle       g_process = 0;
static u32          g_codeSize;
static u32          g_heapSize;
static Error        g_error;
static PluginLoadParameters    g_userDefinedLoadParameters;

static MyThread     g_pluginLoaderThread;
static u8 ALIGN(8)  g_pluginLoaderThreadStack[0x4000];


void        gamePatchFunc(void);

void        PluginLoader__ThreadMain(void);
MyThread *  PluginLoader__CreateThread(void)
{
    s64 out;

    svcGetSystemInfo(&out, 0x10000, 0x102);
    g_isEnabled = out & 1;
    g_userDefinedLoadParameters.isEnabled = false;

    if(R_FAILED(MyThread_Create(&g_pluginLoaderThread, PluginLoader__ThreadMain, g_pluginLoaderThreadStack, 0x4000, 20, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);
    return &g_pluginLoaderThread;
}

bool        PluginLoader__IsEnabled(void)
{
    return g_isEnabled;
}

void        PluginLoader__MenuCallback(void)
{
    g_isEnabled = !g_isEnabled;
    SaveSettings();
    PluginLoader__UpdateMenu();
}

void        PluginLoader__UpdateMenu(void)
{
    static const char *status[2] =
    {
        "Plugin Loader: [Disabled]",
        "Plugin Loader: [Enabled]"
    };

    rosalinaMenu.items[isN3DS + 1].title = status[g_isEnabled];
}

static Result     MapPluginInProcess(Handle proc, u32 size)
{
    u32     heapsize = MemBlockSize - size;
    Result  res;
    PluginHeader *header = (PluginHeader *)g_memBlock;

    header->heapVA = 0x06000000;
    g_heapSize = header->heapSize = heapsize;
    g_codeSize = size;

    
    svcControlProcess(proc, PROCESSOP_SET_MMU_TO_RWX, 0, 0);

    
    if (R_FAILED((res = svcMapProcessMemoryEx(proc, 0x07000000, CUR_PROCESS_HANDLE, (u32)g_memBlock, size))))
    {
        g_error.message = "Couldn't map plugin memory block";
        g_error.code = res;
        return res;
    }

    
    if (R_FAILED((res = svcMapProcessMemoryEx(proc, 0x06000000, CUR_PROCESS_HANDLE, (u32)g_memBlock + size, heapsize))))
    {
        g_error.message = "Couldn't map heap memory block";
        g_error.code = res;
        goto exit;
    }

    
    memset32(g_memBlock + size, 0, heapsize);

exit:
    return res;
}

static u32     strlen16(const u16 *str)
{
    u32 size = 0;

    while (str && *str++) ++size;
    return size;
}

#define MAX_PLUGIN_ENTRIES 32
#define SELECTION_FILENAME ".selected3gx"
#define COLOR_GRAY RGB565(0x08, 0x10, 0x08)

static char     g_pluginEntries[MAX_PLUGIN_ENTRIES][256];
static bool     g_pluginEntryOk[MAX_PLUGIN_ENTRIES];
static u32      g_pluginEntryCount;
static u32      g_pluginEntryOkCount;
static char     g_pluginDir[300];

static bool     IsPluginLoadable(const char *fileName)
{
    char    path[sizeof(g_path) + sizeof(g_pluginEntries[0])];
    IFile   f;
    bool    ok;

    sprintf(path, "%s%s", g_path, fileName);

    if (R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
        return false;

    ok = R_SUCCEEDED(Check_3gx_Magic(&f));
    IFile_Close(&f);

    return ok;
}

static Result   EnumeratePluginFiles(u64 tid)
{
    u32                 entriesNb = 0;
    Handle              dir = 0;
    Result              res;
    FS_Archive          sdmcArchive = 0;
    FS_DirectoryEntry   entries[10];
    static const u16 *  validExtension = u".3gx";

    g_pluginEntryCount = 0;

    sprintf(g_path, "/luma/plugins/%016llX", tid);

    if (R_FAILED((res = FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))))
        return res;

    if (R_FAILED((res = FSUSER_OpenDirectory(&dir, sdmcArchive, fsMakePath(PATH_ASCII, g_path)))))
    {
        FSUSER_CloseArchive(sdmcArchive);
        return res;
    }

    strcat(g_path, "/");
    strcpy(g_pluginDir, g_path);

    while (R_SUCCEEDED(FSDIR_Read(dir, &entriesNb, 10, entries)) && entriesNb != 0)
    {
        for (u32 i = 0; i < entriesNb && g_pluginEntryCount < MAX_PLUGIN_ENTRIES; ++i)
        {
            FS_DirectoryEntry *entry = &entries[i];

            if (entry->attributes & 1)
                continue;

            u32 size = strlen16(entry->name);
            if (size <= 4)
                continue;

            if (entry->name[0] == u'.')
                continue;

            u16 *fileExt = entry->name + size - 4;
            if (memcmp(fileExt, validExtension, 4 * sizeof(u16)))
                continue;

            int units = utf16_to_utf8((u8 *)g_pluginEntries[g_pluginEntryCount],
                                      entry->name, sizeof(g_pluginEntries[0]) - 1);
            if (units < 0 || (u32)units >= sizeof(g_pluginEntries[0]))
                continue;
            g_pluginEntries[g_pluginEntryCount][units] = 0;
            ++g_pluginEntryCount;
        }
    }

    FSDIR_Close(dir);
    FSUSER_CloseArchive(sdmcArchive);

    for (u32 i = 1; i < g_pluginEntryCount; ++i)
    {
        char    tmp[256];
        s32     j = (s32)i - 1;

        strcpy(tmp, g_pluginEntries[i]);
        while (j >= 0 && strcmp(g_pluginEntries[j], tmp) > 0)
        {
            strcpy(g_pluginEntries[j + 1], g_pluginEntries[j]);
            --j;
        }
        strcpy(g_pluginEntries[j + 1], tmp);
    }

    g_pluginEntryOkCount = 0;
    for (u32 i = 0; i < g_pluginEntryCount; ++i)
    {
        g_pluginEntryOk[i] = IsPluginLoadable(g_pluginEntries[i]);
        if (g_pluginEntryOk[i])
            ++g_pluginEntryOkCount;
    }

    return 0;
}

static bool     LoadSelection(char *outName, u32 outSize)
{
    char    path[300];
    IFile   f;
    u64     total = 0;
    bool    ok = false;

    sprintf(path, "%s%s", g_path, SELECTION_FILENAME);

    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
    {
        memset(outName, 0, outSize);
        if (R_SUCCEEDED(IFile_Read(&f, &total, outName, outSize - 1)) && total > 0)
        {
            for (u32 i = 0; i < (u32)total; ++i)
            {
                if (outName[i] == '\r' || outName[i] == '\n')
                {
                    outName[i] = 0;
                    break;
                }
            }
            ok = outName[0] != 0;
        }
        IFile_Close(&f);
    }

    return ok;
}

static void     SaveSelection(const char *name)
{
    char    path[300];
    IFile   f;
    u64     total;

    sprintf(path, "%s%s", g_path, SELECTION_FILENAME);

    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, path), FS_OPEN_CREATE | FS_OPEN_WRITE)))
    {
        IFile_Write(&f, &total, (void *)name, strlen(name), FS_WRITE_FLUSH);
        FSFILE_SetSize(f.handle, strlen(name));
        IFile_Close(&f);
    }
}

static void     ClearSelection(void)
{
    char        path[300];
    FS_Archive  sdmcArchive = 0;

    sprintf(path, "%s%s", g_path, SELECTION_FILENAME);

    if (R_SUCCEEDED(FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
    {
        FSUSER_DeleteFile(sdmcArchive, fsMakePath(PATH_ASCII, path));
        FSUSER_CloseArchive(sdmcArchive);
    }
}

static s32      NextLoadableEntry(s32 from, s32 step)
{
    if (g_pluginEntryCount == 0)
        return -1;

    for (u32 n = 0; n < g_pluginEntryCount; ++n)
    {
        from = (from + step + (s32)g_pluginEntryCount) % (s32)g_pluginEntryCount;
        if (g_pluginEntryOk[from])
            return from;
    }
    return -1;
}

#define SELECTOR_ROWS       17
#define SELECTOR_NAME_WIDTH ((SCROLL_MARK_X - 10) / SPACING_X - 3)

static bool     ShowPluginSelector(char *outName, u32 outSize)
{
    s32     cursor;
    u32     keys;
    char    pinned[256];
    bool    havePinned;
    u32     first = 0;

    if (g_pluginEntryOkCount == 0)
        return false;

    cursor = g_pluginEntryOk[0] ? 0 : NextLoadableEntry(0, 1);
    if (cursor < 0)
        return false;

    havePinned = LoadSelection(pinned, sizeof(pinned));

    menuEnter();

    do
    {
        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 10, COLOR_TITLE, "Plugin selector");
        Draw_DrawString(10, 30, COLOR_WHITE, Lang(STR_A_USE_X_PIN_UNPIN_B_NONE));

        if (cursor >= 0)
        {
            if ((u32)cursor < first)
                first = cursor;
            else if ((u32)cursor >= first + SELECTOR_ROWS)
                first = cursor - SELECTOR_ROWS + 1;
        }

        u32 posY = 50;
        for (u32 i = first; i < g_pluginEntryCount && i < first + SELECTOR_ROWS; ++i)
        {
            char    name[SELECTOR_NAME_WIDTH + 1];
            char    line[sizeof(name) + 2];
            u32     color;

            strncpy(name, g_pluginEntries[i], sizeof(name) - 1);
            name[sizeof(name) - 1] = 0;

            if (!g_pluginEntryOk[i])
                color = COLOR_GRAY;
            else if (havePinned && strcmp(g_pluginEntries[i], pinned) == 0)
                color = COLOR_YELLOW;
            else
                color = COLOR_WHITE;

            sprintf(line, "%s%s", ((s32)i == cursor) ? "> " : "  ", name);
            Draw_DrawString(10, posY, color, line);
            posY += SPACING_Y;
        }
        Draw_DrawScrollMarks(50, SELECTOR_ROWS, first, g_pluginEntryCount, g_cursorColor);

        Draw_FlushFramebuffer();
        Draw_Unlock();

        keys = waitInput();

        if (terminationRequest)
            break;

        if (keys & BUTTON_DOWN)
            cursor = NextLoadableEntry(cursor, 1);
        else if (keys & BUTTON_UP)
            cursor = NextLoadableEntry(cursor, -1);
        else if (keys & BUTTON_A)
        {
            if (cursor < 0)
                continue;
            strncpy(outName, g_pluginEntries[cursor], outSize - 1);
            outName[outSize - 1] = 0;
            menuLeave();
            return true;
        }
        else if (keys & BUTTON_X)
        {
            if (cursor < 0)
                continue;
            if (havePinned && strcmp(g_pluginEntries[cursor], pinned) == 0)
            {
                ClearSelection();
                havePinned = false;
                pinned[0] = 0;
            }
            else
            {
                strncpy(pinned, g_pluginEntries[cursor], sizeof(pinned) - 1);
                pinned[sizeof(pinned) - 1] = 0;
                SaveSelection(pinned);
                havePinned = true;
            }
        }
        else if (keys & BUTTON_B)
        {
            menuLeave();
            return false;
        }
    }
    while (!terminationRequest);

    menuLeave();
    return false;
}

static bool     ResolvePluginPath(u64 tid)
{
    char    chosen[256];
    char    saved[256];
    bool    haveChoice = false;
    u32     heldKeys;

    if (R_FAILED(EnumeratePluginFiles(tid)) || g_pluginEntryCount == 0)
        return false;

    if (g_pluginEntryOkCount == 0)
        return false;

    heldKeys = HID_PAD;

    if (g_pluginEntryOkCount == 1 && g_pluginEntryCount == 1)
    {
        strcpy(chosen, g_pluginEntries[0]);
        haveChoice = true;
    }
    else if (!(heldKeys & BUTTON_Y) && LoadSelection(saved, sizeof(saved)))
    {
        bool stillUsable = false;

        for (u32 i = 0; i < g_pluginEntryCount; ++i)
        {
            if (strcmp(g_pluginEntries[i], saved) == 0)
            {
                stillUsable = g_pluginEntryOk[i];
                break;
            }
        }

        if (stillUsable)
        {
            strcpy(chosen, saved);
            haveChoice = true;
        }
        else
            haveChoice = ShowPluginSelector(chosen, sizeof(chosen));
    }
    else
        haveChoice = ShowPluginSelector(chosen, sizeof(chosen));

    if (!haveChoice)
        return false;

    strcat(g_path, chosen);
    return true;
}

typedef struct DestructiveKeyword
{
    const char *text;
    u32         weight;
} DestructiveKeyword;

static const DestructiveKeyword g_destructiveKeywords[] =
{
    { "boot9strap",   40 },
    { "boot.firm",    40 },
    { "private",      25 },
    { "cias",         20 },
    { "gm9",          20 },
    { "fbi",          20 },
    { "Nintendo 3DS", 15 },
    { "DCIM",         10 },
    { "JKSV",         10 },
    { "Splashes",     10 },
};
#define DESTRUCTIVE_KEYWORD_COUNT (sizeof(g_destructiveKeywords) / sizeof(g_destructiveKeywords[0]))
#define SCAN_BUFFER_SIZE    0x2000
#define SCAN_CARRY_SIZE     32
#define APPROVED_PATH       "/luma/plugins/.approved3gx"

static char lowerAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static bool     MatchPathKeyword(const u8 *buf, u32 len, u32 pos, const char *keyword, u32 unit, bool last)
{
    for (u32 i = 0; keyword[i] != 0; ++i, pos += unit)
    {
        if (pos + unit > len || lowerAscii((char)buf[pos]) != lowerAscii(keyword[i]) || (unit == 2 && buf[pos + 1] != 0))
            return false;
    }

    if (pos + unit > len)
        return last;

    return (buf[pos] == '/' || buf[pos] == '"' || buf[pos] == 0) && (unit == 1 || buf[pos + 1] == 0);
}

static void     ScanForKeywords(const u8 *buf, u32 len, u32 base, bool last, bool *found, u32 *offsets, u8 *units)
{
    for (u32 s = 0; s + 1 < len; ++s)
    {
        if (buf[s] != '/' && buf[s] != ':')
            continue;

        for (u32 unit = 1; unit <= 2; ++unit)
        {
            if (unit == 2 && buf[s + 1] != 0)
                continue;

            for (u32 i = 0; i < DESTRUCTIVE_KEYWORD_COUNT; ++i)
            {
                if (!found[i] && MatchPathKeyword(buf, len, s + unit, g_destructiveKeywords[i].text, unit, last))
                {
                    found[i] = true;
                    offsets[i] = base + s + unit;
                    units[i] = (u8)unit;
                }
            }
        }
    }
}

#define APPROVED_CACHE_SIZE 32

static u64      g_approvedHashes[APPROVED_CACHE_SIZE];
static u32      g_approvedCount;
static bool     g_approvedLoaded;

static void     LoadApprovals(void)
{
    IFile   f;
    u64     total = 0;

    g_approvedLoaded = true;

    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, APPROVED_PATH), FS_OPEN_READ)))
    {
        if (R_FAILED(IFile_Read(&f, &total, g_approvedHashes, sizeof(g_approvedHashes))))
            total = 0;
        IFile_Close(&f);
    }

    g_approvedCount = (u32)total / sizeof(u64);
}

static void     SaveApprovals(void)
{
    IFile   f;
    u64     total;
    u32     size = g_approvedCount * sizeof(u64);

    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, APPROVED_PATH), FS_OPEN_CREATE | FS_OPEN_WRITE)))
    {
        IFile_Write(&f, &total, g_approvedHashes, size, FS_WRITE_FLUSH);
        FSFILE_SetSize(f.handle, size);
        IFile_Close(&f);
    }
}

static bool     IsHashApproved(u64 hash)
{
    if (!g_approvedLoaded)
        LoadApprovals();

    for (u32 i = 0; i < g_approvedCount; ++i)
        if (g_approvedHashes[i] == hash)
            return true;

    return false;
}

static void     RememberApproval(u64 hash)
{
    if (IsHashApproved(hash))
        return;

    if (g_approvedCount < APPROVED_CACHE_SIZE)
        g_approvedHashes[g_approvedCount++] = hash;
    else
    {
        for (u32 i = 1; i < APPROVED_CACHE_SIZE; ++i)
            g_approvedHashes[i - 1] = g_approvedHashes[i];

        g_approvedHashes[APPROVED_CACHE_SIZE - 1] = hash;
    }

    SaveApprovals();
}

static void     DrawMatchDetails(IFile *plugin, const bool *found, const u32 *offsets, const u8 *units)
{
    static char raw[48];
    char        text[25];
    u64         total;
    u32         posY = 30;

    Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_MATCHED_LOCATIONS));

    for (u32 i = 0; i < DESTRUCTIVE_KEYWORD_COUNT; ++i)
    {
        u32 n = 0;

        if (!found[i])
            continue;

        u32 back = 8 * units[i];
        u32 base = offsets[i] > back ? offsets[i] - back : 0;

        plugin->pos = base;
        if (R_SUCCEEDED(IFile_Read(plugin, &total, raw, 24 * units[i])))
        {
            u32 got = (u32)total / units[i];

            for (n = 0; n < got; ++n)
            {
                char c = raw[n * units[i]];
                text[n] = (c >= 0x20 && c < 0x7F) ? c : '.';
            }
        }
        text[n] = 0;

        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "%08lx %-12s %s\n",
                                        offsets[i], g_destructiveKeywords[i].text, text);
    }
}

static bool     CheckForDestructivePlugin(IFile *plugin, const _3gx_Header *header, u64 fileSize)
{
    static u8   buffer[SCAN_BUFFER_SIZE];
    const u32   ranges[1][2] = { { (u32)header->code, header->codeSize } };
    u64         savedPos = plugin->pos;
    u64         total;
    u64         hash = 0xCBF29CE484222325ULL;
    bool        found[DESTRUCTIVE_KEYWORD_COUNT] = { false };
    u32         matchOffset[DESTRUCTIVE_KEYWORD_COUNT] = { 0 };
    u8          matchUnit[DESTRUCTIVE_KEYWORD_COUNT] = { 0 };
    bool        readFailed = false;

    for (u32 r = 0; r < sizeof(ranges) / sizeof(ranges[0]) && !readFailed; ++r)
    {
        u32 offset = ranges[r][0];
        u32 end = ranges[r][0] + ranges[r][1];
        u32 carry = 0;

        if (end < offset || end > fileSize)
            readFailed = true;

        while (!readFailed && offset < end)
        {
            u32 chunk = end - offset;

            if (chunk > sizeof(buffer) - carry)
                chunk = sizeof(buffer) - carry;

            plugin->pos = offset;
            if (R_FAILED(IFile_Read(plugin, &total, buffer + carry, chunk)) || (u32)total != chunk)
            {
                readFailed = true;
                break;
            }

            for (u32 i = 0; i < chunk; ++i)
            {
                hash ^= buffer[carry + i];
                hash *= 0x100000001B3ULL;
            }

            u32 len = carry + chunk;

            offset += chunk;
            ScanForKeywords(buffer, len, offset - len, offset == end, found, matchOffset, matchUnit);

            carry = len < SCAN_CARRY_SIZE ? len : SCAN_CARRY_SIZE;
            for (u32 i = 0; i < carry; ++i)
                buffer[i] = buffer[len - carry + i];
        }
    }

    plugin->pos = savedPos;

    u32 risk = 0;
    for (u32 i = 0; i < DESTRUCTIVE_KEYWORD_COUNT; ++i)
        if (found[i])
            risk += g_destructiveKeywords[i].weight;

    if (risk > 100)
        risk = 100;

    if (risk == 0 && !readFailed)
        return true;

    if (!readFailed && IsHashApproved(hash))
        return true;

    const char  *warningText = risk <= 20 ? "little dangerous" : risk <= 40 ? "moderately dangerous" : "very dangerous";
    char        textBuf[64];
    u32         keys;
    bool        result = true;
    bool        details = false;

    if (readFailed)
        sprintf(textBuf, "Could not scan the whole file");
    else
        sprintf(textBuf, "Risk : %u%% (%s)", (unsigned)risk, warningText);

    char matched[256];
    matched[0] = 0;
    for (u32 i = 0; i < DESTRUCTIVE_KEYWORD_COUNT; ++i)
    {
        if (found[i])
        {
            if (matched[0] != 0 && (u32)(strlen(matched) + 2) < (u32)sizeof(matched))
                strcat(matched, ", ");
            if ((u32)(strlen(matched) + strlen(g_destructiveKeywords[i].text) + 1) < (u32)sizeof(matched))
                strcat(matched, g_destructiveKeywords[i].text);
        }
    }

    if (matched[0] == 0)
        strcat(matched, "(none)");

    menuEnter();

    do
    {
        Draw_Lock();
        Draw_ClearFramebuffer();

        if (details)
        {
            DrawMatchDetails(plugin, found, matchOffset, matchUnit);
            Draw_DrawString(10, 205, COLOR_WHITE, Lang(STR_X_BACK_TO_THE_WARNING));
        }
        else
        {
            Draw_DrawString(10, 10, COLOR_RED, "--- KEIKOKU / WARNING ---");
            u32 posY = readFailed
                ? Draw_DrawBilingual(30, 30, COLOR_WHITE, "Kono 3gx wa saigo made kensa dekimasen",
                                     "This 3gx could not be fully checked")
                : Draw_DrawBilingual(30, 30, COLOR_WHITE, "Kono 3gx wa hakaiteki kamo shiremasen",
                                     "This 3gx may be a destructive plugin");
            posY = Draw_DrawString(30, posY + SPACING_Y, COLOR_WHITE, textBuf);
            posY = Draw_DrawString(30, posY + SPACING_Y, COLOR_WHITE, "Matched keywords:");
            posY = Draw_DrawString(30, posY + SPACING_Y, COLOR_YELLOW, matched);
            Draw_DrawString(30, posY + SPACING_Y, COLOR_WHITE, "X: basho wo hyouji / show where found");
            Draw_DrawString(10, 205, COLOR_WHITE, "A: sonomama tsukau / use anyway");
            Draw_DrawString(10, 216, COLOR_WHITE, "B: tsukawanai / don't use it");
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();

        keys = waitInput();

        if (keys & BUTTON_X)
            details = !details;
    }
    while (!(keys & (BUTTON_A | BUTTON_B)) && !terminationRequest);

    if (terminationRequest || (keys & BUTTON_B))
        result = false;

    if (result && !readFailed)
        RememberApproval(hash);

    plugin->pos = savedPos;
    menuLeave();
    return result;
}

static Result   OpenFile(IFile *file, const char *path)
{
    return IFile_Open(file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, path), FS_OPEN_READ);
}

static Result   CheckPluginCompatibility(_3gx_Header *header, u32 processTitle)
{
    static char   errorBuf[0x100];
    char          titleName[48];
    char          authorName[48];

    if (header->targets.count == 0)
        return 0;

    for (u32 i = 0; i < header->targets.count; ++i)
    {
        if (header->targets.titles[i] == processTitle)
            return 0;
    }

    strncpy(titleName, header->infos.titleMsg ? header->infos.titleMsg : "", sizeof(titleName) - 1);
    titleName[sizeof(titleName) - 1] = 0;
    strncpy(authorName, header->infos.authorMsg ? header->infos.authorMsg : "", sizeof(authorName) - 1);
    authorName[sizeof(authorName) - 1] = 0;

    sprintf(errorBuf, "The plugin - %s -\nis not compatible with this game.\n" \
                      "Contact \"%s\" for more infos.", titleName, authorName);
    g_error.message = errorBuf;

    return -1;
}
static bool     TryToLoadPlugin(Handle process)
{
    u64     fileSize;
    u64     tid;
    u32     procStart = 0x00100000;
    IFile   plugin;
    PluginHeader    *hdr = (PluginHeader *)g_memBlock;
    _3gx_Header     *header;
    Result  res;

    
    memset32(g_memBlock, 0, MemBlockSize);

    hdr->magic = HeaderMagic;

    
    svcGetProcessInfo((s64 *)&tid, process, 0x10001);
    if (R_FAILED((res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, procStart, process, procStart, 0x1000))))
    {
        g_error.message = "Couldn't map process";
        g_error.code = res;
        return false;
    }

    
    if (g_userDefinedLoadParameters.isEnabled && (u32)tid == g_userDefinedLoadParameters.titleid)
    {
        g_userDefinedLoadParameters.isEnabled = false;
        if (OpenFile(&plugin, g_userDefinedLoadParameters.path))
            goto exitFail;
        hdr->pluginPathPA = PA_FROM_VA_PTR(g_userDefinedLoadParameters.path);
        memcpy(hdr->config, g_userDefinedLoadParameters.config, 32 * sizeof(u32));
    }
    else
    {
        if (!ResolvePluginPath(tid) || OpenFile(&plugin, g_path))
        {
            
            const char *defaultPath = "/luma/plugins/default.3gx";
            if (OpenFile(&plugin, defaultPath))
                goto exitFail;
            hdr->isDefaultPlugin = 1;
            hdr->pluginPathPA = PA_FROM_VA_PTR(defaultPath);
        }
        else
            hdr->pluginPathPA = PA_FROM_VA_PTR(g_path);
    }

    if (R_FAILED((res = IFile_GetSize(&plugin, &fileSize))))
        g_error.message = "Couldn't get file size";

    
    header = (_3gx_Header *)(g_memBlock + MemBlockSize - (u32)fileSize);

    
    if (!res && R_FAILED(res = Check_3gx_Magic(&plugin)))
    {
        static const char *const errors[] =
        {
            "Couldn't read file.",
            "Invalid plugin file\nNot a valid 3GX plugin format!",
            "Outdated plugin file\nCheck for an updated plugin.",
            "Outdated plugin loader\nCheck for Luma3DS updates."
        };

        g_error.message = errors[R_MODULE(res) == RM_LDR ? R_DESCRIPTION(res) : 0];
    }

    
    if (!res && R_FAILED((res = Read_3gx_Header(&plugin, header))))
        g_error.message = "Couldn't read file";

    
    if (!res) res = CheckPluginCompatibility(header, (u32)tid);

    if (!res && !CheckForDestructivePlugin(&plugin, header, fileSize))
    {
        IFile_Close(&plugin);
        goto exitFail;
    }

    
    if (!res && R_FAILED(res = Read_3gx_Code(&plugin, header, g_memBlock + sizeof(PluginHeader))))
        g_error.message = "Couldn't read plugin's code";

    
    IFile_Close(&plugin);
    if (R_FAILED(res))
    {
        g_error.code = res;
        goto exitFail;
    }

    hdr->version = header->version;
    
    fileSize = (header->codeSize + 0x1100) & ~0xFFF;

    if (MapPluginInProcess(process, fileSize) == 0)
    
    {
        extern u32  g_savedGameInstr[2];
        u32  *game = (u32 *)procStart;

        g_savedGameInstr[0] = game[0];
        g_savedGameInstr[1] = game[1];

        game[0] = 0xE51FF004; 
        game[1] = (u32)PA_FROM_VA_PTR(gamePatchFunc);
    }
    else
        goto exitFail;

    svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, procStart, 0x1000);
    return true;

exitFail:
    svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, procStart, 0x1000);
    return false;
}

static void     SetKernelConfigurationMemoryFlag(bool loaded)
{
    u32 *flag = (u32 *)PA_FROM_VA_PTR(0x1FF800F0);

    *flag = loaded;
}

static void     PluginLoader_HandleCommands(void)
{
    u32    *cmdbuf = getThreadCommandBuffer();

    switch (cmdbuf[0] >> 16)
    {
        case 1: 
        {
            if (cmdbuf[0] != IPC_MakeHeader(1, 0, 2))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            Handle process = cmdbuf[2];

            if (!g_safeMode && g_isEnabled && g_memBlock != NULL && TryToLoadPlugin(process))
            {
                if (!g_userDefinedLoadParameters.isEnabled && g_userDefinedLoadParameters.noFlash)
                    g_userDefinedLoadParameters.noFlash = false;
                else
                {
                    for (u32 i = 0; i < 64; i++)
                    {
                        REG32(0x10202204) = 0x01FF9933;
                        svcSleepThread(5000000);
                    }
                    REG32(0x10202204) = 0;
                }

                g_process = process;
            }
            else
                svcCloseHandle(process);

            cmdbuf[0] = IPC_MakeHeader(1, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 2: 
        {
            if (cmdbuf[0] != IPC_MakeHeader(2, 0, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            cmdbuf[0] = IPC_MakeHeader(2, 2, 0);
            cmdbuf[1] = 0;
            cmdbuf[2] = (u32)g_isEnabled;
            break;
        }

        case 3: 
        {
            if (cmdbuf[0] != IPC_MakeHeader(3, 1, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            if (cmdbuf[1] != g_isEnabled)
            {
                g_isEnabled = cmdbuf[1];
                SaveSettings();
            }

            cmdbuf[0] = IPC_MakeHeader(3, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 4: 
        {
            if (cmdbuf[0] != IPC_MakeHeader(4, 2, 4))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            g_userDefinedLoadParameters.isEnabled = true;
            g_userDefinedLoadParameters.noFlash = cmdbuf[1];
            g_userDefinedLoadParameters.titleid = cmdbuf[2];
            strncpy(g_userDefinedLoadParameters.path, (const char *)cmdbuf[4], 255);
            memcpy(g_userDefinedLoadParameters.config, (void *)cmdbuf[6], 32 * sizeof(u32));

            cmdbuf[0] = IPC_MakeHeader(4, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 5: 
        {
            if (cmdbuf[0] != IPC_MakeHeader(5, 1, 8))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            u32 nbItems = cmdbuf[1];
            u32 states = cmdbuf[3];
            DisplayPluginMenu(cmdbuf);

            cmdbuf[0] = IPC_MakeHeader(5, 1, 2);
            cmdbuf[1] = 0;
            cmdbuf[2] = IPC_Desc_Buffer(nbItems, IPC_BUFFER_RW);
            cmdbuf[3] = states;
            break;
        }

        case 6: 
        {
            if (cmdbuf[0] != IPC_MakeHeader(6, 0, 4))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            const char *title = (const char *)cmdbuf[2];
            const char *body = (const char *)cmdbuf[4];

            DispMessage(title, body);

            cmdbuf[0] = IPC_MakeHeader(6, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 7: 
        {
            if (cmdbuf[0] != IPC_MakeHeader(7, 1, 4))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            const char *title = (const char *)cmdbuf[3];
            const char *body = (const char *)cmdbuf[5];

            DispErrMessage(title, body, cmdbuf[1]);

            cmdbuf[0] = IPC_MakeHeader(7, 1, 0);
            cmdbuf[1] = 0;
            break;
        }
    }
}

void        PluginLoader__ThreadMain(void)
{
    const char *title = "Plugin loader";

    Result  res = 0;
    MemOp   memRegion = isN3DS ? MEMOP_REGION_BASE : MEMOP_REGION_SYSTEM;
    Handle  handles[3];
    Handle  serverHandle, clientHandle, sessionHandle = 0;

    u32 *cmdbuf = getThreadCommandBuffer();
    u32 replyTarget = 0;
    u32 nbHandle;
    s32 index;

    
    {
        bool isAcuRegistered = false;

        while (true)
        {
            if (R_SUCCEEDED(srvIsServiceRegistered(&isAcuRegistered, "ac:u"))
                && isAcuRegistered)
                break;
            svcSleepThread(100000);
        }
    }

    
    if (R_FAILED((res = svcControlMemoryEx((u32 *)&g_memBlock, 0x07000000, 0, MemBlockSize, memRegion | MEMOP_ALLOC, MEMPERM_RW, true))))
    {
        g_memBlock = NULL;
        svcSleepThread(5000000000ULL);
        DispErrMessage(title, "Couldn't allocate memblock", res);
    }

    assertSuccess(svcCreatePort(&serverHandle, &clientHandle, "plg:ldr", 1));

    do
    {
        g_error.message = NULL;
        g_error.code = 0;
        handles[0] = serverHandle;
        handles[1] = sessionHandle == 0 ? g_process : sessionHandle;
        handles[2] = g_process;

        if(replyTarget == 0) 
            cmdbuf[0] = 0xFFFF0000;

        nbHandle = 1 + (sessionHandle != 0) + (g_process != 0);
        res = svcReplyAndReceive(&index, handles, nbHandle, replyTarget);

        if(R_FAILED(res))
        {
            if((u32)res == 0xC920181A) 
            {
                svcCloseHandle(sessionHandle);
                sessionHandle = 0;
                replyTarget = 0;
            }
            else
                svcBreak(USERBREAK_PANIC);
        }
        else
        {
            if(index == 0)
            {
                Handle session;
                assertSuccess(svcAcceptSession(&session, serverHandle));

                if(sessionHandle == 0)
                    sessionHandle = session;
                else
                    svcCloseHandle(session);
            }
            else if (index == 1 && handles[1] == sessionHandle)
            {
                PluginLoader_HandleCommands();
                replyTarget = sessionHandle;
            }
            else 
            {
                
                svcUnmapProcessMemoryEx(g_process, 0x07000000, g_codeSize);
                svcUnmapProcessMemoryEx(g_process, 0x06000000, g_heapSize);
                svcCloseHandle(g_process);
                g_process = 0;
            }
        }

        if (g_error.message != NULL)
            DispErrMessage(title, g_error.message, g_error.code);

        SetKernelConfigurationMemoryFlag(g_process != 0);

    } while(!terminationRequest);

    svcCloseHandle(sessionHandle);
    svcCloseHandle(clientHandle);
    svcCloseHandle(serverHandle);
    if (g_memBlock != NULL)
        svcControlMemoryEx((u32 *)&g_memBlock, (u32)g_memBlock, 0, MemBlockSize, memRegion | MEMOP_FREE, 0, true);
}
