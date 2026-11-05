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
#include "luma_config.h"
#include "menus/sysconfig.h"
#include "memory.h"
#include "draw.h"
#include "menus/lang.h"
#include "fmt.h"
#include "utils.h"
#include "ifile.h"
#include "luminance.h"

static Menu audioRoutingMenu = {
    "Audio routing menu",
    {
        { "Auto (default)", METHOD, .method = &SysConfigMenu_SetAudioRoutingAuto },
        { "Force headphones (reapply on HP ins./rem.)", METHOD, .method = &SysConfigMenu_SetAudioRoutingHeadphones },
        { "Force speakers (reapply on HP ins./rem.)", METHOD, .method = &SysConfigMenu_SetAudioRoutingSpeakers },
        {},
    }
};

Menu sysconfigMenu = {
    "System configuration menu",
    {
        { "Set volume slider override...", METHOD, .method=&SysConfigMenu_AdjustVolume},
        { "Control Wireless connection", METHOD, .method = &SysConfigMenu_ControlWifi },
        { "Set LED master brightness override...", METHOD, .method = &SysConfigMenu_SetLedMasterBrightnessOverride },
        { "Set audio routing...", MENU, .menu = &audioRoutingMenu },
        { "Toggle Wireless", METHOD, .method = &SysConfigMenu_ToggleWireless },
        { "Toggle Power Button", METHOD, .method=&SysConfigMenu_TogglePowerButton },
        { "Toggle power to card slot", METHOD, .method=&SysConfigMenu_ToggleCardIfPower},
        { "Change screen brightness", METHOD, .method = &SysConfigMenu_ChangeScreenBrightness },
        {},
    }
};

bool isConnectionForced = false;

static Result SysConfigMenu_ApplyLedMasterBrightnessOverride(void)
{
    u8 override = lumaMcuConfig.ledMasterBrightnessOverride;

    u8 brightness = PctOverride_IsActive(override) ? (255 * PctOverride_GetPercentage(override) + 50) / 100 : 255;

    Result res = mcuHwcInit();
    if (R_SUCCEEDED(res))
    {
        res = MCUHWC_WriteRegister(0x28, &brightness, 1);
        mcuHwcExit();
    }

    return res;
}

static void SysConfigMenu_SetLedMasterBrightnessOverrideValue(u8 override)
{
    if (override == lumaMcuConfig.ledMasterBrightnessOverride)
        return;

    lumaMcuConfig.ledMasterBrightnessOverride = override;

    svcKernelSetState(0x10008, override);
    SysConfigMenu_ApplyLedMasterBrightnessOverride();
    LumaConfig_SaveMcuConfig();
}

#define SYSCONFIG_COL(x) (10 + (x) * SPACING_X)
#define SYSCONFIG_ROW(y) (10 + (y) * SPACING_Y)

static void SysConfigMenu_DrawOverrideStatus(u8 override)
{
    bool enabled = PctOverride_IsEnabled(override);
    Draw_DrawString(SYSCONFIG_COL(0), SYSCONFIG_ROW(2), COLOR_WHITE, Lang_Translate("Override status:    "));
    Draw_DrawString(SYSCONFIG_COL(20), SYSCONFIG_ROW(2), enabled ? COLOR_GREEN : COLOR_RED, enabled ? Lang_Translate("ENABLED ") : Lang_Translate("DISABLED"));
    Draw_DrawFormattedString(SYSCONFIG_COL(28), SYSCONFIG_ROW(2), COLOR_WHITE, "  (%s)    ", PctOverride_HasValue(override) ? Lang_Translate("set") : Lang_Translate("not set"));
}

