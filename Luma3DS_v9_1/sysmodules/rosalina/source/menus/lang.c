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
#include "menus/lang.h"
#include "draw.h"
#include "fmt.h"
#include "ifile.h"
#include "memory.h"

#define LANG_CFG_PATH "/luma/lang.cfg"

LangCode g_language = LANG_EN;
static bool g_langLoaded = false;

static const char *const g_strings[STR_COUNT][LANG_COUNT] =
{
    [STR_OPERATION_FAILED_0X] = { "Operation failed (0x%08lx).", "Sousa ni shippai (0x%08lx)." },
    [STR_POWER_OFF_REBOOT] = { "Power Off / Reboot", "Dengen off / Saikidou" },
    [STR_PRESS_A_TO_POWER_OFF_PRESS_Y_TO_REBOOT_P] = { "Press A to power off.\nPress Y to reboot.\nPress B to go back.", "A: dengen off\nY: saikidou\nB: modoru" },
    [STR_VERSION_SWITCH_CHAINLOAD] = { "Version switch chainload", "Version kirikae chainload" },
    [STR_CURRENT_V_V] = { "Current v%s   ->   v%s", "Genzai v%s   ->   v%s" },
    [STR_A_LAUNCH_B_GO_BACK] = { "A: launch   B: go back", "A: jikkou   B: modoru" },
    [STR_ROSALINA_SYSTEM_INFO] = { "Rosalina -- System info", "Rosalina -- System jouhou" },
    [STR_TOP_SCREEN_TYPE] = { "Top screen type:    %s\n", "Ue gamen no shurui:  %s\n" },
    [STR_BOTTOM_SCREEN_TYPE] = { "Bottom screen type: %s\n\n", "Shita gamen no shurui: %s\n\n" },
    [STR_KERNEL_VERSION] = { "Kernel version:     %lu.%lu-%lu\n\n", "Kernel version:     %lu.%lu-%lu\n\n" },
    [STR_MCU_FW_VERSION] = { "MCU FW version:     %lu.%lu\n", "MCU FW version:     %lu.%lu\n" },
    [STR_PMIC_VENDOR] = { "PMIC vendor:        %hhu\n", "PMIC meekaa:        %hhu\n" },
    [STR_BATTERY_VENDOR] = { "Battery vendor:     %hhu\n\n", "Denchi meekaa:      %hhu\n\n" },
    [STR_ROSALINA_DEBUG_INFO] = { "Rosalina -- Debug info", "Rosalina -- Debug jouhou" },
    [STR_RETURN_TO_HOME_MENU] = { "Return to HOME Menu", "HOME menu ni modoru" },
    [STR_PRESS_A_TO_CONFIRM_PRESS_B_TO_GO_BACK] = { "Press A to confirm.\nPress B to go back.", "A: kettei\nB: modoru" },
    [STR_TIME_SPENT_CONVERTING_MS] = { "Time spent converting:    %5lums\n", "Henkan jikan:      %5lums\n" },
    [STR_TIME_SPENT_WRITING_FILES_MS] = { "Time spent writing files: %5lums\n", "Kakikomi jikan:    %5lums\n" },
    [STR_SELECT_NEXT_A_SAVE_B_CANCEL] = { "SELECT: next   A: save   B: cancel", "SELECT: tsugi  A: hozon  B: cancel" },
    [STR_TOUCH_BAR_BOX_SWATCHES_TO_PICK] = { "Touch bar / box / swatches to pick", "Bar / box / mihon wo touch shite erabu" },
    [STR_NO_GAME_IS_RUNNING] = { "No game is running.", "Game ga kidou shite imasen." },
    [STR_OPEN_A_GAME_FIRST_THEN_RUN_THIS] = { "Open a game first, then run this.", "Saki ni game wo kidou shite kudasai." },
    [STR_ALL_PRESENT_REPORT_REMOVED] = { "All present. Report removed.", "Subete arimasu. Report wo sakujo." },
    [STR_FREE_MB] = { "Free: %u MB", "Kuuki: %u MB" },
    [STR_SD_FREE_N_A] = { "SD free: N/A", "SD kuuki: fumei" },
    [STR_SD_FREE_MB] = { "SD free: %u MB", "SD kuuki: %u MB" },
    [STR_CHECKING_FILES] = { "Checking files...", "Fairu wo kakunin shiteimasu..." },
    [STR_SD_CHECK_LEGEND] = { "OK: fine   --: missing   NG: broken", "OK: seijou   --: nashi   NG: kowarete imasu" },
    [STR_PRESS_ANY_BUTTON] = { "Press any button.", "Nanika button wo oshite kudasai." },
    [STR_A_OPEN_X_COPY_Y_CUT_R_PASTE] = { "A: open   X: copy  Y: cut  R: paste", "A: hiraku X: copy Y: kiritori R: hari" },
    [STR_SELECT_DELETE_START_RENAME_L_NEW_DIR] = { "SELECT: delete  START: rename  L: new dir", "SELECT: sakujo START: kaimei L: folder" },
    [STR_CHARGE] = { "Charge:        %s %%\n", "Zanryou:       %s %%\n" },
    [STR_VOLTAGE_V] = { "Voltage:       %lu.%03lu V\n", "Denatsu:       %lu.%03lu V\n" },
    [STR_PTM_LEVEL_5] = { "PTM level:     %lu / 5\n", "PTM reberu:    %lu / 5\n" },
    [STR_BATTERY_MAKER] = { "Battery maker: %lu\n", "Denchi meekaa: %lu\n" },
    [STR_PMIC_MAKER] = { "PMIC maker:    %lu\n", "PMIC meekaa:   %lu\n" },
    [STR_NOMINAL_PACK_MAH] = { "\nNominal pack:  %lu mAh\n", "\nKousho youryou:%lu mAh\n" },
    [STR_SAMPLES_HELD] = { "Samples held:  %lu / %lu\n", "Sample suu:    %lu / %lu\n" },
    [STR_COLLECTING_DATA_SAMPLES_ONE_EVERY_S_3_NE] = { "Collecting data: %lu samples,\none every %lu s, 3 needed.\n\n", "Data shuushuu chuu: %lu ken,\n%lu byou goto, 3 ken hitsuyou.\n\n" },
    [STR_RATE_H] = { "Rate:          %s %%/h\n", "Sokudo:        %s %%/h\n" },
    [STR_ESTIMATED_MA_W] = { "Estimated:     %lu mA  (%lu.%02lu W)\n", "Suitei:        %lu mA  (%lu.%02lu W)\n" },
    [STR_REMAINING_RATE_TOO_FLAT] = { "Remaining:     rate too flat\n", "Nokori:        henka ga sukunai\n" },
    [STR_UNTIL_FULL] = { "Until full:    %s\n", "Mantan made:   %s\n" },
    [STR_UNTIL_EMPTY] = { "Until empty:   %s\n", "Kara made:     %s\n" },
    [STR_UNTIL_10] = { "Until 10%%:     %s\n", "10%% made:      %s\n" },
    [STR_REMAINING_DIRECTION_UNCLEAR] = { "Remaining:     direction unclear\n", "Nokori:        houkou ga fumei\n" },
    [STR_FIT_WINDOW_PTS] = { "Fit window:    %s, %lu pts\n\n", "Kaiseki han-i: %s, %lu ten\n\n" },
    [STR_SINCE_RESET] = { "Since reset:   %s, %s %%\n", "Reset kara:    %s, %s %%\n" },
    [STR_THIS_PHASE] = { "This phase:    %s, %s %%\n", "Kono kukan:    %s, %s %%\n" },
    [STR_CHARGE_RANGE] = { "Charge range:  %s - %s %%\n", "Zanryou han-i: %s - %s %%\n" },
    [STR_VOLTAGE_RANGE_V] = { "Voltage range: %lu.%03lu - %lu.%03lu V\n", "Denatsu haba:  %lu.%03lu - %lu.%03lu V\n" },
    [STR_PLUG_EVENTS] = { "Plug events:   %lu\n", "Sashinuki suu: %lu\n" },
    [STR_NEED_AT_LEAST_2_SAMPLES_HAVE] = { "Need at least 2 samples (have %lu).\n", "Sample ga 2 ken hitsuyou (genzai %lu).\n" },
    [STR_SPAN_OVER_SAMPLES] = { "\nSpan: %s over %lu samples\n", "\nKikan: %s / %lu ken\n" },
    [STR_OLDEST_NEWEST] = { "Oldest %s %% -> newest %s %%\n", "Furui %s %% -> atarashii %s %%\n" },
    [STR_BATTERY_3] = { "Battery -- %s (%lu/3)", "Denchi -- %s (%lu/3)" },
    [STR_L_R_PAGE_X_RESET_Y_EXPORT_B_BACK] = { "L/R page  X reset  Y export  B back", "L/R page X reset Y export B modoru" },
    [STR_ANY_USERNAME_AND_PASSWORD] = { "Any username and password.\n", "User mei to password wa nandemo ka.\n" },
    [STR_PASSIVE_MODE_ONLY] = { "Passive mode only.\n\n", "Passive mode nomi taiou.\n\n" },
    [STR_KEEPS_RUNNING_WITH_THE_MENU_CLOSED] = { "Keeps running with the menu closed.\n", "Menu wo tojite mo ugokitsuzukemasu.\n" },
    [STR_A_STOP_B_BACK] = { "A: stop      B: back", "A: teishi    B: modoru" },
    [STR_SERVES_THE_SD_CARD_OVER_FTP_SO_YOU] = { "Serves the SD card over FTP so you\n", "SD card wo FTP de koukai shi,\n" },
    [STR_CAN_MANAGE_FILES_FROM_A_PC] = { "can manage files from a PC.\n\n", "PC kara file sousa ga dekimasu.\n\n" },
    [STR_WI_FI_MUST_BE_CONNECTED_FIRST] = { "Wi-Fi must be connected first.\n", "Saki ni Wi-Fi ni setsuzoku shite kudasai.\n" },
    [STR_A_START_B_BACK] = { "A: start     B: back", "A: kaishi    B: modoru" },
    [STR_REGION] = { "Region %lu / %lu", "Ryouiki %lu / %lu" },
    [STR_WRITTEN_KB] = { "Written: %lu KB   ", "Kakikomi zumi: %lu KB   " },
    [STR_DONE_REGIONS_KB] = { "Done: %lu regions, %lu KB", "Kanryou: %lu ryouiki, %lu KB" },
    [STR_CANCELLED_REGIONS_KB] = { "Cancelled: %lu regions, %lu KB", "Chuushi: %lu ryouiki, %lu KB" },
    [STR_MAP_TXT_LISTS_EVERY_REGION] = { "map.txt lists every region.", "map.txt ni zen ryouiki no ichiran." },
    [STR_MEMORY_DUMP_PICK_A_PROCESS] = { "Memory dump -- pick a process", "Memory dump -- process wo erabu" },
    [STR_A_DUMP_ALL_X_CODE_ONLY] = { "A: dump all   X: code only", "A: zenbu   X: code nomi" },
    [STR_Y_REFRESH_B_BACK] = { "Y: refresh    B: back", "Y: saishutoku B: modoru" },
    [STR_A_USE_X_PIN_UNPIN_B_NONE] = { "A: use   X: pin/unpin   B: none", "A: shiyou X: joyou kirikae B: nashi" },
    [STR_X_BACK_TO_THE_WARNING] = { "X: back to the warning", "X: keikoku ni modoru" },
    [STR_MATCHED_LOCATIONS] = { "Matched locations", "Icchi shita basho" },
    [STR_A_TYPE_X_BACKSPACE_Y_CASE] = { "A: type   X: backspace   Y: case", "A: nyuuryoku X: 1moji kesu Y: daishou" },
    [STR_START_CONFIRM_B_CANCEL] = { "START: confirm   B: cancel", "START: kettei   B: cancel" },
    [STR_A_CONFIRM_B_CANCEL] = { "A: confirm      B: cancel", "A: kettei       B: cancel" },
};

