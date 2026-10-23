/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2021 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include <3ds.h>
#include "menus/memdump.h"
#include "menus/lang.h"
#include "menus/text_input.h"
#include "draw.h"
#include "fmt.h"
#include "ifile.h"
#include "memory.h"
#include "csvc.h"

#define MD_MAX_PROCESSES    0x40
#define MD_MAX_REGIONS      64
#define MD_CHUNK            MENU_WORK_BUF_SIZE
#define MD_DUMP_DIR         "/luma/dumps"
#define MD_MAP_ADDR         0x00100000

typedef struct MdProcess {
    u32  pid;
    u64  titleId;
    char name[9];
} MdProcess;

typedef struct MdRegion {
    u32 base;
    u32 size;
    u32 perm;
    u32 state;
} MdRegion;

static MdProcess g_procs[MD_MAX_PROCESSES];
static u32       g_procCount;

static MdRegion  g_regions[MD_MAX_REGIONS];
static u32       g_regionCount;

static u32 MemDump_ListProcesses(void)
{
    u32 pidList[MD_MAX_PROCESSES];
    s32 processAmount = 0;

    g_procCount = 0;

    if (R_FAILED(svcGetProcessList(&processAmount, pidList, MD_MAX_PROCESSES)))
        return 0;

    for (s32 i = 0; i < processAmount && g_procCount < MD_MAX_PROCESSES; ++i)
    {
        Handle handle = 0;

        if (R_FAILED(svcOpenProcess(&handle, pidList[i])))
            continue;

        MdProcess *p = &g_procs[g_procCount];
        s64 out = 0;

        memset(p->name, 0, sizeof(p->name));
        svcGetProcessInfo((s64 *)p->name, handle, 0x10000);
        svcGetProcessInfo(&out, handle, 0x10001);

        p->name[8] = 0;
        p->titleId = (u64)out;
        p->pid = pidList[i];

        ++g_procCount;
        svcCloseHandle(handle);
    }

    return g_procCount;
}

static u32 MemDump_ScanRegions(Handle processHandle)
{
    u32 addr = 0;

    g_regionCount = 0;

    while (g_regionCount < MD_MAX_REGIONS)
    {
        MemInfo info;
        PageInfo page;

        if (R_FAILED(svcQueryProcessMemory(&info, &page, processHandle, addr)))
            break;

        if (info.state != MEMSTATE_FREE && info.size != 0 && (info.perm & MEMPERM_READ) != 0)
        {
            MdRegion *r = &g_regions[g_regionCount++];
            r->base = info.base_addr;
            r->size = info.size;
            r->perm = info.perm;
            r->state = info.state;
        }

        u32 next = info.base_addr + info.size;
        if (info.size == 0 || next <= addr)
            break;

        addr = next;

        if (addr >= 0x80000000u)
            break;
    }

    return g_regionCount;
}

static void MemDump_EnsureDir(void)
{
    FS_Archive archive = 0;

    if (R_SUCCEEDED(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
    {
        FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, MD_DUMP_DIR), 0);
        FSUSER_CloseArchive(archive);
    }
}

static void MemDump_DeleteFile(const char *path)
{
    FS_Archive archive = 0;

    if (R_SUCCEEDED(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
    {
        FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, path));
        FSUSER_CloseArchive(archive);
    }
}

static void MemDump_DrawWritten(u64 bytes)
{
    Draw_Lock();
    Draw_DrawFormattedString(10, 4 + 7 * SPACING_Y, COLOR_WHITE, Lang(STR_WRITTEN_KB), (u32)(bytes >> 10));
    Draw_FlushFramebuffer();
    Draw_Unlock();
}