void SysConfigMenu_SetLedMasterBrightnessOverride(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    bool mcuHwcOpened = R_SUCCEEDED(mcuHwcInit());

    do
    {
        u8 override = lumaMcuConfig.ledMasterBrightnessOverride;

        u8 curPct = PctOverride_HasValue(override) ? PctOverride_GetPercentage(override) : 100;

        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        SysConfigMenu_DrawOverrideStatus(override);
        Draw_DrawFormattedString(SYSCONFIG_COL(0), SYSCONFIG_ROW(3), COLOR_WHITE, Lang_Translate("Override value:     %u%%  "), curPct);
        Draw_DrawString(SYSCONFIG_COL(0), SYSCONFIG_ROW(5), COLOR_WHITE,
            Lang_Translate("Press A to toggle override.\n"
            "Press Right/Left to +/- the value by 10 points.\n"
            "Press Up/Down to +/- the value by 1 point.\n"
            "Press X to clear or refresh the override.\n"
            "Press B to go back.\n")
        );
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(15);

        if (pressed & KEY_B)
            break;

        override = lumaMcuConfig.ledMasterBrightnessOverride;
        curPct = PctOverride_HasValue(override) ? PctOverride_GetPercentage(override) : 100;
        s32 delta = 0;

        if (pressed & KEY_A)
        {
            override = PctOverride_SetEnabled(override, !PctOverride_IsEnabled(override));

            if (PctOverride_IsEnabled(override))
                override = PctOverride_SetPercentage(override, curPct);

            SysConfigMenu_SetLedMasterBrightnessOverrideValue(override);
        }
        else if (pressed & KEY_LEFT)
            delta = -10;
        else if (pressed & KEY_RIGHT)
            delta = +10;
        else if (pressed & KEY_DOWN)
            delta = -1;
        else if (pressed & KEY_UP)
            delta = +1;
        else if (pressed & KEY_X)
        {

            override = PctOverride_IsEnabled(override) ? PctOverride_SetPercentage(override, 100) : PCT_OVERRIDE_UNSET;
            SysConfigMenu_SetLedMasterBrightnessOverrideValue(override);
        }

        if (delta != 0)
            SysConfigMenu_SetLedMasterBrightnessOverrideValue(PctOverride_SetPercentage(override, (u8)CLAMP((s32)curPct + delta, 0, 100)));
    }
    while(!menuShouldExit);

    if (mcuHwcOpened)
        mcuHwcExit();
}

static Result SysConfigMenu_OpenCdcChkSession(void)
{

    Handle *cdcChkHandlePtr = cdcChkGetSessionHandle();
    *cdcChkHandlePtr = 0;

    Result res = srvGetServiceHandle(cdcChkHandlePtr, "cdc:CHK");

    if (R_FAILED(res))
        res = svcControlService(SERVICEOP_STEAL_CLIENT_SESSION, cdcChkHandlePtr, "cdc:CHK");

    return res;
}

static void SysConfigMenu_CloseCdcChkSession(void)
{
    Handle *cdcChkHandlePtr = cdcChkGetSessionHandle();
    svcCloseHandle(*cdcChkHandlePtr);
    *cdcChkHandlePtr = 0;
}

Result SysConfigMenu_ApplyAudioRouting(void)
{

    bool headphones;
    switch (LumaConfig_GetAudioRouting())
    {
        case AUDIO_ROUTING_SPEAKERS:
            headphones = false;
            break;
        case AUDIO_ROUTING_HEADPHONES:
            headphones = true;
            break;
        case AUDIO_ROUTING_AUTO:
        default:

            headphones = (*(vu8 *)((0x10147000 | (1u << 31)) + 0x10) & 1) != 0;
            break;
    }

    Result res = SysConfigMenu_OpenCdcChkSession();
    if (R_FAILED(res))
        return res;

    u8 reg;
    res = CDCCHK_ReadRegisters2(100, 69, &reg, 1);
    reg = (reg & ~0x30) | 0x20 | (headphones ? 0x10 : 0x00);
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(100, 69, &reg, 1);

    SysConfigMenu_CloseCdcChkSession();
    return res;
}

static void SysConfigMenu_SetAudioRouting(AudioRouting routing)
{
    LumaConfig_SetMcuConfigFlags(MCUCFG_FLAGS_AUDIO_ROUTING_MASK, (u8)routing << MCUCFG_FLAGS_AUDIO_ROUTING_SHIFT);
    SysConfigMenu_ApplyAudioRouting();
}

