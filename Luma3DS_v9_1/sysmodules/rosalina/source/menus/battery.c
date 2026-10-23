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
#include "menus/battery.h"
#include "menus/lang.h"
#include "draw.h"
#include "fmt.h"
#include "ifile.h"
#include "csvc.h"

#define BATTERY_HISTORY_SIZE        120
#define BATTERY_SAMPLE_INTERVAL_S   30
#define BATTERY_NOMINAL_MAH         1300
#define BATTERY_GRAPH_COLS          46
#define BATTERY_GRAPH_ROWS          8
#define BATTERY_CSV_PATH            "/luma/battery.csv"

#define BATTERY_FLAG_CHARGING       1
#define BATTERY_FLAG_ADAPTER        2

#define BATTERY_PCT_MAX             25600

typedef struct BatterySample {
    u32 timeSec;
    u16 pctQ8;
    u16 milliVolts;
    u8  temperature;
    u8  flags;
} BatterySample;

static BatterySample g_samples[BATTERY_HISTORY_SIZE];
static u32 g_sampleCount;
static u32 g_sampleHead;

static bool g_attempted;
static bool g_hasReading;
static u32  g_lastAttemptSec;

static u16  g_pctQ8;
static u16  g_milliVolts;
static u8   g_temperature;
static u8   g_ptmuLevel;
static u8   g_shellState;
static bool g_charging;
static bool g_adapter;

static bool g_statsValid;
static u32  g_sessionStartSec;
static u16  g_sessionStartPctQ8;
static u16  g_minPctQ8;
static u16  g_maxPctQ8;
static u16  g_minMilliVolts;
static u16  g_maxMilliVolts;
static u8   g_minTemperature;
static u8   g_maxTemperature;
static u32  g_cycleStartSec;
static u16  g_cycleStartPctQ8;
static u32  g_chargeTransitions;

static u32  g_mcuFwVersion;
static u8   g_mcuInfoTable[10];
static bool g_mcuInfoTableRead;

static bool batteryServiceUsable(const char *name)
{
    bool registered = false;
    return R_SUCCEEDED(srvIsServiceRegistered(&registered, name)) && registered;
}

static u32 batteryNowSec(void)
{
    return (u32)(svcGetSystemTick() / SYSCLOCK_ARM11);
}

static const BatterySample *batteryGetSample(u32 index)
{
    u32 base = (g_sampleHead + BATTERY_HISTORY_SIZE - g_sampleCount) % BATTERY_HISTORY_SIZE;
    return &g_samples[(base + index) % BATTERY_HISTORY_SIZE];
}

static void batteryPushSample(u32 nowSec)
{
    BatterySample *s = &g_samples[g_sampleHead];

    s->timeSec = nowSec;
    s->pctQ8 = g_pctQ8;
    s->milliVolts = g_milliVolts;
    s->temperature = g_temperature;
    s->flags = (u8)((g_charging ? BATTERY_FLAG_CHARGING : 0) | (g_adapter ? BATTERY_FLAG_ADAPTER : 0));

    g_sampleHead = (g_sampleHead + 1) % BATTERY_HISTORY_SIZE;
    if (g_sampleCount < BATTERY_HISTORY_SIZE)
        ++g_sampleCount;
}

static bool batteryReadMcu(void)
{
    u8 data[4];
    Result res;

    if (!batteryServiceUsable("mcu::HWC"))
        return false;

    Handle *handlePtr = mcuHwcGetSessionHandle();
    *handlePtr = 0;

    res = srvGetServiceHandle(handlePtr, "mcu::HWC");
    if (R_FAILED(res))
        res = svcControlService(SERVICEOP_STEAL_CLIENT_SESSION, handlePtr, "mcu::HWC");
    if (R_FAILED(res))
        return false;

    res = MCUHWC_ReadRegister(0xA, data, 4);

    if (g_mcuFwVersion == 0)
    {
        u8 minor = 0, major = 0;
        MCUHWC_GetFwVerHigh(&major);
        MCUHWC_GetFwVerLow(&minor);
        g_mcuFwVersion = SYSTEM_VERSION(major - 0x10, minor, 0);
    }

    if (!g_mcuInfoTableRead)
        g_mcuInfoTableRead = R_SUCCEEDED(MCUHWC_ReadRegister(0x7F, g_mcuInfoTable, sizeof(g_mcuInfoTable)));

    svcCloseHandle(*handlePtr);

    if (R_FAILED(res))
        return false;

    g_temperature = data[0];

    u32 q8 = (u32)data[1] * 256u + data[2];
    g_pctQ8 = (u16)(q8 > BATTERY_PCT_MAX ? BATTERY_PCT_MAX : q8);
    g_milliVolts = (u16)((u32)data[3] * 20u);

    return true;
}

