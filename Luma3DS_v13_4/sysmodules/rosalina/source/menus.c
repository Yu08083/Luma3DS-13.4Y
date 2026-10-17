/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2020 Aurora Wright, TuxSH
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
#include <3ds/os.h>
#include "menus.h"
#include "menus/lang.h"
#include "menus/controls.h"
#include "menus/theme.h"
#include "menus/online_id.h"
#include "menus/online_bridge.h"
#include "menus/streaming.h"
#include "menu.h"
#include "draw.h"
#include "menus/process_list.h"
#include "menus/n3ds.h"
#include "menus/debugger.h"
#include "menus/miscellaneous.h"
#include "menus/sysconfig.h"
#include "menus/screen_filters.h"
#include "menus/battery.h"
#include "menus/memdump.h"
#include "menus/ftpd.h"
#include "menus/text_input.h"
#include "plugin.h"
#include "ifile.h"
#include "memory.h"
#include "fmt.h"
#include "sha256.h"
#include "process_patches.h"
#include "luma_config.h"

Menu rosalinaMenu = {
    "Rosalina menu",
    {
        { "Take screenshot", METHOD, .method = &RosalinaMenu_TakeScreenshot },
        { "Wireless streaming...", METHOD, .method = &N3DSMenu_Streaming, .visibility = &menuCheckN3ds },
        { "Screen filters...", MENU, .menu = &screenFiltersMenu },
        { "Cheats...", METHOD, .method = &RosalinaMenu_Cheats },
        { "", METHOD, .method = PluginLoader__MenuCallback},
        { "New 3DS menu...", MENU, .menu = &N3DSMenu, .visibility = &menuCheckN3ds },
        { "Process list", METHOD, .method = &RosalinaMenu_ProcessList },
        { "Debugger options...", MENU, .menu = &debuggerMenu },
        { "System configuration...", MENU, .menu = &sysconfigMenu },
        { "Miscellaneous options...", MENU, .menu = &miscellaneousMenu },
        { "Colors...", METHOD, .method = &RosalinaMenu_AccentColor },
        { "Language / 言語", METHOD, .method = &RosalinaMenu_Language },
        { "Menu controls...", METHOD, .method = &RosalinaMenu_Controls },
        { "Background theme...", METHOD, .method = &RosalinaMenu_Theme },
        { "Online ID...", METHOD, .method = &RosalinaMenu_OnlineId },
        { "Online connection...", METHOD, .method = &RosalinaMenu_OnlineBridge },
        { "3gx / txt checker", METHOD, .method = &RosalinaMenu_3gxTxtChecker },
        { "Check SD Card", METHOD, .method = &RosalinaMenu_CheckSdCard },
        { "File browser", METHOD, .method = &RosalinaMenu_FileBrowser },
        { "Save settings", METHOD, .method = &RosalinaMenu_SaveSettings },
        { "Return To HOME Menu", METHOD, .method = &RosalinaMenu_ReturnToHomeMenu },
        { "Switch version (v9.1/v13.4Y)", METHOD, .method = &RosalinaMenu_ChainloadVersionToggle },
        { "Power off / reboot", METHOD, .method = &RosalinaMenu_PowerOffOrReboot },
        { "Battery info", METHOD, .method = &RosalinaMenu_BatteryInfo },
        { "Memory dump", METHOD, .method = &RosalinaMenu_MemoryDump },
        { "FTP server", METHOD, .method = &RosalinaMenu_Ftpd },
        { "System info", METHOD, .method = &RosalinaMenu_ShowSystemInfo },
        { "Credits", METHOD, .method = &RosalinaMenu_ShowCredits },
        { "Debug info", METHOD, .method = &RosalinaMenu_ShowDebugInfo, .visibility = &rosalinaMenuShouldShowDebugInfo },
        {},
    }
};

bool rosalinaMenuShouldShowDebugInfo(void)
{
    

    s64 out;
    svcGetSystemInfo(&out, 0x10000, 0x200);
    return out == 0;
}

void RosalinaMenu_SaveSettings(void)
{
    Result res = LumaConfig_SaveSettings();
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Save settings"));
        if(R_SUCCEEDED(res))
            Draw_DrawString(10, 30, COLOR_WHITE, Lang_Translate("Operation succeeded."));
        else
            Draw_DrawFormattedString(10, 30, COLOR_WHITE, Lang(STR_OPERATION_FAILED_0X), res);
        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while(!(waitInput() & KEY_B) && !menuShouldExit);
}

void RosalinaMenu_PowerOffOrReboot(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_POWER_OFF_REBOOT));
        Draw_DrawString(10, 30, COLOR_WHITE, Lang(STR_PRESS_A_TO_POWER_OFF_PRESS_Y_TO_REBOOT_P));
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_Y)
        {
            menuLeave();
            APT_HardwareResetAsync();
            return;
        }
        else if(pressed & KEY_A)
        {
            
            menuLeave();
            srvPublishToSubscriber(0x203, 0);
            return;
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}




#define MY_LUMA_VERSION FORK_LUMA_VERSION
#define OTHER_LUMA_VERSION FORK_OTHER_VERSION

#define CHAINLOAD_FLAG_PATH "/luma/chainload_ver.flag"


static bool chainloadVersionFlagSet(const char *version)
{
    IFile f;
    u64 total = 0;

    if(R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                           fsMakePath(PATH_ASCII, CHAINLOAD_FLAG_PATH),
                           FS_OPEN_CREATE | FS_OPEN_WRITE)))
        return false;

    
    char buf[4];
    u32 len = strlen(version);

    memcpy(buf, version, len);
    memset(buf + len, '\n', sizeof(buf) - len);

    bool ok = R_SUCCEEDED(IFile_Write(&f, &total, buf, sizeof(buf), FS_WRITE_FLUSH));
    IFile_Close(&f);
    return ok;
}

bool chainloadVersionFirmExists(const char *version)
{
    IFile f;
    u64 size = 0;
    char path[40];

    sprintf(path, "/3ds/luma/%s/boot.firm", version);

    if(R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                           fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
        return false;

    IFile_GetSize(&f, &size);

    
    u64 total = 0;
    char magic[4] = {0};
    bool ok = size > 0x200
              && R_SUCCEEDED(IFile_Read(&f, &total, magic, sizeof(magic)))
              && total == sizeof(magic)
              && memcmp(magic, "FIRM", 4) == 0;

    IFile_Close(&f);
    return ok;
}


void RosalinaMenu_ChainloadVersionToggle(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    const char *target = OTHER_LUMA_VERSION;

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_VERSION_SWITCH_CHAINLOAD));
        Draw_DrawFormattedString(10, 30, COLOR_WHITE, Lang(STR_CURRENT_V_V),
            FORK_DISPLAY_VERSION, FORK_OTHER_DISPLAY_VERSION);
        Draw_DrawString(10, 30 + SPACING_Y, COLOR_WHITE, Lang(STR_A_LAUNCH_B_GO_BACK));
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            if(!chainloadVersionFirmExists(target))
            {
                LogLine("[バージョン切り替え: 切り替え先の boot.firm が見つかりません] / [version switch: target boot.firm missing]");
                Draw_Lock();
                Draw_ClearFramebuffer();
                Draw_FlushFramebuffer();
                Draw_Unlock();

                do
                {
                    Draw_Lock();
                    Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_VERSION_SWITCH_CHAINLOAD));
                    Draw_DrawFormattedString(10, 30, COLOR_RED, Lang_Translate("[boot.firm for v%s was not found]"), target);
                    Draw_DrawFormattedString(10, 30 + SPACING_Y, COLOR_RED, Lang_Translate("[boot.firm for v%s was not found]"), target);
                    Draw_DrawString(10, 30 + 3 * SPACING_Y, COLOR_WHITE, Lang_Translate("B: go back"));
                    Draw_FlushFramebuffer();
                    Draw_Unlock();
                }
                while(!(waitInput() & KEY_B) && !menuShouldExit);

                return;
            }

            
            if(!chainloadVersionFlagSet(target))
            {
                LogLine("[バージョン切り替え: SD への書き込みに失敗しました] / [version switch: SD write failed]");
                Draw_Lock();
                Draw_ClearFramebuffer();
                Draw_FlushFramebuffer();
                Draw_Unlock();

                do
                {
                    Draw_Lock();
                    Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_VERSION_SWITCH_CHAINLOAD));
                    Draw_DrawString(10, 30, COLOR_RED, Lang_Translate("Failed to write to the SD card"));
                    Draw_DrawString(10, 30 + 3 * SPACING_Y, COLOR_WHITE, Lang_Translate("B: go back"));
                    Draw_FlushFramebuffer();
                    Draw_Unlock();
                }
                while(!(waitInput() & KEY_B) && !menuShouldExit);

                return;
            }

            menuLeave();
            APT_HardwareResetAsync();
            return;
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void RosalinaMenu_ShowSystemInfo(void)
{
    u32 kver = osGetKernelVersion();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_ROSALINA_SYSTEM_INFO));

        u32 posY = 30;

        if (areScreenTypesInitialized)
        {
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_TOP_SCREEN_TYPE), topScreenType);
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_BOTTOM_SCREEN_TYPE), bottomScreenType);
        }

        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_KERNEL_VERSION), GET_VERSION_MAJOR(kver), GET_VERSION_MINOR(kver), GET_VERSION_REVISION(kver));
        if (mcuFwVersion != 0 && mcuInfoTableRead)
        {
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_MCU_FW_VERSION), GET_VERSION_MAJOR(mcuFwVersion), GET_VERSION_MINOR(mcuFwVersion));
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_PMIC_VENDOR), mcuInfoTable[1]);
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_BATTERY_VENDOR), mcuInfoTable[2]);
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while(!(waitInput() & KEY_B) && !menuShouldExit);
}