void SysConfigMenu_SetAudioRoutingAuto(void)
{
    SysConfigMenu_SetAudioRouting(AUDIO_ROUTING_AUTO);
}

void SysConfigMenu_SetAudioRoutingHeadphones(void)
{
    SysConfigMenu_SetAudioRouting(AUDIO_ROUTING_HEADPHONES);
}

void SysConfigMenu_SetAudioRoutingSpeakers(void)
{
    SysConfigMenu_SetAudioRouting(AUDIO_ROUTING_SPEAKERS);
}

void SysConfigMenu_ToggleWireless(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    bool nwmRunning = isServiceUsable("nwm::EXT");

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        Draw_DrawString(10, 30, COLOR_WHITE, Lang_Translate("Press A to toggle, press B to go back."));

        u8 wireless = (*(vu8 *)((0x10140000 | (1u << 31)) + 0x180));

        if(nwmRunning)
        {
            Draw_DrawString(10, 50, COLOR_WHITE, Lang_Translate("Current status:"));
            Draw_DrawString(100, 50, (wireless ? COLOR_GREEN : COLOR_RED), (wireless ? Lang_Translate(" ON ") : Lang_Translate(" OFF")));
        }
        else
        {
            Draw_DrawString(10, 50, COLOR_RED, Lang_Translate("NWM isn't running."));
            Draw_DrawString(10, 60, COLOR_RED, Lang_Translate("If you're currently on Test Menu,"));
            Draw_DrawString(10, 70, COLOR_RED, Lang_Translate("exit then press R+RIGHT to toggle the WiFi."));
            Draw_DrawString(10, 80, COLOR_RED, Lang_Translate("Otherwise, simply exit and wait a few seconds."));
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A && nwmRunning)
        {
            nwmExtInit();
            NWMEXT_ControlWirelessEnabled(!wireless);
            nwmExtExit();
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_UpdateStatus(bool control)
{
    MenuItem *item = &sysconfigMenu.items[1];

    if(control)
    {
        item->title = "Control Wireless connection";
        item->method = &SysConfigMenu_ControlWifi;
    }
    else
    {
        item->title = "Disable forced wireless connection";
        item->method = &SysConfigMenu_DisableForcedWifiConnection;
    }
}

static bool SysConfigMenu_ForceWifiConnection(u32 slot)
{
    char ssid[0x20 + 1] = {0};
    isConnectionForced = false;

    if(R_FAILED(acInit()))
        return false;

    acuConfig config = {0};
    ACU_CreateDefaultConfig(&config);
    ACU_SetNetworkArea(&config, 2);
    ACU_SetAllowApType(&config, 1 << slot);
    ACU_SetRequestEulaVersion(&config);

    Handle connectEvent = 0;
    svcCreateEvent(&connectEvent, RESET_ONESHOT);

    bool forcedConnection = false;
    if(R_SUCCEEDED(ACU_ConnectAsync(&config, connectEvent)))
    {
        if(R_SUCCEEDED(svcWaitSynchronization(connectEvent, -1)) && R_SUCCEEDED(ACU_GetSSID(ssid)))
            forcedConnection = true;
    }
    svcCloseHandle(connectEvent);

    if(forcedConnection)
    {
        isConnectionForced = true;
        SysConfigMenu_UpdateStatus(false);
    }
    else
        acExit();

    char infoString[80] = {0};
    u32 infoStringColor = forcedConnection ? COLOR_GREEN : COLOR_RED;
    if(forcedConnection)
        sprintf(infoString, "Succesfully forced a connection to: %s", ssid);
    else
       sprintf(infoString, "Failed to connect to slot %d", (int)slot + 1);

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        Draw_DrawString(10, 30, infoStringColor, infoString);
        Draw_DrawString(10, 40, COLOR_WHITE, Lang_Translate("Press B to go back."));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_B)
            break;
    }
    while(!menuShouldExit);

    return forcedConnection;
}

