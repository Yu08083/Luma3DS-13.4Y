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

#pragma once

#include <3ds/types.h>
#include "menu.h"

typedef enum LangCode {
    LANG_EN = 0,
    LANG_JP = 1,
    LANG_COUNT
} LangCode;

enum StrId {
    STR_OPERATION_FAILED_0X,
    STR_POWER_OFF_REBOOT,
    STR_PRESS_A_TO_POWER_OFF_PRESS_Y_TO_REBOOT_P,
    STR_VERSION_SWITCH_CHAINLOAD,
    STR_CURRENT_V_V,
    STR_A_LAUNCH_B_GO_BACK,
    STR_ROSALINA_SYSTEM_INFO,
    STR_TOP_SCREEN_TYPE,
    STR_BOTTOM_SCREEN_TYPE,
    STR_KERNEL_VERSION,
    STR_MCU_FW_VERSION,
    STR_PMIC_VENDOR,
    STR_BATTERY_VENDOR,
    STR_ROSALINA_DEBUG_INFO,
    STR_RETURN_TO_HOME_MENU,
    STR_PRESS_A_TO_CONFIRM_PRESS_B_TO_GO_BACK,
    STR_TIME_SPENT_CONVERTING_MS,
    STR_TIME_SPENT_WRITING_FILES_MS,
    STR_SELECT_NEXT_A_SAVE_B_CANCEL,
    STR_TOUCH_BAR_BOX_SWATCHES_TO_PICK,
    STR_NO_GAME_IS_RUNNING,
    STR_OPEN_A_GAME_FIRST_THEN_RUN_THIS,
    STR_ALL_PRESENT_REPORT_REMOVED,
    STR_FREE_MB,
    STR_SD_FREE_N_A,
    STR_SD_FREE_MB,
    STR_CHECKING_FILES,
    STR_SD_CHECK_LEGEND,
    STR_PRESS_ANY_BUTTON,
    STR_A_OPEN_X_COPY_Y_CUT_R_PASTE,
    STR_SELECT_DELETE_START_RENAME_L_NEW_DIR,
    STR_CHARGE,
    STR_VOLTAGE_V,
    STR_PTM_LEVEL_5,
    STR_BATTERY_MAKER,
    STR_PMIC_MAKER,
    STR_NOMINAL_PACK_MAH,
    STR_SAMPLES_HELD,
    STR_COLLECTING_DATA_SAMPLES_ONE_EVERY_S_3_NE,
    STR_RATE_H,
    STR_ESTIMATED_MA_W,
    STR_REMAINING_RATE_TOO_FLAT,
    STR_UNTIL_FULL,
    STR_UNTIL_EMPTY,
    STR_UNTIL_10,
    STR_REMAINING_DIRECTION_UNCLEAR,
    STR_FIT_WINDOW_PTS,
    STR_SINCE_RESET,
    STR_THIS_PHASE,
    STR_CHARGE_RANGE,
    STR_VOLTAGE_RANGE_V,
    STR_PLUG_EVENTS,
    STR_NEED_AT_LEAST_2_SAMPLES_HAVE,
    STR_SPAN_OVER_SAMPLES,
    STR_OLDEST_NEWEST,
    STR_BATTERY_3,
    STR_L_R_PAGE_X_RESET_Y_EXPORT_B_BACK,
    STR_ANY_USERNAME_AND_PASSWORD,
    STR_PASSIVE_MODE_ONLY,
    STR_KEEPS_RUNNING_WITH_THE_MENU_CLOSED,
    STR_A_STOP_B_BACK,
    STR_SERVES_THE_SD_CARD_OVER_FTP_SO_YOU,
    STR_CAN_MANAGE_FILES_FROM_A_PC,
    STR_WI_FI_MUST_BE_CONNECTED_FIRST,
    STR_A_START_B_BACK,
    STR_REGION,
    STR_WRITTEN_KB,
    STR_DONE_REGIONS_KB,
    STR_CANCELLED_REGIONS_KB,
    STR_MAP_TXT_LISTS_EVERY_REGION,
    STR_MEMORY_DUMP_PICK_A_PROCESS,
    STR_A_DUMP_ALL_X_CODE_ONLY,
    STR_Y_REFRESH_B_BACK,
    STR_A_USE_X_PIN_UNPIN_B_NONE,
    STR_X_BACK_TO_THE_WARNING,
    STR_MATCHED_LOCATIONS,
    STR_A_TYPE_X_BACKSPACE_Y_CASE,
    STR_START_CONFIRM_B_CANCEL,
    STR_A_CONFIRM_B_CANCEL,
    STR_COUNT
};

extern LangCode g_language;

const char *Lang(u32 id);
void        Lang_Load(void);
void        Lang_Save(void);

void        RosalinaMenu_Language(void);

__attribute__((format_arg(1)))
const char *Lang_Translate(const char *text);