void RosalinaMenu_ShowDebugInfo(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    char memoryMap[512];
    formatMemoryMapOfProcess(memoryMap, 511, CUR_PROCESS_HANDLE);

    s64 kextAddrSize;
    svcGetSystemInfo(&kextAddrSize, 0x10000, 0x300);
    u32 kextPa = (u32)((u64)kextAddrSize >> 32);
    u32 kextSize = (u32)kextAddrSize;

    FS_SdMmcSpeedInfo speedInfo;

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_ROSALINA_DEBUG_INFO));

        u32 posY = 30;

        posY = Draw_DrawString(10, posY, COLOR_WHITE, memoryMap);
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Kernel ext PA: %08lx - %08lx\n\n", kextPa, kextPa + kextSize);
        if (R_SUCCEEDED(FSUSER_GetSdmcSpeedInfo(&speedInfo)))
        {
            u32 clkDiv = 1 << (1 + (speedInfo.sdClkCtrl & 0xFF));
            posY = Draw_DrawFormattedString(
                10, posY, COLOR_WHITE, "SDMC speed: HS=%d %lukHz\n",
                (int)speedInfo.highSpeedModeEnabled, SYSCLOCK_SDMMC / (1000 * clkDiv)
            );
        }
        if (R_SUCCEEDED(FSUSER_GetNandSpeedInfo(&speedInfo)))
        {
            u32 clkDiv = 1 << (1 + (speedInfo.sdClkCtrl & 0xFF));
            posY = Draw_DrawFormattedString(
                10, posY, COLOR_WHITE, "NAND speed: HS=%d %lukHz\n",
                (int)speedInfo.highSpeedModeEnabled, SYSCLOCK_SDMMC / (1000 * clkDiv)
            );
        }
        {
            posY = Draw_DrawFormattedString(
                10, posY, COLOR_WHITE, "APPMEMTYPE: %lu\n",
                OS_KernelConfig->app_memtype
            );
        }
        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while(!(waitInput() & KEY_B) && !menuShouldExit);
}

void RosalinaMenu_ShowCredits(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Rosalina -- Luma3DS credits"));

        u32 posY = Draw_DrawString(10, 30, COLOR_WHITE, "Luma3DS (c) 2016-2026 LumaTeam") + SPACING_Y;

        posY = Draw_DrawString(10, posY + SPACING_Y, COLOR_WHITE, Lang_Translate("3DSX loading code by fincs"));
        posY = Draw_DrawString(10, posY + SPACING_Y, COLOR_WHITE, Lang_Translate("Networking code & basic GDB functionality by Stary"));
        posY = Draw_DrawString(10, posY + SPACING_Y, COLOR_WHITE, Lang_Translate("InputRedirection by Stary (PoC by ShinyQuagsire)"));
        Draw_DrawString(10, posY + SPACING_Y, COLOR_WHITE, Lang_Translate("Other features & Japanese translation: @yu_"));

        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while(!(waitInput() & KEY_B) && !menuShouldExit);
}

#define TRY(expr) if(R_FAILED(res = (expr))) goto end;

static s64 timeSpentConvertingScreenshot = 0;
static s64 timeSpentWritingScreenshot = 0;

static Result RosalinaMenu_WriteScreenshot(IFile *file, u32 width, bool top, bool left)
{
    u64 total;
    Result res = 0;
    u32 lineSize = 3 * width;

    
    
    u32 scaleFactorY = width > 400 ? 2 : 1;
    u32 numLinesScaled = 240 * scaleFactorY;
    u32 remaining = lineSize * numLinesScaled;

    TRY(Draw_AllocateFramebufferCacheForScreenshot(remaining));

    u8 *framebufferCache = (u8 *)Draw_GetFramebufferCache();
    u8 *framebufferCacheEnd = framebufferCache + Draw_GetFramebufferCacheSize();

    u8 *buf = framebufferCache;
    Draw_CreateBitmapHeader(framebufferCache, width, numLinesScaled);
    const u32 headerSize = 0x40;
    buf += headerSize;

    u32 y = 0;
    
    while (remaining != 0)
    {
        s64 t0 = svcGetSystemTick();
        u32 available = (u32)(framebufferCacheEnd - buf);
        u32 size = available < remaining ? available : remaining;
        u32 nlines = size / (lineSize * scaleFactorY);
        Draw_ConvertFrameBufferLines(buf, width, y, nlines, scaleFactorY, top, left);

        s64 t1 = svcGetSystemTick();
        timeSpentConvertingScreenshot += t1 - t0;
        TRY(IFile_Write(file, &total, framebufferCache, (y == 0 ? headerSize : 0) + lineSize * nlines * scaleFactorY, 0)); 
        timeSpentWritingScreenshot += svcGetSystemTick() - t1;

        y += nlines;
        remaining -= lineSize * nlines * scaleFactorY;
        buf = framebufferCache;
    }
    end:

    Draw_FreeFramebufferCache();
    return res;
}

