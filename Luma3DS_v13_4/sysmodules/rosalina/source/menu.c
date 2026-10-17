/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2022 Aurora Wright, TuxSH
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
#include "menu.h"
#include "menu_input.h"
#include "menus/controls.h"
#include "menus/theme.h"
#include "menus/battery.h"
#include "draw.h"
#include "menus/lang.h"
#include "fmt.h"
#include "memory.h"
#include "ifile.h"
#include "menus.h"
#include "utils.h"
#include "luma_config.h"
#include "menus/n3ds.h"
#include "menus/cheats.h"
#include "minisoc.h"
#include "plugin.h"
#include "menus/screen_filters.h"
#include "shell.h"



u32 menuCombo = 0;
bool isHidInitialized = false;
bool isQtmInitialized = false;
u32 mcuFwVersion = 0;
u8 mcuInfoTable[10] = {0};
bool mcuInfoTableRead = false;

const char *topScreenType = NULL;
const char *bottomScreenType = NULL;
bool areScreenTypesInitialized = false;



bool hidShouldUseIrrst(void)
{

    return false;
}

static inline u32 convertHidKeys(u32 keys)
{

    return keys;
}

void scanInputHook(void)
{
    hidScanInput();

#ifdef ROSALINA_MENU_SELF_SCREENSHOT

    u32 selfScreenshotCombo = KEY_L | KEY_DUP | KEY_SELECT;
    if ((hidKeysHeld() & selfScreenshotCombo) == selfScreenshotCombo && (hidKeysDown() & selfScreenshotCombo) != 0)
        menuTakeSelfScreenshot();
#endif
}

static u32 waitInputTimed(u32 *outHeldKeys, s32 msec)
{
    u64 started = svcGetSystemTick();
    u64 timeout = msec < 0 ? 0 : (u64)msec * SYSCLOCK_ARM11 / 1000;
    if(outHeldKeys) *outHeldKeys = 0;
    while(isHidInitialized && !menuShouldExit)
    {
        Draw_Lock();
        scanInputHook();
        u32 keys = convertHidKeys(hidKeysDown()) | (convertHidKeys(hidKeysDownRepeat()) & DIRECTIONAL_KEYS);
        if(outHeldKeys) *outHeldKeys = convertHidKeys(hidKeysHeld());
        Draw_Unlock();
        if(keys) return keys;
        if(msec >= 0 && (u64)(svcGetSystemTick() - started) >= timeout) return 0;
        svcSleepThread(4 * 1000 * 1000LL);
    }
    return 0;
}

u32 waitInputWithTimeout(s32 msec)
{
    return waitInputTimed(NULL, msec);
}

u32 waitInputWithTimeoutEx(u32 *outHeldKeys, s32 msec)
{
    return waitInputTimed(outHeldKeys, msec);
}

u32 waitInput(void)
{
    return waitInputWithTimeout(-1);
}

static u32 scanHeldKeys(void)
{
    u32 keys;

    Draw_Lock();

    if (!isHidInitialized || menuShouldExit)
        keys = 0;
    else
    {
        scanInputHook();
        keys = convertHidKeys(hidKeysHeld());
    }

    Draw_Unlock();
    return keys;
}

static u32 waitStableButtons(u32 timeout, bool waitForRelease)
{
    u64 started = svcGetSystemTick();
    u64 limit = (u64)timeout * SYSCLOCK_ARM11 / 1000;
    u32 candidate = 0, stable = 0, captured = 0;
    bool released = false;
    while (!menuShouldExit && isHidInitialized)
    {
        if (timeout && (u64)(svcGetSystemTick() - started) >= limit)
            return 0;
        u32 held = scanHeldKeys();
        if (!released)
            released = held == 0;
        else if (held == 0)
        {
            if (waitForRelease && captured)
                return captured;
            candidate = stable = 0;
        }
        else if (held != candidate)
        {
            candidate = held;
            stable = 0;
        }
        else if (++stable >= 2)
        {
            if (!waitForRelease)
                return held;
            captured |= held;
            stable = 2;
        }
        svcSleepThread(1 * 1000 * 1000LL);
    }
    return 0;
}

u32 waitComboWithTimeout(s32 msec)
{
    return msec == 0 ? 0 : waitStableButtons(msec < 0 ? 0 : (u32)msec, true);
}

u32 waitCombo(void)
{
    return waitComboWithTimeout(-1);
}

static MyThread menuThread;
static u8 CTR_ALIGN(8) menuThreadStack[0x3000];
static bool menuCloseRequested = false;

static float batteryPercentage;
static float batteryVoltage;
static u8 batteryTemperature;