const char *Lang(u32 id)
{
    if (!g_langLoaded)
        Lang_Load();

    if (id >= STR_COUNT)
        return "";

    const char *s = g_strings[id][g_language];

    return (s != NULL && s[0] != 0) ? s : g_strings[id][LANG_EN];
}

void Lang_Load(void)
{
    IFile file;
    u64   total = 0;
    char  buf[4] = { 0 };

    g_langLoaded = true;

    if (R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, LANG_CFG_PATH), FS_OPEN_READ)))
        return;

    file.pos = 0;
    if (R_SUCCEEDED(IFile_Read(&file, &total, buf, 2)) && total >= 1 && buf[0] == '1')
        g_language = LANG_JP;

    IFile_Close(&file);
}

void Lang_Save(void)
{
    IFile      file;
    FS_Archive archive = 0;
    u64        written = 0;
    char       buf[2];

    if (R_SUCCEEDED(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
    {
        FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, LANG_CFG_PATH));
        FSUSER_CloseArchive(archive);
    }

    if (R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""),
                            fsMakePath(PATH_ASCII, LANG_CFG_PATH), FS_OPEN_CREATE | FS_OPEN_WRITE)))
        return;

    file.pos = 0;
    buf[0] = (g_language == LANG_JP) ? '1' : '0';
    buf[1] = '\n';

    IFile_Write(&file, &written, buf, 2, FS_WRITE_FLUSH);
    IFile_Close(&file);
}