void RosalinaMenu_ReturnToHomeMenu(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang(STR_RETURN_TO_HOME_MENU));
        Draw_DrawString(10, 30, COLOR_WHITE, Lang(STR_PRESS_A_TO_CONFIRM_PRESS_B_TO_GO_BACK));
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            srvPublishToSubscriber(0x204, 0);
            menuRequestClose();
            return;
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void RosalinaMenu_TakeScreenshot(void)
{
    IFile file = {0};
    Result res = 0;

    char filename[64];
    char dateTimeStr[32];

    FS_Archive archive;
    FS_ArchiveID archiveId;
    s64 out;
    bool isSdMode;

    timeSpentConvertingScreenshot = 0;
    timeSpentWritingScreenshot = 0;

    if(R_FAILED(svcGetSystemInfo(&out, 0x10000, 0x203))) svcBreak(USERBREAK_ASSERT);
    isSdMode = (bool)out;

    archiveId = isSdMode ? ARCHIVE_SDMC : ARCHIVE_NAND_RW;
    Draw_Lock();
    Draw_RestoreFramebuffer();
    Draw_FreeFramebufferCache();

    svcFlushEntireDataCache();

    bool is3d;
    u32 topWidth, bottomWidth; 

    Draw_GetCurrentScreenInfo(&bottomWidth, &is3d, false);
    Draw_GetCurrentScreenInfo(&topWidth, &is3d, true);

    res = FSUSER_OpenArchive(&archive, archiveId, fsMakePath(PATH_EMPTY, ""));
    if(R_SUCCEEDED(res))
    {
        res = FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, "/luma/screenshots"), 0);
        if((u32)res == 0xC82044BE) 
            res = 0;
        FSUSER_CloseArchive(archive);
    }
    else
    {
        archive = 0;
        goto end;
    }

    dateTimeToString(dateTimeStr, osGetTime(), true);

    sprintf(filename, "/luma/screenshots/%s_top.bmp", dateTimeStr);
    TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
    TRY(RosalinaMenu_WriteScreenshot(&file, topWidth, true, true));
    TRY(IFile_Close(&file));

    sprintf(filename, "/luma/screenshots/%s_bot.bmp", dateTimeStr);
    TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
    TRY(RosalinaMenu_WriteScreenshot(&file, bottomWidth, false, true));
    TRY(IFile_Close(&file));

    if(is3d && (Draw_GetCurrentFramebufferAddress(true, true) != Draw_GetCurrentFramebufferAddress(true, false)))
    {
        sprintf(filename, "/luma/screenshots/%s_top_right.bmp", dateTimeStr);
        TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
        TRY(RosalinaMenu_WriteScreenshot(&file, topWidth, true, false));
        TRY(IFile_Close(&file));
    }

end:
    IFile_Close(&file);

    if (archive != 0)
        FSUSER_CloseArchive(archive);

    if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
        __builtin_trap(); 

    svcFlushEntireDataCache();
    Draw_SetupFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Screenshot"));
        if(R_FAILED(res))
            Draw_DrawFormattedString(10, 30, COLOR_WHITE, Lang(STR_OPERATION_FAILED_0X), (u32)res);
        else
        {
            u32 t1 = (u32)(1000 * timeSpentConvertingScreenshot / SYSCLOCK_ARM11);
            u32 t2 = (u32)(1000 * timeSpentWritingScreenshot / SYSCLOCK_ARM11);
            u32 posY = 30;
            posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Operation succeeded.\n\n"));
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_TIME_SPENT_CONVERTING_MS), t1);
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_TIME_SPENT_WRITING_FILES_MS), t2);
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while(!(waitInput() & KEY_B) && !menuShouldExit);
}

static Result menuWriteSelfScreenshot(IFile *file)
{
    u64 total;
    Result res = 0;

    u32 width = 320;
    u32 lineSize = 3 * width;

    u32 scaleFactorY = 1;
    u32 numLinesScaled = 240 * scaleFactorY;

    u32 addr = 0x0D800000; 
    u32 tmp;

    u32 size = ((54 + lineSize * numLinesScaled * scaleFactorY) + 0xFFF) >> 12 << 12; 
    u8 *buffer = NULL;

    TRY(svcControlMemoryEx(&tmp, addr, 0, size, MEMOP_ALLOC | MEMOP_REGION_SYSTEM, MEMPERM_READWRITE, true));
    buffer = (u8 *)addr;

    Draw_CreateBitmapHeader(buffer, width, numLinesScaled);

    Draw_ConvertFrameBufferLines(buffer + 54, width, 0, numLinesScaled, scaleFactorY, false, false);
    TRY(IFile_Write(file, &total, buffer, 54 + lineSize * numLinesScaled * scaleFactorY, 0)); 

end:
    if (buffer)
        svcControlMemoryEx(&tmp, addr, 0, size, MEMOP_FREE, MEMPERM_DONTCARE, false);

    return res;
}

void menuTakeSelfScreenshot(void)
{
    

    IFile file = {0};
    Result res = 0;

    char filename[100];
    char dateTimeStr[64];

    FS_Archive archive;
    FS_ArchiveID archiveId;
    s64 out;
    bool isSdMode;

    timeSpentConvertingScreenshot = 0;
    timeSpentWritingScreenshot = 0;

    if(R_FAILED(svcGetSystemInfo(&out, 0x10000, 0x203))) svcBreak(USERBREAK_ASSERT);
    isSdMode = (bool)out;

    archiveId = isSdMode ? ARCHIVE_SDMC : ARCHIVE_NAND_RW;
    Draw_Lock();
    svcFlushEntireDataCache();

    res = FSUSER_OpenArchive(&archive, archiveId, fsMakePath(PATH_EMPTY, ""));
    if(R_SUCCEEDED(res))
    {
        res = FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, "/luma/screenshots"), 0);
        if((u32)res == 0xC82044BE) 
            res = 0;
        FSUSER_CloseArchive(archive);
    }
    else
    {
        archive = 0;
        goto end;
    }

    dateTimeToString(dateTimeStr, osGetTime(), true);

    sprintf(filename, "/luma/screenshots/rosalina_menu_%s.bmp", dateTimeStr);

    TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
    TRY(menuWriteSelfScreenshot(&file));

end:
    IFile_Close(&file);

    if (archive != 0)
        FSUSER_CloseArchive(archive);
}

#undef TRY

u32 g_accentColor = COLOR_TITLE;
u32 g_textColor   = COLOR_TITLE;
u32 g_cursorColor = COLOR_TITLE;
u16 g_menuBgColor = COLOR_BLACK;
bool g_safeMode = false;

static u32  g_colH[4] = {264, 264, 264, 0};
static u32  g_colS[4] = {99,  99,  99,  0};
static u32  g_colV[4] = {255, 255, 255, 0};
static bool g_colorsLoaded = false;

#define COLORS_PATH "/luma/colors.cfg"


#define HUE_X 10
#define HUE_Y 35
#define HUE_W 300
#define HUE_H 16
#define SV_X  10
#define SV_Y  58
#define SV_W  200
#define SV_H  118
#define PRE_X 280
#define PRE_Y 58
#define PRE_W 30
#define PRE_H 13
#define PRE_N 9
#define PRV_X 10
#define PRV_Y 182
#define PRV_W 90
#define PRV_H 32

static const u16 g_presetHSV[PRE_N][3] =
{
    {180, 255, 255},
    {0,   255, 255},
    {30,  255, 255},
    {60,  255, 255},
    {120, 255, 255},
    {200, 255, 255},
    {240, 255, 255},
    {300, 255, 255},
    {0,   0,   255},
};