static void batteryReadPtmu(void)
{
    u8 chargeState = 0;
    u8 level = 0;
    u8 shell = 0;
    bool adapter = false;

    if (!batteryServiceUsable("ptm:u"))
        return;

    if (R_FAILED(ptmuInit()))
        return;

    if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&chargeState)))
        g_charging = chargeState != 0;
    if (R_SUCCEEDED(PTMU_GetAdapterState(&adapter)))
        g_adapter = adapter;
    if (R_SUCCEEDED(PTMU_GetBatteryLevel(&level)))
        g_ptmuLevel = level;
    if (R_SUCCEEDED(PTMU_GetShellState(&shell)))
        g_shellState = shell;

    ptmuExit();
}

static bool batteryRefresh(void)
{
    bool wasCharging = g_charging;

    if (!batteryReadMcu())
        return false;

    batteryReadPtmu();

    u32 nowSec = batteryNowSec();

    if (!g_statsValid)
    {
        g_sessionStartSec = nowSec;
        g_sessionStartPctQ8 = g_pctQ8;
        g_minPctQ8 = g_pctQ8;
        g_maxPctQ8 = g_pctQ8;
        g_minMilliVolts = g_milliVolts;
        g_maxMilliVolts = g_milliVolts;
        g_minTemperature = g_temperature;
        g_maxTemperature = g_temperature;
        g_cycleStartSec = nowSec;
        g_cycleStartPctQ8 = g_pctQ8;
        g_statsValid = true;
    }
    else
    {
        if (g_pctQ8 < g_minPctQ8) g_minPctQ8 = g_pctQ8;
        if (g_pctQ8 > g_maxPctQ8) g_maxPctQ8 = g_pctQ8;
        if (g_milliVolts < g_minMilliVolts) g_minMilliVolts = g_milliVolts;
        if (g_milliVolts > g_maxMilliVolts) g_maxMilliVolts = g_milliVolts;
        if (g_temperature < g_minTemperature) g_minTemperature = g_temperature;
        if (g_temperature > g_maxTemperature) g_maxTemperature = g_temperature;

        if (g_charging != wasCharging)
        {
            g_cycleStartSec = nowSec;
            g_cycleStartPctQ8 = g_pctQ8;
            ++g_chargeTransitions;
        }
    }

    g_hasReading = true;
    return true;
}

static void batteryResetStats(void)
{
    g_sampleCount = 0;
    g_sampleHead = 0;
    g_statsValid = false;
    g_chargeTransitions = 0;
    g_attempted = false;
}

void Battery_Sample(void)
{
    u32 nowSec = batteryNowSec();

    if (g_attempted && nowSec - g_lastAttemptSec < BATTERY_SAMPLE_INTERVAL_S)
        return;

    g_attempted = true;
    g_lastAttemptSec = nowSec;

    if (!batteryRefresh())
        return;

    batteryPushSample(nowSec);
}

static bool batteryComputeRate(s32 *outQ8PerHour, u32 *outCount, u32 *outSpanSec)
{
    if (g_sampleCount < 3)
        return false;

    u32 last = g_sampleCount - 1;
    u8 mode = (u8)(batteryGetSample(last)->flags & BATTERY_FLAG_CHARGING);
    u32 first = last;

    while (first > 0 && (batteryGetSample(first - 1)->flags & BATTERY_FLAG_CHARGING) == mode)
        --first;

    u32 n = last - first + 1;
    if (n < 3)
        return false;

    u32 t0 = batteryGetSample(first)->timeSec;
    u32 span = batteryGetSample(last)->timeSec - t0;

    if (span < 60)
        return false;

    s64 sx = 0, sy = 0, sxx = 0, sxy = 0;

    for (u32 i = first; i <= last; ++i)
    {
        const BatterySample *s = batteryGetSample(i);
        s64 x = (s64)(s->timeSec - t0);
        s64 y = (s64)s->pctQ8;

        sx += x;
        sy += y;
        sxx += x * x;
        sxy += x * y;
    }

    s64 den = (s64)n * sxx - sx * sx;
    if (den == 0)
        return false;

    s64 num = (s64)n * sxy - sx * sy;

    *outQ8PerHour = (s32)((num * 3600) / den);
    *outCount = n;
    *outSpanSec = span;

    return true;
}