void SysConfigMenu_TogglePowerButton(void)
{
    u32 mcuIRQMask;

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    mcuHwcInit();
    MCUHWC_ReadRegister(0x18, (u8*)&mcuIRQMask, 4);
    mcuHwcExit();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        Draw_DrawString(10, 30, COLOR_WHITE, Lang_Translate("Press A to toggle, press B to go back."));

        Draw_DrawString(10, 50, COLOR_WHITE, Lang_Translate("Current status:"));
        Draw_DrawString(100, 50, (((mcuIRQMask & 0x00000001) == 0x00000001) ? COLOR_RED : COLOR_GREEN), (((mcuIRQMask & 0x00000001) == 0x00000001) ? Lang_Translate(" DISABLED") : Lang_Translate(" ENABLED ")));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            mcuHwcInit();
            MCUHWC_ReadRegister(0x18, (u8*)&mcuIRQMask, 4);
            mcuIRQMask ^= 0x00000001;
            MCUHWC_WriteRegister(0x18, (u8*)&mcuIRQMask, 4);
            mcuHwcExit();
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_ControlWifi(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    u32 slot = 0;
    char ssids[3][32] = {{0}};

    Result resInit = acInit();
    for (u32 i = 0; i < 3; i++)
    {
        if (R_SUCCEEDED(ACI_LoadNetworkSetting(i)))
            ACI_GetNetworkWirelessEssidSecuritySsid(ssids[i]);
        else
            strcpy(ssids[i], "(not configured)");
    }
    if (R_SUCCEEDED(resInit))
        acExit();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        u32 posY = Draw_DrawString(10, 30, COLOR_WHITE, Lang_Translate("Press A to force a connection to slot, B to go back\n\n"));

        for (u32 i = 0; i < 3; i++)
        {
            Draw_DrawString(10, posY + SPACING_Y * i, g_cursorColor, slot == i ? ">" : " ");
            Draw_DrawFormattedString(30, posY + SPACING_Y * i, COLOR_WHITE, "[%d] %s", (int)i + 1, ssids[i]);
        }

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            if(SysConfigMenu_ForceWifiConnection(slot))
            {

                break;
            }

            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_FlushFramebuffer();
            Draw_Unlock();
        }
        else if(pressed & KEY_DOWN)
        {
            slot = (slot + 1) % 3;
        }
        else if(pressed & KEY_UP)
        {
            slot = (3 + slot - 1) % 3;
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_DisableForcedWifiConnection(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    acExit();
    SysConfigMenu_UpdateStatus(true);

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        Draw_DrawString(10, 30, COLOR_WHITE, Lang_Translate("Forced connection successfully disabled.\nNote: auto-connection may remain broken."));

        u32 pressed = waitInputWithTimeout(1000);
        if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

void SysConfigMenu_ToggleCardIfPower(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    bool cardIfStatus = false;
    bool updatedCardIfStatus = false;

    do
    {
        Result res = FSUSER_CardSlotGetCardIFPowerStatus(&cardIfStatus);
        if (R_FAILED(res)) cardIfStatus = false;

        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        u32 posY = Draw_DrawString(10, 30, COLOR_WHITE, Lang_Translate("Press A to toggle, press B to go back.\n\n"));
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Inserting or removing a card will reset the status,\nand you'll need to reinsert the cart if you want to\nplay it.\n\n"));
        Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Current status:"));
        Draw_DrawString(100, posY, !cardIfStatus ? COLOR_RED : COLOR_GREEN, !cardIfStatus ? Lang_Translate(" DISABLED") : Lang_Translate(" ENABLED "));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if(pressed & KEY_A)
        {
            if (!cardIfStatus)
                res = FSUSER_CardSlotPowerOn(&updatedCardIfStatus);
            else
                res = FSUSER_CardSlotPowerOff(&updatedCardIfStatus);

            if (R_SUCCEEDED(res))
                cardIfStatus = !updatedCardIfStatus;
        }
        else if(pressed & KEY_B)
            return;
    }
    while(!menuShouldExit);
}

static const u8 stereoBalanceAttenuationLut[100] = {
     0,  0,  0,  1,  1,  1,  1,  1,  1,  2,
     2,  2,  2,  2,  3,  3,  3,  3,  3,  4,
     4,  4,  4,  5,  5,  5,  5,  5,  6,  6,
     6,  6,  7,  7,  7,  7,  8,  8,  8,  9,
     9,  9,  9, 10, 10, 10, 11, 11, 11, 12,
    12, 12, 13, 13, 13, 14, 14, 15, 15, 15,
    16, 16, 17, 17, 18, 18, 19, 19, 20, 20,
    21, 22, 22, 23, 23, 24, 25, 26, 26, 27,
    28, 29, 30, 31, 32, 33, 34, 35, 37, 38,
    40, 42, 44, 46, 49, 52, 56, 61, 68, 80,
};

static s8 SysConfigMenu_ConvertSliderPercentageToVolume(u8 pct)
{

    u8 rawPinGain = 127 - (71 * pct + 50) / 100;

    if (rawPinGain <= 90)
        return 36 - rawPinGain;
    else if (rawPinGain >= 91 && rawPinGain <= 126)
        return 126 - 2 * rawPinGain;
    else
        return -128;
}

static s8 SysConfigMenu_AttenuateVolume(s8 volume, s8 balancePct)
{

    if (volume == -128 || balancePct >= 100)
        return -128;

    s32 v = (s32)volume - (s32)stereoBalanceAttenuationLut[balancePct];
    return (s8)CLAMP(v, -127, 48);
}

static Result SysConfigMenu_ApplyVolumeOverride(void)
{

    u8 override = lumaMcuConfig.volumeSliderOverride;
    s8 balancePct = lumaMcuConfig.volumeSliderOverrideStereoBalance;
    bool active = PctOverride_IsActive(override);

    u8 i2s1Mux;
    u8 i2s2Mux;
    Result res = SysConfigMenu_OpenCdcChkSession();
    if (R_FAILED(res))
        return res;

    res = CDCCHK_ReadRegisters2(0,  116, &i2s1Mux, 1);
    if (R_SUCCEEDED(res)) res = CDCCHK_ReadRegisters2(100, 49, &i2s2Mux, 1);

    if (active)
    {
        i2s1Mux &= ~0x80;
        i2s2Mux |=  0x20;
    }
    else
    {
        i2s1Mux |=  0x80;
        i2s2Mux &= ~0x20;
    }

    s8 i2s1VolumeL;
    s8 i2s1VolumeR;
    s8 i2s2Volume;
    if (active)
    {
        s8 volume = SysConfigMenu_ConvertSliderPercentageToVolume(PctOverride_GetPercentage(override));

        i2s1VolumeL = balancePct > 0 ? SysConfigMenu_AttenuateVolume(volume, balancePct) : volume;
        i2s1VolumeR = balancePct < 0 ? SysConfigMenu_AttenuateVolume(volume, -balancePct) : volume;
        i2s2Volume = volume;
    }
    else
    {

        i2s1VolumeL =  -3;
        i2s1VolumeR =  -3;
        i2s2Volume  = -20;
    }

    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(0, 65, &i2s1VolumeL, 1);
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(0, 66, &i2s1VolumeR, 1);
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(100, 123, &i2s2Volume, 1);

    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(0, 116, &i2s1Mux, 1);
    if (R_SUCCEEDED(res)) res = CDCCHK_WriteRegisters2(100, 49, &i2s2Mux, 1);

    SysConfigMenu_CloseCdcChkSession();
    return res;
}

void SysConfigMenu_RestoreSettings(void)
{

    if (isServiceUsable("cdc:CHK"))
    {
        if (PctOverride_IsActive(lumaMcuConfig.volumeSliderOverride))
            SysConfigMenu_ApplyVolumeOverride();

        if (LumaConfig_GetAudioRouting() != AUDIO_ROUTING_AUTO)
        {

            svcSleepThread(20 * 1000 * 1000LL);
            SysConfigMenu_ApplyAudioRouting();
        }
    }

    if (PctOverride_IsActive(lumaMcuConfig.ledMasterBrightnessOverride) && isServiceUsable("mcu::HWC"))
        SysConfigMenu_ApplyLedMasterBrightnessOverride();
}

static u8 SysConfigMenu_SampleVolumeSliderPercentage(void)
{

    u8 sliderLevel = 63;
    MCUHWC_ReadRegister(0x09, &sliderLevel, 1);
    return (100 * sliderLevel + 31) / 63;
}

void SysConfigMenu_AdjustVolume(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    u32 pos = 0;
    Result res = 0;

    bool mcuHwcOpened = R_SUCCEEDED(mcuHwcInit());

    do
    {
        u8 override = lumaMcuConfig.volumeSliderOverride;
        s8 balancePct = lumaMcuConfig.volumeSliderOverrideStereoBalance;
        u8 curPct = PctOverride_HasValue(override) ? PctOverride_GetPercentage(override) : SysConfigMenu_SampleVolumeSliderPercentage();

        char volumeStr[16];
        char balanceStr[16];
        sprintf(volumeStr, "%u%%", curPct);
        if (balancePct < 0)
            sprintf(balanceStr, "L %d%%", -balancePct);
        else if (balancePct > 0)
            sprintf(balanceStr, "R %d%%", balancePct);
        else
            strcpy(balanceStr, Lang_Translate("centered"));

        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("System configuration menu"));
        SysConfigMenu_DrawOverrideStatus(override);
        Draw_DrawFormattedString(SYSCONFIG_COL(0), SYSCONFIG_ROW(3), COLOR_WHITE, Lang_Translate("   Override value:  %-10s    "), volumeStr);
        Draw_DrawFormattedString(SYSCONFIG_COL(0), SYSCONFIG_ROW(4), COLOR_WHITE, Lang_Translate("   Stereo balance:  %-10s    "), balanceStr);
        for (u32 i = 0; i < 2; i++)
            Draw_DrawString(SYSCONFIG_COL(0), SYSCONFIG_ROW(3 + i), COLOR_TITLE, i == pos ? ">" : " ");
        Draw_DrawString(SYSCONFIG_COL(0), SYSCONFIG_ROW(6), COLOR_WHITE,
            Lang_Translate("Press Up/Down to select value.\n"
            "Press Right/Left to +/- by 1 point.\n"
            "Hold R to adjust by 10 points.\n\n"
            "Press A to toggle override.\n"
            "Press X to clear or refresh volume.\n"
            "Press Y to center stereo balance.\n"
            "Press B to go back.\n")
        );
        if (R_FAILED(res))
            Draw_DrawFormattedString(SYSCONFIG_COL(0), SYSCONFIG_ROW(15), COLOR_RED, Lang_Translate("Failed: 0x%08lX"), res);
        else
            Draw_DrawString(SYSCONFIG_COL(0), SYSCONFIG_ROW(15), COLOR_WHITE, "                  ");
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 kHeld = 0;
        u32 pressed = waitInputWithTimeoutEx(&kHeld, 15);
        bool apply = false;

        if (pressed & KEY_B)
            break;

        override = lumaMcuConfig.volumeSliderOverride;
        balancePct = lumaMcuConfig.volumeSliderOverrideStereoBalance;
        if (pressed != 0)
            curPct = PctOverride_HasValue(override) ? PctOverride_GetPercentage(override) : SysConfigMenu_SampleVolumeSliderPercentage();

        if (pressed & KEY_A)
        {
            override = PctOverride_SetEnabled(override, !PctOverride_IsEnabled(override));

            if (PctOverride_IsEnabled(override))
                override = PctOverride_SetPercentage(override, curPct);

            apply = true;
        }
        else if (pressed & KEY_UP)
            pos = (2 + pos - 1) % 2;
        else if (pressed & KEY_DOWN)
            pos = (pos + 1) % 2;
        else if (pressed & KEY_Y)
        {
            balancePct = 0;
            apply = true;
        }
        else if (pressed & KEY_X)
        {

            override = PctOverride_IsEnabled(override) ? PctOverride_SetPercentage(override, SysConfigMenu_SampleVolumeSliderPercentage()) : PCT_OVERRIDE_UNSET;
            apply = true;
        }
        else
        {
            s32 delta = 0;
            s32 mult = (kHeld & KEY_R) ? 10 : 1;

            if (pressed & KEY_LEFT)
                delta = -mult;
            else if (pressed & KEY_RIGHT)
                delta = +mult;

            if (delta != 0)
            {
                if (pos == 0)
                    override = PctOverride_SetPercentage(override, (u8)CLAMP((s32)curPct + delta, 0, 100));
                else
                    balancePct = (s8)CLAMP((s32)balancePct + delta, -100, 100);

                apply = true;
            }
        }

        if (apply)
        {
            bool changed = override != lumaMcuConfig.volumeSliderOverride || balancePct != lumaMcuConfig.volumeSliderOverrideStereoBalance;
            lumaMcuConfig.volumeSliderOverride = override;
            lumaMcuConfig.volumeSliderOverrideStereoBalance = balancePct;
            res = SysConfigMenu_ApplyVolumeOverride();
            if (changed)
                LumaConfig_SaveMcuConfig();
        }
    } while(!menuShouldExit);

    if (mcuHwcOpened)
        mcuHwcExit();
}

void SysConfigMenu_ChangeScreenBrightness(void)
{
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    u32 luminance = getCurrentLuminance(false);
    u32 minLum = getMinLuminancePreset();
    u32 maxLum = getMaxLuminancePreset();

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Screen brightness"));
        u32 posY = 30;
        posY = Draw_DrawFormattedString(
            10,
            posY,
            COLOR_WHITE, Lang_Translate("Current luminance: %lu (min. %lu, max. %lu)\n\n"),
            luminance,
            minLum,
            maxLum
        );
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Controls: Up/Down for +-1, Right/Left for +-10.\n"));
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Press A to start, B to exit.\n\n"));

        posY = Draw_DrawString(10, posY, COLOR_RED, Lang_Translate("WARNING: \n"));
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("  * value will be limited by calibration.\n"));
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("  * bottom framebuffer will be restored until\nyou exit."));
        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if (pressed & KEY_A)
            break;

        if (pressed & KEY_B)
            return;
    }
    while (!menuShouldExit);

    Draw_Lock();

    Draw_RestoreFramebuffer();
    Draw_FreeFramebufferCache();

    svcKernelSetState(0x10000, 2);
    gspLcdInit();

    s32 lum = (s32)luminance;

    do
    {
        u32 pressed = waitInputWithTimeout(1000);
        if (pressed & DIRECTIONAL_KEYS)
        {
            if (pressed & KEY_UP)
                lum += 1;
            else if (pressed & KEY_DOWN)
                lum -= 1;
            else if (pressed & KEY_RIGHT)
                lum += 10;
            else if (pressed & KEY_LEFT)
                lum -= 10;

            lum = lum < (s32)minLum ? (s32)minLum : lum;
            lum = lum > (s32)maxLum ? (s32)maxLum : lum;

            GSPLCD_SetBrightnessRaw(BIT(GSP_SCREEN_TOP) | BIT(GSP_SCREEN_BOTTOM), lum);
        }

        if (pressed & KEY_B)
            break;
    }
    while (!menuShouldExit);

    gspLcdExit();
    svcKernelSetState(0x10000, 2);

    if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
    {

        __builtin_trap();
    }
    else
        Draw_SetupFramebuffer();

    Draw_Unlock();
}