static u16 AccentHsv(u32 h, u32 s, u32 v)
{
    u32 r, g, b;

    h %= 360;

    if (s == 0)
        r = g = b = v;
    else
    {
        u32 region = h / 60;
        u32 rem = (h % 60) * 255 / 60;
        u32 p = v * (255 - s) / 255;
        u32 q = v * (255 - s * rem / 255) / 255;
        u32 t = v * (255 - s * (255 - rem) / 255) / 255;

        switch (region)
        {
            case 0:  r = v; g = t; b = p; break;
            case 1:  r = q; g = v; b = p; break;
            case 2:  r = p; g = v; b = t; break;
            case 3:  r = p; g = q; b = v; break;
            case 4:  r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }

    return RGB565(r >> 3, g >> 2, b >> 3);
}

static void AccentPixel(u32 x, u32 y, u16 color)
{
    if (x < SCREEN_BOT_WIDTH && y < SCREEN_BOT_HEIGHT)
        ((volatile u16 *)FB_BOTTOM_VRAM_ADDR)[x * SCREEN_BOT_HEIGHT + (SCREEN_BOT_HEIGHT - 1 - y)] = color;
}

static void AccentFillRect(u32 x, u32 y, u32 w, u32 h, u16 color)
{
    for (u32 sy = y; sy < y + h; ++sy)
        for (u32 sx = x; sx < x + w; ++sx)
            AccentPixel(sx, sy, color);
}

static void AccentDrawBase(u32 hue, u32 sat, u32 val)
{
    Draw_InvalidateFramebuffer();
    for (u32 i = 0; i < HUE_W; ++i)
        AccentFillRect(HUE_X + i, HUE_Y, 1, HUE_H, AccentHsv(i * 360 / HUE_W, 255, 255));

    for (u32 dy = 0; dy < SV_H; ++dy)
    {
        u32 vv = 255 - dy * 255 / (SV_H - 1);
        for (u32 dx = 0; dx < SV_W; ++dx)
            AccentPixel(SV_X + dx, SV_Y + dy, AccentHsv(hue, dx * 255 / (SV_W - 1), vv));
    }

    for (u32 i = 0; i < PRE_N; ++i)
        AccentFillRect(PRE_X, PRE_Y + i * PRE_H, PRE_W, PRE_H - 1,
                       AccentHsv(g_presetHSV[i][0], g_presetHSV[i][1], g_presetHSV[i][2]));

    u32 hx = HUE_X + hue * HUE_W / 360;
    AccentFillRect(hx, HUE_Y - 2, 2, HUE_H + 4, COLOR_WHITE);

    u32 mx = SV_X + sat * (SV_W - 1) / 255;
    u32 my = SV_Y + (255 - val) * (SV_H - 1) / 255;
    AccentFillRect(mx >= 2 ? mx - 2 : 0, my >= 2 ? my - 2 : 0, 5, 5, COLOR_WHITE);
    AccentPixel(mx, my, COLOR_BLACK);

    AccentFillRect(PRV_X, PRV_Y, PRV_W, PRV_H, AccentHsv(hue, sat, val));
}

static void ColorsApply(void)
{
    g_accentColor = AccentHsv(g_colH[0], g_colS[0], g_colV[0]);
    g_textColor   = AccentHsv(g_colH[1], g_colS[1], g_colV[1]);
    g_cursorColor = AccentHsv(g_colH[2], g_colS[2], g_colV[2]);
    g_menuBgColor = (u16)AccentHsv(g_colH[3], g_colS[3], g_colV[3]);
}

void AccentColor_EnsureLoaded(void)
{
    if (g_colorsLoaded)
        return;
    g_colorsLoaded = true;

    if (g_safeMode)
        return;

    IFile f;
    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                               fsMakePath(PATH_ASCII, COLORS_PATH), FS_OPEN_READ)))
    {
        char buf[160];
        u64 total = 0;

        memset(buf, 0, sizeof(buf));
        if (R_SUCCEEDED(IFile_Read(&f, &total, buf, sizeof(buf) - 1)) && total > 0)
        {
            u32 nums[13];
            u32 cnt = 0;
            u32 i = 0;

            while (buf[i] && cnt < 13)
            {
                while (buf[i] == ' ' || buf[i] == '\r' || buf[i] == '\n')
                    ++i;
                if (buf[i] < '0' || buf[i] > '9')
                    break;

                u32 v = 0;
                while (buf[i] >= '0' && buf[i] <= '9')
                {
                    v = v * 10 + (u32)(buf[i] - '0');
                    ++i;
                }
                nums[cnt++] = v;
            }

            if (cnt >= 12)
            {
                for (u32 r = 0; r < 4; ++r)
                {
                    g_colH[r] = nums[r * 3 + 0] % 360;
                    g_colS[r] = nums[r * 3 + 1] > 255 ? 255 : nums[r * 3 + 1];
                    g_colV[r] = nums[r * 3 + 2] > 255 ? 255 : nums[r * 3 + 2];
                }
                if((cnt < 13 || nums[12] != 2) && g_colH[2] == 180 && g_colS[2] == 255 && g_colV[2] == 255)
                {
                    if(g_colH[0] == 180 && g_colS[0] == 255 && g_colV[0] == 255 && g_colS[1] == 0 && g_colV[1] == 255 && g_colV[3] == 0)
                    {
                        g_colH[0] = g_colH[1] = 264;
                        g_colS[0] = g_colS[1] = 99;
                    }
                    g_colH[2] = g_colH[0];
                    g_colS[2] = g_colS[0];
                    g_colV[2] = g_colV[0];
                }
                ColorsApply();
            }
        }
        IFile_Close(&f);
    }
}

void AccentColor_Refresh(void)
{
    AccentColor_EnsureLoaded();
    if(!g_safeMode) ColorsApply();
}

static void SdmcDeleteFile(const char *path)
{
    FS_Archive archive = 0;
    if (R_SUCCEEDED(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
    {
        FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, path));
        FSUSER_CloseArchive(archive);
    }
}

static void ColorsSave(void)
{
    SdmcDeleteFile(COLORS_PATH);

    IFile f;
    if (R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, COLORS_PATH), FS_OPEN_CREATE | FS_OPEN_WRITE)))
        return;

    char buf[160];
    u64 total = 0;
    u32 len = (u32)sprintf(buf, "%u %u %u\n%u %u %u\n%u %u %u\n%u %u %u\n2\n",
        (unsigned)g_colH[0], (unsigned)g_colS[0], (unsigned)g_colV[0],
        (unsigned)g_colH[1], (unsigned)g_colS[1], (unsigned)g_colV[1],
        (unsigned)g_colH[2], (unsigned)g_colS[2], (unsigned)g_colV[2],
        (unsigned)g_colH[3], (unsigned)g_colS[3], (unsigned)g_colV[3]);
    IFile_Write(&f, &total, buf, len, FS_WRITE_FLUSH);
    IFile_Close(&f);
}

void RosalinaMenu_AccentColor(void)
{
    AccentColor_EnsureLoaded();

    u32 H[4], S[4], V[4];
    for (u32 i = 0; i < 4; ++i) { H[i] = g_colH[i]; S[i] = g_colS[i]; V[i] = g_colV[i]; }

    static const char *const names[4] = { "accent", "text", "cursor", "bg" };
    u32 target = 0;

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_Unlock();

    do
    {
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        if (down & KEY_SELECT)
            target = (target + 1) & 3;

        if (down & KEY_X)
        {
            for (u32 i = 0; i < 4; ++i)
            {
                H[i] = i < 3 ? 264 : 0;
                S[i] = i < 3 ? 99 : 0;
                V[i] = i < 3 ? 255 : 0;
            }
        }

        if (held & KEY_TOUCH)
        {
            u32 previousH = H[0], previousS = S[0], previousV = V[0];
            touchPosition tp;
            hidTouchRead(&tp);
            u32 px = tp.px;
            u32 py = tp.py;

            if (px >= HUE_X && px < HUE_X + HUE_W && py + 4 >= HUE_Y && py <= HUE_Y + HUE_H + 4)
                H[target] = (px - HUE_X) * 360 / HUE_W;
            else if (px >= SV_X && px < SV_X + SV_W && py >= SV_Y && py < SV_Y + SV_H)
            {
                S[target] = (px - SV_X) * 255 / (SV_W - 1);
                V[target] = 255 - (py - SV_Y) * 255 / (SV_H - 1);
            }
            else if (px >= PRE_X && px < PRE_X + PRE_W && py >= PRE_Y && py < PRE_Y + PRE_N * PRE_H)
            {
                u32 idx = (py - PRE_Y) / PRE_H;
                if (idx < PRE_N) { H[target] = g_presetHSV[idx][0]; S[target] = g_presetHSV[idx][1]; V[target] = g_presetHSV[idx][2]; }
            }

            if (H[target] > 359) H[target] = 359;
            if (S[target] > 255) S[target] = 255;
            if (V[target] > 255) V[target] = 255;
            if (target == 0 && (H[0] != previousH || S[0] != previousS || V[0] != previousV))
            {
                H[2] = H[0];
                S[2] = S[0];
                V[2] = V[0];
            }
        }

        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_WHITE, Lang_Translate("Colors"));
        Draw_DrawFormattedString(90, 10, COLOR_WHITE, "SELECT: %s    ", Lang_Translate(names[target]));
        AccentDrawBase(H[target], S[target], V[target]);
        Draw_DrawFormattedString(110, PRV_Y, COLOR_WHITE, "H %u    ", (unsigned)H[target]);
        Draw_DrawFormattedString(110, PRV_Y + SPACING_Y, COLOR_WHITE, "S %u  V %u    ", (unsigned)S[target], (unsigned)V[target]);
        Draw_DrawString(110, PRV_Y + 2 * SPACING_Y + 2, COLOR_WHITE, Lang_Translate("X: defaults  A: save  B: cancel"));
        Draw_DrawString(10, 220, COLOR_WHITE, Lang(STR_TOUCH_BAR_BOX_SWATCHES_TO_PICK));
        Draw_FlushFramebuffer();
        Draw_Unlock();

        if (down & KEY_A)
        {
            for (u32 i = 0; i < 4; ++i) { g_colH[i] = H[i]; g_colS[i] = S[i]; g_colV[i] = V[i]; }
            ColorsApply();
            ColorsSave();
            break;
        }
        if (down & KEY_B)
            break;

        svcSleepThread(20 * 1000 * 1000LL);
    }
    while (!menuShouldExit);

}