static void batteryFormatDuration(char *out, u32 seconds)
{
    u32 h = seconds / 3600u;
    u32 m = (seconds / 60u) % 60u;
    u32 s = seconds % 60u;

    if (h >= 100u)
        sprintf(out, "> 99h");
    else if (h > 0u)
        sprintf(out, "%luh %02lum", h, m);
    else
        sprintf(out, "%lum %02lus", m, s);
}

static void batteryFormatPercent(char *out, u32 pctQ8)
{
    sprintf(out, "%lu.%02lu", pctQ8 / 256u, ((pctQ8 % 256u) * 100u) / 256u);
}

static void batteryFormatSignedPercent(char *out, s32 pctQ8)
{
    u32 mag = (u32)(pctQ8 < 0 ? -pctQ8 : pctQ8);
    sprintf(out, "%s%lu.%02lu", pctQ8 < 0 ? "-" : "+", mag / 256u, ((mag % 256u) * 100u) / 256u);
}

static void batteryDeleteFile(const char *path)
{
    FS_Archive archive = 0;

    if (R_SUCCEEDED(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
    {
        FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, path));
        FSUSER_CloseArchive(archive);
    }
}

static Result batteryExportCsv(void)
{
    IFile file;
    Result res;
    char buf[160];
    u64 written = 0;
    u32 len;

    batteryDeleteFile(BATTERY_CSV_PATH);

    res = IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                     fsMakePath(PATH_ASCII, BATTERY_CSV_PATH), FS_OPEN_CREATE | FS_OPEN_WRITE);
    if (R_FAILED(res))
        return res;

    file.pos = 0;

    len = (u32)sprintf(buf, "elapsed_s,percent,millivolts,celsius,charging,adapter\r\n");
    res = IFile_Write(&file, &written, buf, len, FS_WRITE_FLUSH);

    for (u32 i = 0; i < g_sampleCount && R_SUCCEEDED(res); ++i)
    {
        const BatterySample *s = batteryGetSample(i);

        len = (u32)sprintf(buf, "%lu,%lu.%02lu,%lu,%lu,%lu,%lu\r\n",
                           (u32)(s->timeSec - g_sessionStartSec),
                           (u32)s->pctQ8 / 256u,
                           (((u32)s->pctQ8 % 256u) * 100u) / 256u,
                           (u32)s->milliVolts,
                           (u32)s->temperature,
                           (u32)((s->flags & BATTERY_FLAG_CHARGING) ? 1u : 0u),
                           (u32)((s->flags & BATTERY_FLAG_ADAPTER) ? 1u : 0u));

        res = IFile_Write(&file, &written, buf, len, FS_WRITE_FLUSH);
    }

    IFile_Close(&file);
    return res;
}

static u32 batteryDrawNow(u32 posY)
{
    char text[32];

    batteryFormatPercent(text, g_pctQ8);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_CHARGE), text);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_VOLTAGE_V),
                                    (u32)g_milliVolts / 1000u, (u32)g_milliVolts % 1000u);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Temperature:   %lu \xF8""C\n", (u32)g_temperature);
    posY = Draw_DrawFormattedString(10, posY, g_charging ? COLOR_LIME : COLOR_WHITE,
                                    "Charging:      %s\n", g_charging ? "yes" : "no");
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Adapter:       %s\n",
                                    g_adapter ? "plugged in" : "unplugged");
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_PTM_LEVEL_5), (u32)g_ptmuLevel);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Shell:         %s\n",
                                    g_shellState ? "open" : "closed");

    if (g_mcuFwVersion != 0)
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "MCU firmware:  %lu.%lu\n",
                                        GET_VERSION_MAJOR(g_mcuFwVersion), GET_VERSION_MINOR(g_mcuFwVersion));
    if (g_mcuInfoTableRead)
    {
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_BATTERY_MAKER), (u32)g_mcuInfoTable[2]);
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_PMIC_MAKER), (u32)g_mcuInfoTable[1]);
    }

    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_NOMINAL_PACK_MAH), (u32)BATTERY_NOMINAL_MAH);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_SAMPLES_HELD),
                                    g_sampleCount, (u32)BATTERY_HISTORY_SIZE);

    return posY;
}