void RosalinaMenu_Language(void)
{
    if (!g_langLoaded)
        Lang_Load();

    LangCode previous = g_language;

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    do
    {
        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 10, COLOR_TITLE, "Language / Gengo");

        u32 posY = 30;

        posY = Draw_DrawFormattedString(10, posY, g_language == LANG_EN ? COLOR_LIME : COLOR_WHITE,
                                        "%c English\n", g_language == LANG_EN ? '>' : ' ');
        posY = Draw_DrawFormattedString(10, posY, g_language == LANG_JP ? COLOR_LIME : COLOR_WHITE,
                                        "%c Nihongo (romaji)\n", g_language == LANG_JP ? '>' : ' ');

        posY = Draw_DrawString(10, posY + SPACING_Y, COLOR_WHITE,
                               "Kanji cannot be drawn by the 3DS font,\nso Japanese is shown in romaji.\n");

        Draw_DrawString(10, 214, COLOR_WHITE, "Up/Down: select   A: save");
        Draw_DrawString(10, 225, COLOR_WHITE, "B: back");

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if (terminationRequest || (pressed & BUTTON_B))
            break;

        if (pressed & (BUTTON_UP | BUTTON_DOWN))
            g_language = (g_language == LANG_EN) ? LANG_JP : LANG_EN;

        if (pressed & BUTTON_A)
        {
            Lang_Save();
            return;
        }
    }
    while (!terminationRequest);

    g_language = previous;
}