static u64 Checker_GetForegroundTitleId(void)
{
    u32 pidList[0x40];
    s32 count = 0;

    if (R_FAILED(svcGetProcessList(&count, pidList, 0x40)))
        return 0;

    for (s32 i = 0; i < count; ++i)
    {
        Handle h;
        if (R_FAILED(svcOpenProcess(&h, pidList[i])))
            continue;

        u64 tid = 0;
        svcGetProcessInfo((s64 *)&tid, h, 0x10001);
        svcCloseHandle(h);

        u32 hi = (u32)(tid >> 32);
        if (hi == 0x00040010 || hi == 0x00040000)
            return tid;
    }

    return 0;
}

static bool Checker_HasPlugin(u64 tid, char *outName, u32 outSize)
{
    char        dirPath[64];
    FS_Archive  arch = 0;
    Handle      dir = 0;
    bool        found = false;

    sprintf(dirPath, "/luma/plugins/%016llX", tid);

    if (R_FAILED(FSUSER_OpenArchive(&arch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
        return false;

    if (R_SUCCEEDED(FSUSER_OpenDirectory(&dir, arch, fsMakePath(PATH_ASCII, dirPath))))
    {
        u32                 n = 0;
        FS_DirectoryEntry   entry;
        static const u16    ext[] = u".3gx";

        while (!found && R_SUCCEEDED(FSDIR_Read(dir, &n, 1, &entry)) && n > 0)
        {
            if (entry.attributes & 1)
                continue;

            u32 len = 0;
            while (entry.name[len])
                ++len;
            if (len <= 4 || entry.name[0] == u'.')
                continue;

            if (memcmp(entry.name + len - 4, ext, 4 * sizeof(u16)) == 0)
            {
                if (outName && outSize > 0)
                {
                    memset(outName, 0, outSize);
                    utf16_to_utf8((u8 *)outName, entry.name, outSize - 1);
                }
                found = true;
            }
        }
        FSDIR_Close(dir);
    }

    FSUSER_CloseArchive(arch);
    return found;
}

static bool Checker_HasCheats(u64 tid, char *outPath, u32 outSize)
{
    char  path[64];
    IFile f;

    sprintf(path, "/luma/titles/%016llX/cheats.txt", tid);
    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                               fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
    {
        IFile_Close(&f);
        if (outPath && outSize > 0) { strncpy(outPath, path, outSize - 1); outPath[outSize - 1] = 0; }
        return true;
    }

    sprintf(path, "/cheats/%016llX.txt", tid);
    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                               fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
    {
        IFile_Close(&f);
        if (outPath && outSize > 0) { strncpy(outPath, path, outSize - 1); outPath[outSize - 1] = 0; }
        return true;
    }

    return false;
}

static void Checker_DeleteReport(u64 tid)
{
    char path[64];

    sprintf(path, "/luma/%016llX.txt", tid);
    SdmcDeleteFile(path);
}

static void Checker_WriteReport(u64 tid, bool hasPlugin, bool hasCheats)
{
    char  path[64];
    IFile f;
    u64   total = 0;
    char  buf[512];

    Checker_DeleteReport(tid);

    sprintf(path, "/luma/%016llX.txt", tid);

    if (R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, path), FS_OPEN_CREATE | FS_OPEN_WRITE)))
        return;

    int n = sprintf(buf,
        "Title ID: %016llX\r\n\r\n"
        "3gx plugin: %s\r\n"
        "  put a .3gx file in this folder:\r\n"
        "  sdmc:/luma/plugins/%016llX/\r\n\r\n"
        "cheats txt: %s\r\n"
        "  put the cheats file at either path:\r\n"
        "  sdmc:/luma/titles/%016llX/cheats.txt\r\n"
        "  sdmc:/cheats/%016llX.txt\r\n",
        tid,
        hasPlugin ? "FOUND" : Lang_Translate("MISSING"), tid,
        hasCheats ? "FOUND" : Lang_Translate("MISSING"), tid, tid);

    IFile_Write(&f, &total, buf, (u32)n, FS_WRITE_FLUSH);
    IFile_Close(&f);
}

void RosalinaMenu_3gxTxtChecker(void)
{
    u64  tid = Checker_GetForegroundTitleId();
    char pluginName[64] = {0};
    char cheatsPath[64] = {0};
    bool hasPlugin = false;
    bool hasCheats = false;

    if (tid != 0)
    {
        hasPlugin = Checker_HasPlugin(tid, pluginName, sizeof(pluginName));
        hasCheats = Checker_HasCheats(tid, cheatsPath, sizeof(cheatsPath));

        if (hasPlugin && hasCheats)
            Checker_DeleteReport(tid);
        else
        {
            Checker_WriteReport(tid, hasPlugin, hasCheats);
            LogLine("[チェッカー: 3gx またはチートファイルが不足しています] / [checker: missing 3gx or cheats txt]");
        }
    }


    do
    {
        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("3gx / txt checker"));

        if (tid == 0)
        {
            Draw_DrawString(10, 40, COLOR_RED, Lang(STR_NO_GAME_IS_RUNNING));
            Draw_DrawString(10, 40 + SPACING_Y, COLOR_WHITE, Lang(STR_OPEN_A_GAME_FIRST_THEN_RUN_THIS));
        }
        else
        {
            u32 y = 34;
            Draw_DrawFormattedString(10, y, COLOR_WHITE, Lang_Translate("Title ID: %016llX"), tid);
            y += SPACING_Y + 6;

            Draw_DrawFormattedString(10, y, hasPlugin ? COLOR_LIME : COLOR_RED,
                "3gx : %s", hasPlugin ? Lang_Translate("found") : Lang_Translate("MISSING"));
            y += SPACING_Y;
            if (hasPlugin)
                Draw_DrawFormattedString(20, y, COLOR_WHITE, "%s", pluginName);
            else
                Draw_DrawFormattedString(20, y, COLOR_WHITE, "/luma/plugins/%016llX/", tid);
            y += SPACING_Y + 4;

            Draw_DrawFormattedString(10, y, hasCheats ? COLOR_LIME : COLOR_RED,
                "txt : %s", hasCheats ? Lang_Translate("found") : Lang_Translate("MISSING"));
            y += SPACING_Y;
            if (hasCheats)
            {
                Draw_DrawFormattedString(20, y, COLOR_WHITE, "%s", cheatsPath);
                y += SPACING_Y;
            }
            else
            {
                Draw_DrawFormattedString(20, y, COLOR_WHITE, "/luma/titles/%016llX/cheats.txt", tid);
                y += SPACING_Y;
                Draw_DrawFormattedString(20, y, COLOR_WHITE, "/cheats/%016llX.txt", tid);
                y += SPACING_Y;
            }
            y += 6;

            if (hasPlugin && hasCheats)
                Draw_DrawString(10, y, COLOR_LIME, Lang(STR_ALL_PRESENT_REPORT_REMOVED));
            else
                Draw_DrawFormattedString(10, y, COLOR_YELLOW, Lang_Translate("Wrote /luma/%016llX.txt"), tid);
        }

        Draw_DrawString(10, 220, COLOR_WHITE, Lang_Translate("B: back"));
        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while (!(waitInput() & KEY_B) && !menuShouldExit);

}