static u32 batteryDrawEstimates(u32 posY)
{
    char text[32];
    char text2[32];
    s32 rateQ8PerHour = 0;
    u32 rateSamples = 0;
    u32 rateSpan = 0;

    if (!batteryComputeRate(&rateQ8PerHour, &rateSamples, &rateSpan))
    {
        posY = Draw_DrawFormattedString(10, posY, COLOR_YELLOW,
                                        Lang(STR_COLLECTING_DATA_SAMPLES_ONE_EVERY_S_3_NE),
                                        g_sampleCount, (u32)BATTERY_SAMPLE_INTERVAL_S);
    }
    else
    {
        u32 mag = (u32)(rateQ8PerHour < 0 ? -rateQ8PerHour : rateQ8PerHour);

        batteryFormatSignedPercent(text, rateQ8PerHour);
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_RATE_H), text);

        u32 milliAmps = (u32)(((u64)mag * BATTERY_NOMINAL_MAH) / BATTERY_PCT_MAX);
        u32 milliWatts = (u32)(((u64)milliAmps * g_milliVolts) / 1000u);

        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_ESTIMATED_MA_W),
                                        milliAmps, milliWatts / 1000u, (milliWatts % 1000u) / 10u);

        if (mag < 8u)
        {
            posY = Draw_DrawString(10, posY, COLOR_YELLOW, Lang(STR_REMAINING_RATE_TOO_FLAT));
        }
        else if (g_charging && rateQ8PerHour > 0)
        {
            batteryFormatDuration(text, (u32)(((u64)(BATTERY_PCT_MAX - g_pctQ8) * 3600u) / mag));
            posY = Draw_DrawFormattedString(10, posY, COLOR_LIME, Lang(STR_UNTIL_FULL), text);
        }
        else if (!g_charging && rateQ8PerHour < 0)
        {
            batteryFormatDuration(text, (u32)(((u64)g_pctQ8 * 3600u) / mag));
            posY = Draw_DrawFormattedString(10, posY, COLOR_LIME, Lang(STR_UNTIL_EMPTY), text);

            batteryFormatDuration(text2, g_pctQ8 > 2560 ? (u32)(((u64)(g_pctQ8 - 2560) * 3600u) / mag) : 0u);
            posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_UNTIL_10), text2);
        }
        else
        {
            posY = Draw_DrawString(10, posY, COLOR_YELLOW, Lang(STR_REMAINING_DIRECTION_UNCLEAR));
        }

        batteryFormatDuration(text, rateSpan);
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_FIT_WINDOW_PTS),
                                        text, rateSamples);
    }

    u32 nowSec = batteryNowSec();

    batteryFormatDuration(text, nowSec - g_sessionStartSec);
    batteryFormatSignedPercent(text2, (s32)g_pctQ8 - (s32)g_sessionStartPctQ8);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_SINCE_RESET), text, text2);

    batteryFormatDuration(text, nowSec - g_cycleStartSec);
    batteryFormatSignedPercent(text2, (s32)g_pctQ8 - (s32)g_cycleStartPctQ8);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_THIS_PHASE), text, text2);

    batteryFormatPercent(text, g_minPctQ8);
    batteryFormatPercent(text2, g_maxPctQ8);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_CHARGE_RANGE), text, text2);

    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_VOLTAGE_RANGE_V),
                                    (u32)g_minMilliVolts / 1000u, (u32)g_minMilliVolts % 1000u,
                                    (u32)g_maxMilliVolts / 1000u, (u32)g_maxMilliVolts % 1000u);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "Temp range:    %lu - %lu \xF8""C\n",
                                    (u32)g_minTemperature, (u32)g_maxTemperature);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_PLUG_EVENTS), g_chargeTransitions);

    return posY;
}