static Result menuUpdateMcuInfo(void)
{
    Result res = 0;
    u8 data[4];

    if (!isServiceUsable("mcu::HWC"))
        return -1;

    Handle *mcuHwcHandlePtr = mcuHwcGetSessionHandle();
    *mcuHwcHandlePtr = 0;

    res = srvGetServiceHandle(mcuHwcHandlePtr, "mcu::HWC");

    if (R_FAILED(res))
        res = svcControlService(SERVICEOP_STEAL_CLIENT_SESSION, mcuHwcHandlePtr, "mcu::HWC");
    if (res != 0)
        return res;


    res = MCUHWC_ReadRegister(0xA, data, 4);

    if (R_SUCCEEDED(res))
    {
        batteryTemperature = data[0];



        batteryPercentage = data[1] + data[2] / 256.0f;
        batteryPercentage = (u32)((batteryPercentage + 0.05f) * 10.0f) / 10.0f;


        batteryVoltage = 0.02f * data[3];
        batteryVoltage = (u32)((batteryVoltage + 0.005f) * 100.0f) / 100.0f;
    }


    if (mcuFwVersion == 0)
    {
        u8 minor = 0, major = 0;
        MCUHWC_GetFwVerHigh(&major);
        MCUHWC_GetFwVerLow(&minor);


        mcuFwVersion = SYSTEM_VERSION(major - 0x10, minor, 0);
    }

    if (!mcuInfoTableRead)
        mcuInfoTableRead = R_SUCCEEDED(MCUHWC_ReadRegister(0x7F, mcuInfoTable, sizeof(mcuInfoTable)));

    svcCloseHandle(*mcuHwcHandlePtr);
    return res;
}

static const char *menuGetScreenTypeStr(u8 vendorId)
{
    switch (vendorId)
    {
        case 1:  return "IPS";
        case 12: return "TN";
        default: return "unknown";
    }
}

static void menuReadScreenTypes(void)
{
    if (areScreenTypesInitialized)
        return;

    if (!isN3DS)
    {

        topScreenType = "TN";
        bottomScreenType = "TN";
        areScreenTypesInitialized = true;
    }
    else
    {
        srvSetBlockingPolicy(false);

        Result res = gspLcdInit();
        if (R_SUCCEEDED(res))
        {
            u8 vendors = 0;
            if (R_SUCCEEDED(GSPLCD_GetVendors(&vendors)))
            {
                topScreenType = menuGetScreenTypeStr(vendors >> 4);
                bottomScreenType = menuGetScreenTypeStr(vendors & 0xF);
                areScreenTypesInitialized = true;
            }

            gspLcdExit();
        }

        srvSetBlockingPolicy(true);
    }
}

static void menuInitializeQtm(void)
{
    if (isQtmInitialized)
        return;


    Handle qtmHandle = 0;
    for (int i = 0; i < 30 && !qtmIsInitialized(); i++)
    {
        if (R_SUCCEEDED(svcControlService(SERVICEOP_STEAL_CLIENT_SESSION, &qtmHandle, "qtm:sp")))
            *qtmGetSessionHandle() = qtmHandle;
        else
            svcSleepThread(100 * 100 * 1000LL);
    }

    isQtmInitialized = qtmIsInitialized();
}

static inline bool menuItemIsHidden(const MenuItem *item)
{
    return item->visibility != NULL && !item->visibility();
}

bool menuCheckN3ds(void)
{
    return isN3DS;
}

u32 menuCountItems(const Menu *menu)
{
    u32 n;
    for (n = 0; menu->items[n].action_type != MENU_END; n++);
    return n;
}