void LogLine(const char *msg)
{
    IFile f;
    u64   size = 0;

    if (R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, "/luma/log.txt"), FS_OPEN_CREATE | FS_OPEN_WRITE)))
        return;

    IFile_GetSize(&f, &size);

    if (size > 65536)
    {
        IFile_Close(&f);
        SdmcDeleteFile("/luma/log.txt");
        if (R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                                fsMakePath(PATH_ASCII, "/luma/log.txt"), FS_OPEN_CREATE | FS_OPEN_WRITE)))
            return;
        size = 0;
    }

    f.pos = size;

    char line[256];
    u64  wr = 0;
    u32  n = (u32)sprintf(line, "%s\r\n", msg);
    IFile_Write(&f, &wr, line, n, FS_WRITE_FLUSH);
    IFile_Close(&f);
}

static u64 SD_FreeBytes(void)
{
    FS_ArchiveResource res;
    if (R_FAILED(FSUSER_GetArchiveResource(&res, SYSTEM_MEDIATYPE_SD)))
        return (u64)-1;
    return (u64)res.freeClusters * (u64)res.clusterSize;
}

static bool PathExists(const char *path, bool dir)
{
    if (dir)
    {
        FS_Archive a = 0;
        Handle     h = 0;
        bool       ok = false;

        if (R_SUCCEEDED(FSUSER_OpenArchive(&a, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
        {
            if (R_SUCCEEDED(FSUSER_OpenDirectory(&h, a, fsMakePath(PATH_ASCII, path))))
            {
                FSDIR_Close(h);
                ok = true;
            }
            FSUSER_CloseArchive(a);
        }
        return ok;
    }

    IFile f;
    if (R_SUCCEEDED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                               fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
    {
        IFile_Close(&f);
        return true;
    }
    return false;
}

void SDSpaceWarnIfLow(void)
{
    static bool warned = false;
    if (warned)
        return;
    warned = true;

    u64 freeBytes = SD_FreeBytes();
    if (freeBytes == (u64)-1 || freeBytes >= 100ULL * 1024 * 1024)
        return;

    LogLine("[警告: SD カードの空き容量が不足しています] / [warning: low SD space]");

    do
    {
        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 10, COLOR_RED, Lang_Translate("Low SD space"));
        Draw_DrawFormattedString(10, 40, COLOR_WHITE, Lang(STR_FREE_MB), (unsigned)(freeBytes >> 20));
        Draw_DrawString(10, 40 + SPACING_Y, COLOR_WHITE, Lang_Translate("Consider freeing some space"));
        Draw_DrawString(10, 40 + 4 * SPACING_Y, COLOR_WHITE, Lang_Translate("B: continue"));
        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while (!(waitInput() & KEY_B) && !menuShouldExit);
}

static u32 CheckSdCard_Firm(const char *path, u8 *header)
{
    IFile   f;
    u64     size = 0, total = 0;
    u32     status = 2;

    if (R_FAILED(IFile_Open(&f, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
        return 0;

    IFile_GetSize(&f, &size);
    if (size > 0x200 && R_SUCCEEDED(IFile_Read(&f, &total, header, 0x200)) && total == 0x200
        && memcmp(header, "FIRM", 4) == 0)
    {
        status = 1;

        for (u32 i = 0; i < 4 && status == 1; ++i)
        {
            const u8 *section = header + 0x40 + i * 0x30;
            u32     offset, length;
            u8      digest[32];
            Sha256  ctx;

            memcpy(&offset, section, 4);
            memcpy(&length, section + 8, 4);
            if (length == 0)
                continue;

            if (offset < 0x200 || offset + length < offset || offset + length > size)
            {
                status = 2;
                break;
            }

            Sha256_Init(&ctx);
            f.pos = offset;
            while (length != 0 && status == 1)
            {
                u32 n = length < MENU_WORK_BUF_SIZE ? length : MENU_WORK_BUF_SIZE;

                if (R_FAILED(IFile_Read(&f, &total, g_menuWorkBuf, n)) || total != n)
                    status = 2;
                else
                    Sha256_Update(&ctx, g_menuWorkBuf, n);
                length -= n;
            }

            Sha256_Final(&ctx, digest);
            if (status == 1 && memcmp(digest, section + 0x10, 32) != 0)
                status = 2;
        }
    }

    IFile_Close(&f);
    return status;
}

void RosalinaMenu_CheckSdCard(void)
{
    static const char *const paths[] = {
        "/luma", "/luma/plugins", "/3ds/luma/9.1/boot.firm", "/3ds/luma/13.4/boot.firm", "/boot.firm"
    };
    static const char *const marks[] = { "--", "OK", "NG" };
    static u8 headers[3][0x200];
    const char *active = "";
    u32 status[5];

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Check SD Card"));
    Draw_DrawString(10, 34, COLOR_WHITE, Lang(STR_CHECKING_FILES));
    Draw_FlushFramebuffer();
    Draw_Unlock();

    for (u32 i = 0; i < 2; ++i)
        status[i] = PathExists(paths[i], true) ? 1 : 0;
    for (u32 i = 2; i < 5; ++i)
        status[i] = CheckSdCard_Firm(paths[i], headers[i - 2]);

    if (status[4] == 1 && status[2] == 1 && memcmp(headers[2], headers[0], 0x200) == 0)
        active = " (v9.1)";
    else if (status[4] == 1 && status[3] == 1 && memcmp(headers[2], headers[1], 0x200) == 0)
        active = " (v13.4Y)";

    u64 freeBytes = SD_FreeBytes();

    do
    {
        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Check SD Card"));

        u32 y = 34;
        for (u32 i = 0; i < 5; ++i)
        {
            Draw_DrawFormattedString(10, y, status[i] == 1 ? COLOR_LIME : COLOR_RED, "[%s] %s%s",
                marks[status[i]], paths[i], i == 4 ? active : "");
            y += SPACING_Y;
        }
        y += 4;
        if (freeBytes == (u64)-1)
            Draw_DrawString(10, y, COLOR_WHITE, Lang(STR_SD_FREE_N_A));
        else
            Draw_DrawFormattedString(10, y, COLOR_WHITE, Lang(STR_SD_FREE_MB), (unsigned)(freeBytes >> 20));
        Draw_DrawString(10, y + 2 * SPACING_Y, COLOR_WHITE, Lang(STR_SD_CHECK_LEGEND));

        Draw_DrawString(10, 220, COLOR_WHITE, Lang_Translate("B: back"));
        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while (!(waitInput() & KEY_B) && !menuShouldExit);
}

#define FB_MAX 256
#define FB_NAME_COLS ((SCROLL_MARK_X - 10) / SPACING_X - 3)
static char               g_fbName[FB_MAX][64];
static bool               g_fbDir[FB_MAX];
static u64                g_fbSize[FB_MAX];
static FS_DirectoryEntry  g_fbEntries[16];

static u32 FileBrowser_Load(const char *path)
{
    u32 count = 0;

    for (int wantDir = 1; wantDir >= 0; --wantDir)
    {
        FS_Archive arch = 0;
        Handle     dir = 0;

        if (R_FAILED(FSUSER_OpenArchive(&arch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
            break;

        if (R_SUCCEEDED(FSUSER_OpenDirectory(&dir, arch, fsMakePath(PATH_ASCII, path))))
        {
            u32 n = 0;
            while (count < FB_MAX && R_SUCCEEDED(FSDIR_Read(dir, &n, 16, g_fbEntries)) && n > 0)
            {
                for (u32 i = 0; i < n && count < FB_MAX; ++i)
                {
                    bool isDir = (g_fbEntries[i].attributes & 1) != 0;
                    if ((int)isDir != wantDir)
                        continue;

                    int units = utf16_to_utf8((u8 *)g_fbName[count], g_fbEntries[i].name, sizeof(g_fbName[0]) - 1);
                    if (units < 0 || (u32)units >= sizeof(g_fbName[0]))
                        continue;
                    g_fbName[count][units] = 0;
                    g_fbDir[count] = isDir;
                    g_fbSize[count] = g_fbEntries[i].fileSize;
                    ++count;
                }
            }
            FSDIR_Close(dir);
        }

        FSUSER_CloseArchive(arch);
    }

    return count;
}

#define FB_CLIP_NONE  0
#define FB_CLIP_COPY  1
#define FB_CLIP_CUT   2

static char g_fbClipPath[512];
static bool g_fbClipIsDir;
static u32  g_fbClipMode;

static bool FileBrowser_Join(char *dst, u32 dstSize, const char *dir, const char *name)
{
    u32 i = 0, j = 0;

    while (dir[i] != 0 && i + 1 < dstSize)
    {
        dst[i] = dir[i];
        ++i;
    }
    if (i == 0 || dst[i - 1] != '/')
    {
        if (i + 1 < dstSize)
            dst[i++] = '/';
    }
    while (name[j] != 0 && i + 1 < dstSize)
        dst[i++] = name[j++];

    dst[i] = 0;
    return name[j] == 0;
}

static const char *FileBrowser_BaseName(const char *path)
{
    const char *base = path;

    for (const char *p = path; *p != 0; ++p)
        if (*p == '/' && p[1] != 0)
            base = p + 1;

    return base;
}

static bool FileBrowser_IsPrefixOf(const char *prefix, const char *path)
{
    u32 i = 0;

    while (prefix[i] != 0)
    {
        if (path[i] != prefix[i])
            return false;
        ++i;
    }

    return path[i] == 0 || path[i] == '/';
}

static Result __attribute__((noinline)) FileBrowser_CopyFile(const char *src, const char *dst)
{
    IFile in, out;
    Result res;
    u64 size = 0;

    res = IFile_Open(&in, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                     fsMakePath(PATH_ASCII, src), FS_OPEN_READ);
    if (R_FAILED(res))
        return res;

    res = IFile_GetSize(&in, &size);
    if (R_FAILED(res))
    {
        IFile_Close(&in);
        return res;
    }

    SdmcDeleteFile(dst);

    res = IFile_Open(&out, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                     fsMakePath(PATH_ASCII, dst), FS_OPEN_CREATE | FS_OPEN_WRITE);
    if (R_FAILED(res))
    {
        IFile_Close(&in);
        return res;
    }

    in.pos = 0;
    out.pos = 0;

    while (in.pos < size)
    {
        u64 read = 0, written = 0;

        res = IFile_Read(&in, &read, g_menuWorkBuf, sizeof(g_menuWorkBuf));
        if (R_FAILED(res))
            break;
        if (read == 0)
        {
            res = -1;
            break;
        }

        res = IFile_Write(&out, &written, g_menuWorkBuf, (u32)read, FS_WRITE_FLUSH);
        if (R_FAILED(res))
            break;
    }

    IFile_Close(&out);
    IFile_Close(&in);

    if (R_FAILED(res))
        SdmcDeleteFile(dst);

    return res;
}

static Result FileBrowser_CopyTree(char *src, char *dst, u32 size, bool isDir, u32 depth)
{
    static char name[256];
    FS_Archive archive = 0;
    Result res;

    if (!isDir)
        return FileBrowser_CopyFile(src, dst);

    if (depth > 8)
        return -1;

    res = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    if (R_FAILED(res))
        return res;

    FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, dst), 0);

    Handle dir = 0;
    res = FSUSER_OpenDirectory(&dir, archive, fsMakePath(PATH_ASCII, src));
    if (R_FAILED(res))
    {
        FSUSER_CloseArchive(archive);
        return res;
    }

    u32 srcLen = strlen(src);
    u32 dstLen = strlen(dst);

    for (;;)
    {
        FS_DirectoryEntry *entry = &g_fbEntries[0];
        u32 n = 0;

        res = FSDIR_Read(dir, &n, 1, entry);
        if (R_FAILED(res) || n == 0)
            break;

        int units = utf16_to_utf8((u8 *)name, entry->name, sizeof(name) - 1);
        if (units < 0 || (u32)units >= sizeof(name))
        {
            res = -1;
            break;
        }
        name[units] = 0;

        if (!FileBrowser_Join(src, size, src, name) || !FileBrowser_Join(dst, size, dst, name))
            res = -1;
        else
            res = FileBrowser_CopyTree(src, dst, size, (entry->attributes & 1) != 0, depth + 1);

        src[srcLen] = 0;
        dst[dstLen] = 0;

        if (R_FAILED(res))
            break;
    }

    FSDIR_Close(dir);
    FSUSER_CloseArchive(archive);

    return res;
}

static Result FileBrowser_Delete(const char *path, bool isDir)
{
    FS_Archive archive = 0;
    Result res;

    res = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    if (R_FAILED(res))
        return res;

    if (isDir)
        res = FSUSER_DeleteDirectoryRecursively(archive, fsMakePath(PATH_ASCII, path));
    else
        res = FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, path));

    FSUSER_CloseArchive(archive);
    return res;
}

static Result FileBrowser_Rename(const char *src, const char *dst, bool isDir)
{
    FS_Archive archive = 0;
    Result res;

    res = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    if (R_FAILED(res))
        return res;

    if (isDir)
        res = FSUSER_RenameDirectory(archive, fsMakePath(PATH_ASCII, src), archive, fsMakePath(PATH_ASCII, dst));
    else
        res = FSUSER_RenameFile(archive, fsMakePath(PATH_ASCII, src), archive, fsMakePath(PATH_ASCII, dst));

    FSUSER_CloseArchive(archive);
    return res;
}

static Result FileBrowser_MakeDir(const char *path)
{
    FS_Archive archive = 0;
    Result res;

    res = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    if (R_FAILED(res))
        return res;

    res = FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, path), 0);
    FSUSER_CloseArchive(archive);
    return res;
}

static void FileBrowser_Notify(const char *title, const char *msg, bool ok)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_DrawString(10, 4, COLOR_TITLE, Lang_Translate(title));
    Draw_DrawString(10, 4 + 3 * SPACING_Y, ok ? COLOR_LIME : COLOR_RED, msg);
    Draw_DrawString(10, 220, COLOR_WHITE, Lang(STR_PRESS_ANY_BUTTON));
    Draw_FlushFramebuffer();
    Draw_Unlock();
    waitInput();
}

static void FileBrowser_Paste(const char *destDir)
{
    char dst[512];
    Result res;

    if (g_fbClipMode == FB_CLIP_NONE)
    {
        FileBrowser_Notify( Lang_Translate("Paste"), Lang_Translate("Clipboard is empty."), false);
        return;
    }

    if (!FileBrowser_Join(dst, sizeof(dst), destDir, FileBrowser_BaseName(g_fbClipPath)))
    {
        FileBrowser_Notify( Lang_Translate("Paste"), Lang_Translate("Path is too long."), false);
        return;
    }

    if (strcmp(dst, g_fbClipPath) == 0)
    {
        FileBrowser_Notify( Lang_Translate("Paste"), Lang_Translate("Source and destination are the same."), false);
        return;
    }

    if (g_fbClipIsDir && FileBrowser_IsPrefixOf(g_fbClipPath, destDir))
    {
        FileBrowser_Notify( Lang_Translate("Paste"), Lang_Translate("Cannot paste a folder into itself."), false);
        return;
    }

    if (g_fbClipMode == FB_CLIP_CUT)
    {
        res = FileBrowser_Rename(g_fbClipPath, dst, g_fbClipIsDir);
        if (R_SUCCEEDED(res))
        {
            g_fbClipMode = FB_CLIP_NONE;
            FileBrowser_Notify( Lang_Translate("Move"), Lang_Translate("Moved."), true);
            return;
        }
    }

    res = FileBrowser_CopyTree(g_fbClipPath, dst, sizeof(dst), g_fbClipIsDir, 0);

    if (R_FAILED(res))
    {
        FileBrowser_Notify( Lang_Translate("Paste"), Lang_Translate("Failed."), false);
        return;
    }

    if (g_fbClipMode == FB_CLIP_CUT)
    {
        if (R_FAILED(FileBrowser_Delete(g_fbClipPath, g_fbClipIsDir)))
        {
            FileBrowser_Notify( Lang_Translate("Move"), Lang_Translate("Copied, but failed to delete the source."), false);
            return;
        }
        g_fbClipMode = FB_CLIP_NONE;
        FileBrowser_Notify( Lang_Translate("Move"), Lang_Translate("Moved."), true);
    }
    else
    {
        FileBrowser_Notify( Lang_Translate("Copy"), Lang_Translate("Copied."), true);
    }
}

static void FileBrowser_Up(char *path)
{
    u32 l = 0;
    while (path[l]) ++l;
    if (l > 1)
    {
        --l;
        while (l > 0 && path[l - 1] != '/') --l;
        path[l] = 0;
    }
}

void RosalinaMenu_FileBrowser(void)
{
    static char path[512];
    path[0] = '/';
    path[1] = 0;

    u32 count = FileBrowser_Load(path);
    s32 sel = 0, scroll = 0;
    const s32 rows = 15;

    do
    {
        bool atRoot = (path[0] == '/' && path[1] == 0);
        s32  total = (s32)count + (atRoot ? 0 : 1);

        if (sel >= total) sel = total > 0 ? total - 1 : 0;
        if (sel < 0) sel = 0;
        if (sel < scroll) scroll = sel;
        if (sel >= scroll + rows) scroll = sel - rows + 1;

        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 4, COLOR_TITLE, Lang_Translate("File browser"));

        u32 plen = 0; while (path[plen]) ++plen;
        const char *pshow = path;
        if (plen > 48) pshow = path + (plen - 48);
        Draw_DrawFormattedString(10, 4 + SPACING_Y, COLOR_WHITE, "%s%s   ", plen > 48 ? "..." : "", pshow);

        u32 y = 4 + 3 * SPACING_Y;
        for (s32 r = 0; r < rows; ++r)
        {
            s32 idx = scroll + r;
            if (idx >= total) break;

            u32 col = (idx == sel) ? COLOR_LIME : COLOR_WHITE;
            const char *cur = (idx == sel) ? ">" : " ";

            if (!atRoot && idx == 0)
                Draw_DrawFormattedString(10, y, col, "%s [..]", cur);
            else
            {
                s32 e = idx - (atRoot ? 0 : 1);
                if (g_fbDir[e])
                    Draw_DrawFormattedString(10, y, col, "%s [%.*s]", cur, (int)(FB_NAME_COLS - 2), g_fbName[e]);
                else
                {
                    char sz[16];
                    u32  szLen = sprintf(sz, "  (%u KB)", (unsigned)(g_fbSize[e] >> 10));
                    Draw_DrawFormattedString(10, y, col, "%s %.*s%s", cur, (int)(FB_NAME_COLS - szLen), g_fbName[e], sz);
                }
            }
            y += SPACING_Y;
        }
        Draw_DrawScrollMarks(4 + 3 * SPACING_Y, rows, scroll, total, g_cursorColor);

        Draw_DrawFormattedString(10, 203, COLOR_WHITE, Lang_Translate("Clipboard: %s%s   "),
            g_fbClipMode == FB_CLIP_NONE ? Lang_Translate("(empty)") : (g_fbClipMode == FB_CLIP_CUT ? Lang_Translate("[cut] ") : Lang_Translate("[copy] ")),
            g_fbClipMode == FB_CLIP_NONE ? "" : FileBrowser_BaseName(g_fbClipPath));
        Draw_DrawString(10, 214, COLOR_WHITE, Lang(STR_A_OPEN_X_COPY_Y_CUT_R_PASTE));
        Draw_DrawString(10, 225, COLOR_WHITE, Lang(STR_SELECT_DELETE_START_RENAME_L_NEW_DIR));
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 k = waitInput();
        if (menuShouldExit)
            break;

        if (k & KEY_DOWN)      { if (sel + 1 < total) ++sel; }
        else if (k & KEY_UP)   { if (sel > 0) --sel; }
        else if (k & KEY_A)
        {
            if (!atRoot && sel == 0)
            {
                FileBrowser_Up(path);
                count = FileBrowser_Load(path); sel = 0; scroll = 0;
            }
            else
            {
                s32 e = sel - (atRoot ? 0 : 1);
                if (e >= 0 && e < (s32)count && g_fbDir[e])
                {
                    u32 l = 0; while (path[l]) ++l;
                    const char *nm = g_fbName[e];
                    u32 nl = 0; while (nm[nl]) ++nl;
                    if (l + nl + 2 < sizeof(path))
                    {
                        for (u32 i = 0; i < nl; ++i) path[l + i] = nm[i];
                        path[l + nl] = '/';
                        path[l + nl + 1] = 0;
                        count = FileBrowser_Load(path); sel = 0; scroll = 0;
                    }
                }
            }
        }
        else if (k & KEY_B)
        {
            if (atRoot) break;
            FileBrowser_Up(path);
            count = FileBrowser_Load(path); sel = 0; scroll = 0;
        }
        else if (k & KEY_R)
        {
            FileBrowser_Paste(path);
            count = FileBrowser_Load(path);
        }
        else if (k & KEY_L)
        {
            char name[64];

            if (TextInput_Prompt( Lang_Translate("New folder name"), name, sizeof(name), NULL))
            {
                char full[512];
                FileBrowser_Join(full, sizeof(full), path, name);

                if (R_SUCCEEDED(FileBrowser_MakeDir(full)))
                    count = FileBrowser_Load(path);
                else
                    FileBrowser_Notify( Lang_Translate("New folder"), Lang_Translate("Failed."), false);
            }
        }
        else if (k & (KEY_X | KEY_Y | KEY_SELECT | KEY_START))
        {
            s32 e = sel - (atRoot ? 0 : 1);

            if (e < 0 || e >= (s32)count)
                continue;

            char full[512];
            bool isDir = g_fbDir[e];
            FileBrowser_Join(full, sizeof(full), path, g_fbName[e]);

            if (k & (KEY_X | KEY_Y))
            {
                u32 i = 0;
                while (full[i] != 0 && i + 1 < sizeof(g_fbClipPath))
                {
                    g_fbClipPath[i] = full[i];
                    ++i;
                }
                while (i > 1 && g_fbClipPath[i - 1] == '/')
                    --i;
                g_fbClipPath[i] = 0;
                g_fbClipIsDir = isDir;
                g_fbClipMode = (k & KEY_X) ? FB_CLIP_COPY : FB_CLIP_CUT;
            }
            else if (k & KEY_SELECT)
            {
                if (ConfirmPrompt( Lang_Translate("Delete"), Lang_Translate(isDir ? "Delete this folder and everything in it?" : "Delete this file?"), g_fbName[e]))
                {
                    if (R_SUCCEEDED(FileBrowser_Delete(full, isDir)))
                        count = FileBrowser_Load(path);
                    else
                        FileBrowser_Notify( Lang_Translate("Delete"), Lang_Translate("Failed."), false);
                }
            }
            else
            {
                char name[64];

                if (TextInput_Prompt( Lang_Translate("Rename"), name, sizeof(name), g_fbName[e]))
                {
                    char dst[512];
                    FileBrowser_Join(dst, sizeof(dst), path, name);

                    if (R_SUCCEEDED(FileBrowser_Rename(full, dst, isDir)))
                        count = FileBrowser_Load(path);
                    else
                        FileBrowser_Notify( Lang_Translate("Rename"), Lang_Translate("Failed."), false);
                }
            }
        }
    }
    while (!menuShouldExit);
}