static u32 batteryDrawGraph(u32 posY)
{
    char line[BATTERY_GRAPH_COLS + 1];
    u8 rows[BATTERY_GRAPH_COLS];
    char text[32];
    char text2[32];
    u32 n = g_sampleCount;

    if (n < 2)
        return Draw_DrawFormattedString(10, posY, COLOR_YELLOW,
                                        Lang(STR_NEED_AT_LEAST_2_SAMPLES_HAVE), n);

    u32 cols = n < BATTERY_GRAPH_COLS ? n : BATTERY_GRAPH_COLS;
    u32 minY = BATTERY_PCT_MAX;
    u32 maxY = 0;

    for (u32 i = 0; i < n; ++i)
    {
        u32 v = batteryGetSample(i)->pctQ8;
        if (v < minY) minY = v;
        if (v > maxY) maxY = v;
    }

    if (maxY - minY < 256u)
    {
        u32 mid = (minY + maxY) / 2u;
        minY = mid > 128u ? mid - 128u : 0u;
        maxY = minY + 256u;
    }

    for (u32 c = 0; c < cols; ++c)
    {
        u32 idx = (cols == 1u) ? (n - 1u) : (c * (n - 1u)) / (cols - 1u);
        u32 v = batteryGetSample(idx)->pctQ8;
        rows[c] = (u8)(((maxY - v) * (BATTERY_GRAPH_ROWS - 1u)) / (maxY - minY));
    }

    batteryFormatPercent(text, maxY);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "%s %%\n", text);

    for (u32 r = 0; r < BATTERY_GRAPH_ROWS; ++r)
    {
        for (u32 c = 0; c < cols; ++c)
            line[c] = (rows[c] == r) ? '#' : ((rows[c] < r) ? '.' : ' ');
        line[cols] = 0;
        posY = Draw_DrawFormattedString(10, posY, COLOR_LIME, "%s\n", line);
    }

    batteryFormatPercent(text, minY);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, "%s %%\n", text);

    batteryFormatDuration(text, batteryGetSample(n - 1)->timeSec - batteryGetSample(0)->timeSec);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_SPAN_OVER_SAMPLES), text, n);

    batteryFormatPercent(text, batteryGetSample(0)->pctQ8);
    batteryFormatPercent(text2, batteryGetSample(n - 1)->pctQ8);
    posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang(STR_OLDEST_NEWEST), text, text2);

    return posY;
}

void RosalinaMenu_BatteryInfo(void)
{
    static const char *pageNames[3] = { "Now", "Estimates", "History" };
    u32 page = 0;
    Result csvRes = 0;
    bool csvDone = false;

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        batteryRefresh();

        Draw_Lock();
        Draw_ClearFramebuffer();

        Draw_DrawFormattedString(10, 10, COLOR_TITLE, Lang(STR_BATTERY_3), pageNames[page], page + 1u);

        u32 posY = 30;

        if (!g_hasReading)
        {
            posY = Draw_DrawBilingual(10, posY, COLOR_RED, "mcu::HWC ni sesshoku dekimasen", "mcu::HWC is not reachable");
        }
        else
        {
            switch (page)
            {
                case 0:  posY = batteryDrawNow(posY);        break;
                case 1:  posY = batteryDrawEstimates(posY);  break;
                default: posY = batteryDrawGraph(posY);      break;
            }
        }

        if (csvDone)
            posY = Draw_DrawFormattedString(10, posY, R_SUCCEEDED(csvRes) ? COLOR_LIME : COLOR_RED, "\n%s\n",
                                            R_SUCCEEDED(csvRes) ? "Saved " BATTERY_CSV_PATH : "CSV export failed");

        Draw_DrawString(10, 220, COLOR_TITLE, Lang(STR_L_R_PAGE_X_RESET_Y_EXPORT_B_BACK));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if (pressed & BUTTON_B)
            return;
        if (pressed & (BUTTON_R1 | BUTTON_RIGHT))
            page = (page + 1u) % 3u;
        if (pressed & (BUTTON_L1 | BUTTON_LEFT))
            page = (page + 2u) % 3u;
        if (pressed & BUTTON_X)
        {
            batteryResetStats();
            csvDone = false;
            Battery_Sample();
        }
        if (pressed & BUTTON_Y)
        {
            csvRes = batteryExportCsv();
            csvDone = true;
        }
    }
    while (!terminationRequest);
}