static Result MemDump_WriteRegion(Handle processHandle, const MdRegion *r, const char *path, u64 doneBefore,
                                  u32 *outWritten, bool *cancelled)
{
    IFile file;
    Result res;
    u64 written = 0;

    *outWritten = 0;
    *cancelled = false;

    MemDump_DeleteFile(path);

    res = IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                     fsMakePath(PATH_ASCII, path), FS_OPEN_CREATE | FS_OPEN_WRITE);
    if (R_FAILED(res))
        return res;

    file.pos = 0;

    for (u32 off = 0; off < r->size; off += MD_CHUNK)
    {
        u32 n = r->size - off;
        if (n > MD_CHUNK)
            n = MD_CHUNK;

        if (R_FAILED(svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, MD_MAP_ADDR, processHandle, r->base + off, n)))
            break;

        memcpy(g_menuWorkBuf, (const void *)MD_MAP_ADDR, n);
        svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, MD_MAP_ADDR, n);

        res = IFile_Write(&file, &written, g_menuWorkBuf, n, FS_WRITE_FLUSH);
        if (R_FAILED(res))
            break;

        *outWritten += n;
        MemDump_DrawWritten(doneBefore + *outWritten);

        if (HID_PAD & BUTTON_B)
        {
            *cancelled = true;
            break;
        }
    }

    IFile_Close(&file);
    return res;
}

static void MemDump_WriteMap(const MdProcess *p, const char *path, u32 dumped)
{
    IFile file;
    char buf[192];
    u64 written = 0;
    u32 len;

    MemDump_DeleteFile(path);

    if (R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, path), FS_OPEN_CREATE | FS_OPEN_WRITE)))
        return;

    file.pos = 0;

    len = (u32)sprintf(buf, "process   %s\r\ntitleid   %016llx\r\npid       %lu\r\nregions   %lu of %lu dumped\r\n\r\n",
                       p->name, p->titleId, p->pid, dumped, g_regionCount);
    IFile_Write(&file, &written, buf, len, FS_WRITE_FLUSH);

    len = (u32)sprintf(buf, "base      size      perm  state\r\n");
    IFile_Write(&file, &written, buf, len, FS_WRITE_FLUSH);

    for (u32 i = 0; i < g_regionCount; ++i)
    {
        const MdRegion *r = &g_regions[i];

        len = (u32)sprintf(buf, "%08lx  %08lx  %c%c%c   %lu\r\n",
                           r->base, r->size,
                           (r->perm & MEMPERM_READ) ? 'r' : '-',
                           (r->perm & MEMPERM_WRITE) ? 'w' : '-',
                           (r->perm & MEMPERM_EXECUTE) ? 'x' : '-',
                           r->state);
        IFile_Write(&file, &written, buf, len, FS_WRITE_FLUSH);
    }

    IFile_Close(&file);
}

static void MemDump_Run(const MdProcess *p, bool codeOnly)
{
    Handle processHandle = 0;
    char dir[128];
    char path[192];
    u32 dumped = 0;
    u64 totalBytes = 0;
    bool cancelled = false;

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_DrawString(10, 4, COLOR_TITLE, "Memory dump");
    Draw_DrawString(10, 4 + 3 * SPACING_Y, COLOR_WHITE, "Opening process...");
    Draw_FlushFramebuffer();
    Draw_Unlock();

    if (R_FAILED(svcOpenProcess(&processHandle, p->pid)))
    {
        Draw_Lock();
        Draw_DrawBilingual(10, 4 + 5 * SPACING_Y, COLOR_RED, "Process wo hirakemasen deshita", "svcOpenProcess failed");
        Draw_FlushFramebuffer();
        Draw_Unlock();
        waitInput();
        return;
    }

    if (MemDump_ScanRegions(processHandle) == 0)
    {
        Draw_Lock();
        Draw_DrawBilingual(10, 4 + 5 * SPACING_Y, COLOR_RED, "Yomitori kanou na ryouiki ga arimasen", "No readable regions found");
        Draw_FlushFramebuffer();
        Draw_Unlock();
        waitInput();
        svcCloseHandle(processHandle);
        return;
    }

    MemDump_EnsureDir();

    sprintf(dir, MD_DUMP_DIR "/%016llx", p->titleId);

    FS_Archive archive = 0;
    if (R_SUCCEEDED(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
    {
        FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, dir), 0);
        FSUSER_CloseArchive(archive);
    }

    for (u32 i = 0; i < g_regionCount && !cancelled; ++i)
    {
        const MdRegion *r = &g_regions[i];

        if ((codeOnly && (r->perm & MEMPERM_EXECUTE) == 0) || r->state == MEMSTATE_IO)
            continue;

        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 4, COLOR_TITLE, "Memory dump");
        Draw_DrawFormattedString(10, 4 + 2 * SPACING_Y, COLOR_WHITE, "%s  %016llx", p->name, p->titleId);
        Draw_DrawFormattedString(10, 4 + 4 * SPACING_Y, COLOR_WHITE, Lang(STR_REGION), i + 1, g_regionCount);
        Draw_DrawFormattedString(10, 4 + 5 * SPACING_Y, COLOR_WHITE, "%08lx  %lu KB   ",
                                 r->base, r->size >> 10);
        Draw_DrawString(10, 220, COLOR_WHITE, "B: cancel");
        Draw_FlushFramebuffer();
        Draw_Unlock();
        MemDump_DrawWritten(totalBytes);

        u32 written = 0;
        sprintf(path, "%s/%08lx_%08lx.bin", dir, r->base, r->size);

        if (R_SUCCEEDED(MemDump_WriteRegion(processHandle, r, path, totalBytes, &written, &cancelled))
            && written != 0 && !cancelled)
        {
            ++dumped;
            totalBytes += written;
        }
        else if (written == 0 || cancelled)
        {
            MemDump_DeleteFile(path);
        }
    }

    sprintf(path, "%s/map.txt", dir);
    MemDump_WriteMap(p, path, dumped);

    svcCloseHandle(processHandle);

    while (cancelled && (HID_PAD & BUTTON_B))
        svcSleepThread(10 * 1000 * 1000LL);

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_DrawString(10, 4, COLOR_TITLE, "Memory dump");
    Draw_DrawFormattedString(10, 4 + 3 * SPACING_Y, cancelled ? COLOR_RED : COLOR_LIME,
                             Lang(cancelled ? STR_CANCELLED_REGIONS_KB : STR_DONE_REGIONS_KB),
                             dumped, (u32)(totalBytes >> 10));
    Draw_DrawFormattedString(10, 4 + 5 * SPACING_Y, COLOR_WHITE, "%s", dir);
    Draw_DrawString(10, 4 + 7 * SPACING_Y, COLOR_WHITE, Lang(STR_MAP_TXT_LISTS_EVERY_REGION));
    Draw_DrawString(10, 220, COLOR_WHITE, Lang(STR_PRESS_ANY_BUTTON));
    Draw_FlushFramebuffer();
    Draw_Unlock();
    waitInput();
}