MyThread *menuCreateThread(void)
{
    if(R_FAILED(MyThread_Create(&menuThread, menuThreadMain, menuThreadStack, 0x3000, 52, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);
    return &menuThread;
}

u32 menuCombo;
u32 g_blockMenuOpen = 0;

void menuThreadMain(void)
{
    while (!isServiceUsable("ac:u") || !isServiceUsable("hid:USER") || !isServiceUsable("gsp::Gpu") || !isServiceUsable("gsp::Lcd") || !isServiceUsable("cdc:CHK"))
        svcSleepThread(250 * 1000 * 1000LL);

    if (isN3DS)
    {
        while (!isServiceUsable("qtm:u"))
            svcSleepThread(250 * 1000 * 1000LL);
        menuInitializeQtm();
        N3DSMenu_UpdateStatus();
    }

    handleShellOpened();

    hidInit();
    isHidInitialized = true;

    menuReadScreenTypes();

    while(!preTerminationRequested)
    {
        svcSleepThread(50 * 1000 * 1000LL);
        if (menuShouldExit)
            continue;

        Cheat_ApplyCheats();
        Battery_Sample();

        u32 kHeld = scanHeldKeys();

        if(((kHeld & menuCombo) == menuCombo) && !g_blockMenuOpen)
        {
            menuEnter();
            if(isN3DS) N3DSMenu_UpdateStatus();
            PluginLoader__UpdateMenu();
            menuShow(&rosalinaMenu);
            menuLeave();
        }

        if (saveSettingsRequest) {
            LumaConfig_SaveSettings();
            saveSettingsRequest = false;
        }
    }
}

static s32 menuRefCount = 0;
void menuEnter(void)
{
    Draw_Lock();
    if(!menuShouldExit && menuRefCount == 0)
    {
        menuCloseRequested = false;
        menuRefCount++;
        svcKernelSetState(0x10000, 2 | 1);
        svcSleepThread(5 * 1000 * 100LL);
        if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
        {

            menuRefCount = 0;
            svcKernelSetState(0x10000, 2 | 1);
            svcSleepThread(5 * 1000 * 100LL);
        }
        else
            Draw_SetupFramebuffer();
    }
    Draw_Unlock();
}

void menuLeave(void)
{
    Draw_Lock();
    if(--menuRefCount == 0)
    {
        Draw_RestoreFramebufferAnimated(g_menuTransitions && !menuShouldExit);
        Draw_FreeFramebufferCache();
        svcKernelSetState(0x10000, 2 | 1);
    }
    Draw_Unlock();
}

void menuRequestClose(void)
{
    menuCloseRequested = true;
}

#define MENU_MAX_ROWS 16

static u32 menuScrollOffset;

u8 g_menuWorkBuf[MENU_WORK_BUF_SIZE];

typedef struct {
    Menu *menu;
    u32 selected;
    u32 scroll;
    u32 visibleMask;
    bool valid;
} MenuDrawState;

typedef struct {
    Menu *menu;
    u8 selected;
    u8 scroll;
} MenuHistory;

static u32 menuNowMs(void)
{
    return (u32)(svcGetSystemTick() / (SYSCLOCK_ARM11 / 1000));
}

static u32 menuVisibleItems(const Menu *menu, u32 *items)
{
    u32 count = 0;
    u32 total = menuCountItems(menu);
    for (u32 i = 0; i < total && i < 32; ++i)
        if (!menuItemIsHidden(&menu->items[i]))
            items[count++] = i;
    return count;
}

static u32 menuMoveSelection(const Menu *menu, u32 selected, s32 steps, bool wrap)
{
    u32 items[32];
    u32 count = menuVisibleItems(menu, items);
    if (count == 0)
        return 0;
    s32 position = 0;
    for (u32 i = 0; i < count; ++i)
        if (items[i] == selected)
            position = (s32)i;
    position += steps;
    if (wrap)
        position = (position % (s32)count + (s32)count) % (s32)count;
    else
    {
        if (position < 0)
            position = 0;
        if (position >= (s32)count)
            position = (s32)count - 1;
    }
    return items[position];
}

static char menuVersionString[16];
static u32 menuCommitHash;
static bool menuIsRelease;
static bool menuVersionLoaded;

static void menuReadVersion(void)
{
    if (menuVersionLoaded)
        return;
    s64 value;
    svcGetSystemInfo(&value, 0x10000, 0);
    u32 version = (u32)value;
    svcGetSystemInfo(&value, 0x10000, 1);
    menuCommitHash = (u32)value;
    svcGetSystemInfo(&value, 0x10000, 0x200);
    menuIsRelease = (bool)value;
    if (GET_VERSION_MAJOR(version) == 13 && GET_VERSION_MINOR(version) == 4 && GET_VERSION_REVISION(version) == 0)
        strcpy(menuVersionString, FORK_DISPLAY_VERSION);
    else if (GET_VERSION_REVISION(version) == 0)
        sprintf(menuVersionString, "v%lu.%lu", (unsigned long)GET_VERSION_MAJOR(version), (unsigned long)GET_VERSION_MINOR(version));
    else
        sprintf(menuVersionString, "v%lu.%lu.%lu", (unsigned long)GET_VERSION_MAJOR(version), (unsigned long)GET_VERSION_MINOR(version), (unsigned long)GET_VERSION_REVISION(version));
    menuVersionLoaded = true;
}

static bool menuStatusLoaded;
static u32 menuStatusUpdatedAt;
static Result menuMcuResult = -1;

static bool menuRefreshStatus(bool allowed)
{
    u32 now = menuNowMs();
    if (!allowed || (menuStatusLoaded && (u32)(now - menuStatusUpdatedAt) < 1000))
        return false;
    menuStatusUpdatedAt = now;
    menuStatusLoaded = true;
    menuMcuResult = menuUpdateMcuInfo();
    return true;
}

static u32 menuWaitNavigation(MenuInput *input)
{
    u32 start = menuNowMs();
    do
    {
        if (menuShouldExit || menuCloseRequested)
            return 0;
        u32 pressed = MenuInput_Update(input, scanHeldKeys(), menuNowMs(), DIRECTIONAL_KEYS);
        if (pressed)
            return pressed;
        svcSleepThread(4 * 1000 * 1000LL);
    }
    while ((u32)(menuNowMs() - start) < 32);
    return 0;
}

static void menuDraw(Menu *menu, u32 selected, bool refreshStatus, MenuDrawState *state)
{
    u32 items[32];
    u32 count = menuVisibleItems(menu, items);
    u32 selectedRow = 0;
    u32 previousRow = 0;
    u32 visibleMask = 0;
    for (u32 i = 0; i < count; ++i)
    {
        visibleMask |= 1u << items[i];
        if (items[i] == selected)
            selectedRow = i;
        if (items[i] == state->selected)
            previousRow = i;
    }
    if (count <= MENU_MAX_ROWS)
        menuScrollOffset = 0;
    else
    {
        if (selectedRow < menuScrollOffset)
            menuScrollOffset = selectedRow;
        else if (selectedRow >= menuScrollOffset + MENU_MAX_ROWS)
            menuScrollOffset = selectedRow - MENU_MAX_ROWS + 1;
        if (menuScrollOffset > count - MENU_MAX_ROWS)
            menuScrollOffset = count - MENU_MAX_ROWS;
    }
    bool full = !state->valid || state->menu != menu || state->visibleMask != visibleMask;
    bool scrolled = state->scroll != menuScrollOffset;
    bool selectionChanged = !state->valid || state->selected != selected;
    bool statusChanged = menuRefreshStatus(refreshStatus);
    if (!full && !scrolled && !selectionChanged && !statusChanged)
        return;
    menuReadVersion();
    AccentColor_EnsureLoaded();
    Draw_SetMenuBackground(Theme_GetBackground());
    if (full)
    {
        Draw_ClearFramebuffer();
        const char *title = Lang_Translate(menu->title);
        u32 tagX = 10 + Draw_MeasureString(title) + SPACING_X;
        if (tagX < 10 + 15 * SPACING_X)
            tagX = 10 + 15 * SPACING_X;
        Draw_DrawString(10, 10, g_accentColor, title);
        Draw_DrawString(tagX, 10, g_accentColor, "[" FORK_DISPLAY_VERSION "]");
        if (g_safeMode)
            Draw_DrawString(tagX + sizeof("[" FORK_DISPLAY_VERSION "]") * SPACING_X, 10, COLOR_RED, "SAFE");
        Draw_DrawString(10, 207, g_textColor, Lang_Translate("Up/Down: move  A: select  B: back  L/R: page"));
    }
    if (full || scrolled)
    {
        for (u32 row = menuScrollOffset; row < count && row < menuScrollOffset + MENU_MAX_ROWS; ++row)
        {
            u32 y = 30 + (row - menuScrollOffset) * SPACING_Y;
            char mark = 0;
            if (count > MENU_MAX_ROWS)
            {
                mark = ' ';
                if (row == menuScrollOffset && menuScrollOffset > 0) mark = '^';
                if (row == menuScrollOffset + MENU_MAX_ROWS - 1 && row + 1 < count) mark = 'v';
            }
            Draw_DrawMenuRow(y, g_textColor, Lang_Translate(menu->items[items[row]].title), g_cursorColor, items[row] == selected, mark);
        }
    }
    else if (selectionChanged)
    {
        Draw_DrawCharacter(10, 30 + (previousRow - menuScrollOffset) * SPACING_Y, g_cursorColor, ' ');
        Draw_DrawCharacter(10, 30 + (selectedRow - menuScrollOffset) * SPACING_Y, g_cursorColor, '>');
    }
    if (full || statusChanged)
    {
        Draw_FillRect(220, 10, 100, 10, g_menuBgColor);
        if (miniSocEnabled)
        {
            char ipBuffer[17];
            u32 ip = miniSocGetCachedHostId();
            u8 *addr = (u8 *)&ip;
            int n = sprintf(ipBuffer, "%hhu.%hhu.%hhu.%hhu", addr[0], addr[1], addr[2], addr[3]);
            Draw_DrawString(SCREEN_BOT_WIDTH - 10 - SPACING_X * n, 10, COLOR_WHITE, ipBuffer);
        }
        Draw_FillRect(0, SCREEN_BOT_HEIGHT - 20, SCREEN_BOT_WIDTH, SPACING_Y, g_menuBgColor);
        if (menuMcuResult == 0)
        {
            char text[32];
            u32 voltageInt = (u32)batteryVoltage;
            u32 voltageFrac = (u32)(batteryVoltage * 100.0f) % 100u;
            u32 percentageInt = (u32)batteryPercentage;
            u32 percentageFrac = (u32)(batteryPercentage * 10.0f) % 10u;
            int n = sprintf(text, "   %02hhu\xF8""C  %lu.%02luV  %lu.%lu%%", batteryTemperature,
                            (unsigned long)voltageInt, (unsigned long)voltageFrac,
                            (unsigned long)percentageInt, (unsigned long)percentageFrac);
            Draw_DrawString(SCREEN_BOT_WIDTH - 10 - SPACING_X * n, SCREEN_BOT_HEIGHT - 20, COLOR_WHITE, text);
        }
        if (menuIsRelease)
            Draw_DrawFormattedString(10, SCREEN_BOT_HEIGHT - 20, g_accentColor, "Luma3DS %s", menuVersionString);
        else
            Draw_DrawFormattedString(10, SCREEN_BOT_HEIGHT - 20, g_accentColor, "Luma3DS %s-%08lx", menuVersionString, (unsigned long)menuCommitHash);
    }
    state->menu = menu;
    state->selected = selected;
    state->scroll = menuScrollOffset;
    state->visibleMask = visibleMask;
    state->valid = true;
    Draw_FlushFramebuffer();
    Draw_SetMenuBackground(NULL);
}

void menuShow(Menu *root)
{
    MenuControls_Load();
    Theme_EnsureLoaded();
    u32 selectedItem = menuMoveSelection(root, 0, 0, false);
    Menu *currentMenu = root;
    u32 historyCount = 0;
    MenuHistory history[128];
    MenuDrawState state = {0};
    MenuInput input;
    MenuInput_Reset(&input, scanHeldKeys());
    menuScrollOffset = 0;
    menuCloseRequested = false;
    MenuControls_Apply();
    SDSpaceWarnIfLow();
    MenuInput_Reset(&input, scanHeldKeys());
    Draw_Lock();
    menuDraw(currentMenu, selectedItem, false, &state);
    Draw_Unlock();
    while (!menuShouldExit && !menuCloseRequested)
    {
        u32 pressed = menuWaitNavigation(&input);
        if (pressed & KEY_A)
        {
            MenuItem *item = &currentMenu->items[selectedItem];
            if (item->action_type == METHOD && item->method != NULL)
            {
                Draw_Lock();
                Draw_ClearFramebuffer();
                Draw_Unlock();
                item->method();
                selectedItem = menuMoveSelection(currentMenu, selectedItem, 0, false);
                MenuInput_Reset(&input, scanHeldKeys());
                state.valid = false;
            }
            else if (item->action_type == MENU && historyCount < sizeof(history) / sizeof(history[0]))
            {
                history[historyCount++] = (MenuHistory){ currentMenu, (u8)selectedItem, (u8)menuScrollOffset };
                currentMenu = item->menu;
                selectedItem = menuMoveSelection(currentMenu, 0, 0, false);
                menuScrollOffset = 0;
                state.valid = false;
            }
        }
        else if (pressed & KEY_B)
        {
            if (historyCount == 0)
            {
                while ((scanHeldKeys() & KEY_B) && !menuShouldExit && !menuCloseRequested)
                    svcSleepThread(4 * 1000 * 1000LL);
                break;
            }
            MenuHistory *previous = &history[--historyCount];
            currentMenu = previous->menu;
            selectedItem = previous->selected;
            menuScrollOffset = previous->scroll;
            state.valid = false;
        }
        else if (pressed & KEY_DOWN)
            selectedItem = menuMoveSelection(currentMenu, selectedItem, 1, true);
        else if (pressed & KEY_UP)
            selectedItem = menuMoveSelection(currentMenu, selectedItem, -1, true);
        else if (pressed & (KEY_LEFT | KEY_L))
            selectedItem = menuMoveSelection(currentMenu, selectedItem, -MENU_MAX_ROWS, false);
        else if (pressed & (KEY_RIGHT | KEY_R))
            selectedItem = menuMoveSelection(currentMenu, selectedItem, MENU_MAX_ROWS, false);
        if (menuShouldExit || menuCloseRequested)
            break;
        Draw_Lock();
        menuDraw(currentMenu, selectedItem, pressed == 0, &state);
        Draw_Unlock();
    }
}