void RosalinaMenu_MemoryDump(void)
{
    s32 sel = 0, scroll = 0;
    const s32 rows = 15;

    MemDump_ListProcesses();

    menuEnter();

    do
    {
        s32 total = (s32)g_procCount;

        if (sel >= total) sel = total > 0 ? total - 1 : 0;
        if (sel < 0) sel = 0;
        if (sel < scroll) scroll = sel;
        if (sel >= scroll + rows) scroll = sel - rows + 1;

        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 4, COLOR_TITLE, Lang(STR_MEMORY_DUMP_PICK_A_PROCESS));

        u32 y = 4 + 2 * SPACING_Y;
        for (s32 r = 0; r < rows; ++r)
        {
            s32 idx = scroll + r;
            if (idx >= total)
                break;

            u32 col = (idx == sel) ? COLOR_LIME : COLOR_WHITE;
            Draw_DrawFormattedString(10, y, col, "%s %-8.8s  %016llx",
                                     (idx == sel) ? ">" : " ",
                                     g_procs[idx].name, g_procs[idx].titleId);
            y += SPACING_Y;
        }
        Draw_DrawScrollMarks(4 + 2 * SPACING_Y, rows, scroll, total, g_cursorColor);

        if (total == 0)
            Draw_DrawBilingual(10, y, COLOR_RED, "Process ga arimasen", "No processes");

        Draw_DrawString(10, 214, COLOR_WHITE, Lang(STR_A_DUMP_ALL_X_CODE_ONLY));
        Draw_DrawString(10, 225, COLOR_WHITE, Lang(STR_Y_REFRESH_B_BACK));
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 k = waitInput();

        if (terminationRequest || (k & BUTTON_B))
            break;

        if (k & BUTTON_DOWN)       { if (sel + 1 < total) ++sel; }
        else if (k & BUTTON_UP)    { if (sel > 0) --sel; }
        else if (k & BUTTON_Y)     { MemDump_ListProcesses(); sel = 0; scroll = 0; }
        else if ((k & (BUTTON_A | BUTTON_X)) && total > 0)
        {
            MemDump_Run(&g_procs[sel], (k & BUTTON_X) != 0);
        }
    }
    while (!terminationRequest);

    menuLeave();
}
