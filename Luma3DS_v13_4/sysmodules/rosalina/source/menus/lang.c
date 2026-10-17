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

LangCode g_language = LANG_JP;
static bool g_langLoaded = false;

static const char *const g_strings[STR_COUNT][LANG_COUNT] =
{
    [STR_OPERATION_FAILED_0X] = { "Operation failed (0x%08lx).", "操作失敗 (0x%08lx)。" },
    [STR_POWER_OFF_REBOOT] = { "Power Off / Reboot", "電源を切る / 再起動" },
    [STR_PRESS_A_TO_POWER_OFF_PRESS_Y_TO_REBOOT_P] = { "Press A to power off.\nPress Y to reboot.\nPress B to go back.", "A: 電源を切る\nY: 再起動\nB: 戻る" },
    [STR_VERSION_SWITCH_CHAINLOAD] = { "Version switch chainload", "バージョン切り替え" },
    [STR_CURRENT_V_V] = { "Current %s   ->   %s", "現在 %s   ->   %s" },
    [STR_A_LAUNCH_B_GO_BACK] = { "A: launch   B: go back", "A: 起動   B: 戻る" },
    [STR_ROSALINA_SYSTEM_INFO] = { "Rosalina -- System info", "Rosalina -- 本体情報" },
    [STR_TOP_SCREEN_TYPE] = { "Top screen type:    %s\n", "上画面の種類:  %s\n" },
    [STR_BOTTOM_SCREEN_TYPE] = { "Bottom screen type: %s\n\n", "下画面の種類:  %s\n\n" },
    [STR_KERNEL_VERSION] = { "Kernel version:     %lu.%lu-%lu\n\n", "カーネル:      %lu.%lu-%lu\n\n" },
    [STR_MCU_FW_VERSION] = { "MCU FW version:     %lu.%lu\n", "MCU FW:        %lu.%lu\n" },
    [STR_PMIC_VENDOR] = { "PMIC vendor:        %hhu\n", "PMICメーカー:  %hhu\n" },
    [STR_BATTERY_VENDOR] = { "Battery vendor:     %hhu\n\n", "電池メーカー:  %hhu\n\n" },
    [STR_ROSALINA_DEBUG_INFO] = { "Rosalina -- Debug info", "Rosalina -- デバッグ情報" },
    [STR_RETURN_TO_HOME_MENU] = { "Return to HOME Menu", "HOMEメニューに戻る" },
    [STR_PRESS_A_TO_CONFIRM_PRESS_B_TO_GO_BACK] = { "Press A to confirm.\nPress B to go back.", "A: 決定\nB: 戻る" },
    [STR_TIME_SPENT_CONVERTING_MS] = { "Time spent converting:    %5lums\n", "変換時間:     %5lums\n" },
    [STR_TIME_SPENT_WRITING_FILES_MS] = { "Time spent writing files: %5lums\n", "書き込み時間: %5lums\n" },
    [STR_SELECT_NEXT_A_SAVE_B_CANCEL] = { "SELECT: next   A: save   B: cancel", "SELECT: 対象切替  A: 保存  B: 取消" },
    [STR_TOUCH_BAR_BOX_SWATCHES_TO_PICK] = { "Touch bar / box / swatches to pick", "バー・四角・色見本をタッチして選択" },
    [STR_NO_GAME_IS_RUNNING] = { "No game is running.", "ゲームが起動していません。" },
    [STR_OPEN_A_GAME_FIRST_THEN_RUN_THIS] = { "Open a game first, then run this.", "先にゲームを起動してください。" },
    [STR_ALL_PRESENT_REPORT_REMOVED] = { "All present. Report removed.", "すべて確認できました。レポート削除済み。" },
    [STR_FREE_MB] = { "Free: %u MB", "空き: %u MB" },
    [STR_SD_FREE_N_A] = { "SD free: N/A", "SD空き: 不明" },
    [STR_SD_FREE_MB] = { "SD free: %u MB", "SD空き: %u MB" },
    [STR_CHECKING_FILES] = { "Checking files...", "ファイルを確認中..." },
    [STR_SD_CHECK_LEGEND] = { "OK: fine   --: missing   NG: broken", "OK: 正常  --: なし  NG: 破損" },
    [STR_PRESS_ANY_BUTTON] = { "Press any button.", "ボタンを押してください。" },
    [STR_A_OPEN_X_COPY_Y_CUT_R_PASTE] = { "A: open   X: copy  Y: cut  R: paste", "A: 開く X: コピー Y: 切取 R: 貼付" },
    [STR_SELECT_DELETE_START_RENAME_L_NEW_DIR] = { "SELECT: delete  START: rename  L: new dir", "SELECT: 削除 START: 改名 L: 新規フォルダ" },
    [STR_CHARGE] = { "Charge:        %s %%\n", "残量:         %s %%\n" },
    [STR_VOLTAGE_V] = { "Voltage:       %lu.%03lu V\n", "電圧:         %lu.%03lu V\n" },
    [STR_PTM_LEVEL_5] = { "PTM level:     %lu / 5\n", "PTMレベル:    %lu / 5\n" },
    [STR_BATTERY_MAKER] = { "Battery maker: %lu\n", "電池メーカー: %lu\n" },
    [STR_PMIC_MAKER] = { "PMIC maker:    %lu\n", "PMICメーカー: %lu\n" },
    [STR_NOMINAL_PACK_MAH] = { "\nNominal pack:  %lu mAh\n", "\n公称容量:     %lu mAh\n" },
    [STR_SAMPLES_HELD] = { "Samples held:  %lu / %lu\n", "記録数:       %lu / %lu\n" },
    [STR_COLLECTING_DATA_SAMPLES_ONE_EVERY_S_3_NE] = { "Collecting data: %lu samples,\none every %lu s, 3 needed.\n\n", "データ収集中: %lu件\n%lu秒ごとに記録。3件必要です。\n\n" },
    [STR_RATE_H] = { "Rate:          %s %%/h\n", "増減速度:     %s %%/h\n" },
    [STR_ESTIMATED_MA_W] = { "Estimated:     %lu mA  (%lu.%02lu W)\n", "推定:         %lu mA (%lu.%02lu W)\n" },
    [STR_REMAINING_RATE_TOO_FLAT] = { "Remaining:     rate too flat\n", "残り時間:     変化が小さすぎます\n" },
    [STR_UNTIL_FULL] = { "Until full:    %s\n", "満充電まで:   %s\n" },
    [STR_UNTIL_EMPTY] = { "Until empty:   %s\n", "空になるまで: %s\n" },
    [STR_UNTIL_10] = { "Until 10%%:     %s\n", "10%%まで:      %s\n" },
    [STR_REMAINING_DIRECTION_UNCLEAR] = { "Remaining:     direction unclear\n", "残り時間:     増減方向が不明\n" },
    [STR_FIT_WINDOW_PTS] = { "Fit window:    %s, %lu pts\n\n", "解析範囲:     %s / %lu件\n\n" },
    [STR_SINCE_RESET] = { "Since reset:   %s, %s %%\n", "リセット後:   %s, %s %%\n" },
    [STR_THIS_PHASE] = { "This phase:    %s, %s %%\n", "現在の区間:   %s, %s %%\n" },
    [STR_CHARGE_RANGE] = { "Charge range:  %s - %s %%\n", "残量範囲:     %s - %s %%\n" },
    [STR_VOLTAGE_RANGE_V] = { "Voltage range: %lu.%03lu - %lu.%03lu V\n", "電圧範囲:     %lu.%03lu - %lu.%03lu V\n" },
    [STR_PLUG_EVENTS] = { "Plug events:   %lu\n", "抜き差し回数: %lu\n" },
    [STR_NEED_AT_LEAST_2_SAMPLES_HAVE] = { "Need at least 2 samples (have %lu).\n", "記録が2件以上必要です (現在%lu件)。\n" },
    [STR_SPAN_OVER_SAMPLES] = { "\nSpan: %s over %lu samples\n", "\n期間: %s / %lu件\n" },
    [STR_OLDEST_NEWEST] = { "Oldest %s %% -> newest %s %%\n", "最古 %s %% -> 最新 %s %%\n" },
    [STR_BATTERY_3] = { "Battery -- %s (%lu/3)", "電池 -- %s (%lu/3)" },
    [STR_L_R_PAGE_X_RESET_Y_EXPORT_B_BACK] = { "L/R page  X reset  Y export  B back", "L/R: ページ X: 初期化 Y: 出力 B: 戻る" },
    [STR_ANY_USERNAME_AND_PASSWORD] = { "Any username and password.\n", "ユーザー名とパスワードは任意です。\n" },
    [STR_PASSIVE_MODE_ONLY] = { "Passive mode only.\n\n", "パッシブモードのみ対応。\n\n" },
    [STR_KEEPS_RUNNING_WITH_THE_MENU_CLOSED] = { "Keeps running with the menu closed.\n", "メニューを閉じても動作します。\n" },
    [STR_A_STOP_B_BACK] = { "A: stop      B: back", "A: 停止   B: 戻る" },
    [STR_SERVES_THE_SD_CARD_OVER_FTP_SO_YOU] = { "Serves the SD card over FTP so you\n", "SDカードをFTPで共有し、\n" },
    [STR_CAN_MANAGE_FILES_FROM_A_PC] = { "can manage files from a PC.\n\n", "PCからファイルを操作できます。\n\n" },
    [STR_WI_FI_MUST_BE_CONNECTED_FIRST] = { "Wi-Fi must be connected first.\n", "先にWi-Fiに接続してください。\n" },
    [STR_A_START_B_BACK] = { "A: start     B: back", "A: 開始   B: 戻る" },
    [STR_REGION] = { "Region %lu / %lu", "領域 %lu / %lu" },
    [STR_WRITTEN_KB] = { "Written: %lu KB   ", "書き込み済み: %lu KB   " },
    [STR_DONE_REGIONS_KB] = { "Done: %lu regions, %lu KB", "完了: %lu領域, %lu KB" },
    [STR_CANCELLED_REGIONS_KB] = { "Cancelled: %lu regions, %lu KB", "中止: %lu領域, %lu KB" },
    [STR_MAP_TXT_LISTS_EVERY_REGION] = { "map.txt lists every region.", "全領域の一覧: map.txt" },
    [STR_MEMORY_DUMP_PICK_A_PROCESS] = { "Memory dump -- pick a process", "メモリダンプ -- プロセス選択" },
    [STR_A_DUMP_ALL_X_CODE_ONLY] = { "A: dump all   X: code only", "A: すべて出力   X: コードのみ" },
    [STR_Y_REFRESH_B_BACK] = { "Y: refresh    B: back", "Y: 更新   B: 戻る" },
    [STR_A_USE_X_PIN_UNPIN_B_NONE] = { "A: use   X: pin/unpin   B: none", "A: 使用 X: 常用切替 B: 使用しない" },
    [STR_X_BACK_TO_THE_WARNING] = { "X: back to the warning", "X: 警告に戻る" },
    [STR_MATCHED_LOCATIONS] = { "Matched locations", "一致した場所" },
    [STR_A_TYPE_X_BACKSPACE_Y_CASE] = { "A: type   X: backspace   Y: case", "A: 入力 X: 1文字削除 Y: 大小切替" },
    [STR_START_CONFIRM_B_CANCEL] = { "START: confirm   B: cancel", "START: 決定   B: 取消" },
    [STR_A_CONFIRM_B_CANCEL] = { "A: confirm      B: cancel", "A: 決定   B: 取消" },
};

typedef struct { const char *english; const char *japanese; } LangEntry;

static const LangEntry g_labels[] =
{
    { "\nFailed: 0x%08lX", "\n失敗: 0x%08lX" },
    { "\nNominal pack:  %lu mAh\n", "\n公称容量:     %lu mAh\n" },
    { "\nSpan: %s over %lu samples\n", "\n期間: %s / %lu件\n" },
    { "\nSuccess!", "\n完了しました。" },
    { "\nValue: [%d%%]    ", "\n音量: [%d%%]    " },
    { "  * Entering sleep mode will reset the LED state!", "  * スリープでLEDの状態がリセットされます。" },
    { "  * LEDs cannot be toggled when the battery is low!", "  * 電池残量が少ないとLEDを切り替えられません。" },
    { "  * bottom framebuffer will be restored until\nyou exit.", "  * 終了するまで下画面は元に戻ります。" },
    { "  * value will be limited by calibration.\n", "  * 補正値によって範囲が制限されます。\n" },
    { " DISABLED", " 無効" },
    { " ENABLED ", " 有効 " },
    { " OFF", " 無効" },
    { " ON ", " 有効 " },
    { "(empty)", "(空)" },
    { "(none)", "(なし)" },
    { "--- WARNING ---", "--- 警告 ---" },
    { "3DS IP: %s", "本体IP: %s" },
    { "3DSX loading code by fincs", "3DSXの読み込み: fincs" },
    { "3gx / txt checker", "3gx / txtチェック" },
    { "A:           save to system config\n", "A: 本体設定に保存\n" },
    { "A: Regenerate   B: Back", "A: 再生成   B: 戻る" },
    { "A: Toggle   B: Back", "A: 切替   B: 戻る" },
    { "A: apply  X: off  Y: refresh  B: back", "A: 適用 X: 解除 Y: 更新 B: 戻る" },
    { "A: confirm      B: cancel", "A: 決定   B: 取消" },
    { "A: dump all   X: code only", "A: すべて出力   X: コードのみ" },
    { "A: launch   B: go back", "A: 起動   B: 戻る" },
    { "A: open   X: copy  Y: cut  R: paste", "A: 開く X: コピー Y: 切取 R: 貼付" },
    { "A: power off   B: go back", "A: 電源を切る   B: 戻る" },
    { "A: reboot   B: go back", "A: 再起動   B: 戻る" },
    { "A: save   B: cancel", "A: 保存   B: 取消" },
    { "A: start     B: back", "A: 開始   B: 戻る" },
    { "A: stop      B: back", "A: 停止   B: 戻る" },
    { "A: type   X: backspace   Y: case", "A: 入力 X: 1文字削除 Y: 大小切替" },
    { "A: use   X: pin/unpin   B: none", "A: 使用 X: 常用切替 B: 使用しない" },
    { "A: use anyway", "A: そのまま使用する" },
    { "Adapter:       %s\n", "ACアダプタ:   %s\n" },
    { "Advanced configuration...", "詳細設定..." },
    { "All present. Report removed.", "すべて確認できました。レポート削除済み。" },
    { "Already enabled!", "すでに有効です。" },
    { "An error occurred (ErrDisp)", "エラーが発生しました (ErrDisp)" },
    { "Any username and password.\n", "ユーザー名とパスワードは任意です。\n" },
    { "Applies when the game starts again.", "次回のゲーム起動から適用します。" },
    { "Auto-barrier adjustment behavior is restored on\nexit.\n\n", "終了すると視差バリアの自動調整に戻ります。\n\n" },
    { "B:           exit this menu\n\n", "B: 戻る\n\n" },
    { "B: Back", "B: 戻る" },
    { "B: back", "B: 戻る" },
    { "B: cancel", "B: 取消" },
    { "B: continue", "B: 続行" },
    { "B: don't use it", "B: 使用しない" },
    { "B: go back", "B: 戻る" },
    { "Background theme", "背景テーマ" },
    { "Background theme...", "背景テーマ..." },
    { "Barrier position: %2hhu\n", "視差バリアの位置: %2hhu\n" },
    { "Battery -- %s (%lu/3)", "電池 -- %s (%lu/3)" },
    { "Battery info", "バッテリー情報" },
    { "Battery maker: %lu\n", "電池メーカー: %lu\n" },
    { "Battery vendor:     %hhu\n\n", "電池メーカー:  %hhu\n\n" },
    { "Bottom screen type: %s\n\n", "下画面の種類:  %s\n\n" },
    { "Bottom screen:\n", "下画面:\n" },
    { "Brightness:  %13s    \n", "明るさ: %13s    \n" },
    { "CSV export failed", "CSV出力に失敗しました" },
    { "Can't start the debugger before the system has fi-\nnished loading.", "本体の起動が完了するまで\nデバッガーを開始できません。" },
    { "Can't start the input redirection before the system\nhas finished loading.", "本体の起動が完了するまで\n入力リダイレクトを開始できません。" },
    { "Can't sync time/date before the system\nhas finished loading.", "本体の起動が完了するまで\n日時を同期できません。" },
    { "Cancelled: %lu regions, %lu KB", "中止: %lu領域, %lu KB" },
    { "Cannot paste a folder into itself.", "フォルダをその中へ貼り付けられません。" },
    { "Center position:          %-5s\n\n", "中央位置:       %-5s\n\n" },
    { "Change screen brightness", "画面の明るさを変更" },
    { "Change the menu combo", "メニューの操作キーを変更" },
    { "Charge range:  %s - %s %%\n", "残量範囲:     %s - %s %%\n" },
    { "Charge:        %s %%\n", "残量:         %s %%\n" },
    { "Charging:      %s\n", "充電中:       %s\n" },
    { "Cheat list", "チート一覧" },
    { "Cheats", "チート" },
    { "Cheats...", "チート..." },
    { "Check SD Card", "SDカードを確認" },
    { "Checking files...", "ファイルを確認中..." },
    { "Clipboard is empty.", "コピー・切り取りした項目がありません。" },
    { "Clipboard: %s%s   ", "クリップボード: %s%s   " },
    { "Close the menu to apply.", "メニューを閉じると適用します。" },
    { "Collecting data: %lu samples,\none every %lu s, 3 needed.\n\n", "データ収集中: %lu件\n%lu秒ごとに記録。3件必要です。\n\n" },
    { "Colors", "配色" },
    { "Colors...", "配色..." },
    { "Companion plugin is not loaded.", "対応プラグインが読み込まれていません。" },
    { "Completed transfers: %lu", "転送完了: %lu" },
    { "Connected", "接続中" },
    { "Connecting...", "接続中..." },
    { "Connections: %lu", "接続数: %lu" },
    { "Consider freeing some space", "空き容量を増やしてください" },
    { "Contrast:    %13s    \n", "コントラスト: %13s    \n" },
    { "Control Wireless connection", "無線接続を管理" },
    { "Control volume", "音量を調整" },
    { "Controls: Up/Down for +-1, Right/Left for +-10.\n", "十字上下: +-1  十字左右: +-10\n" },
    { "Copied, but failed to delete the source.", "コピーは完了しましたが、\n元の項目を削除できませんでした。" },
    { "Copied.", "コピーしました。" },
    { "Copy", "コピー" },
    {"Could not read the whole swap file.", "スワップ全体を読み込めませんでした。"},
    {"Could not resize swap file.", "スワップのサイズを変更できませんでした。"},
    { "Could not save online ID.", "IDを保存できませんでした。" },
    { "Could not scan the whole file", "ファイル全体を検査できませんでした" },
    {"Could not write the whole swap file.", "スワップ全体を保存できませんでした。"},
    { "Couldn't allocate memblock", "メモリ領域を確保できません" },
    { "Couldn't get file size", "ファイルサイズを取得できません" },
    { "Couldn't map heap memory block", "ヒープ用メモリを割り当てられません" },
    { "Couldn't map plugin memory block", "プラグイン用メモリを割り当てられません" },
    { "Couldn't map process", "プロセスをメモリに割り当てられません" },
    { "Couldn't read file", "ファイルを読み取れません" },
    { "Couldn't read file.", "ファイルを読み取れません。" },
    { "Couldn't read plugin's code", "プラグインのコードを読み取れません" },
    { "Couldn't set memblock size.", "メモリ領域のサイズを設定できません。" },
    { "Credits", "クレジット" },
    { "Current %s   ->   %s", "現在 %s   ->   %s" },
    { "Current UTC offset:  %c%02d", "UTC時差: %c%02d" },
    { "Current UTC offset:  %c%02d%02d", "UTC時差: %c%02d%02d" },
    { "Current barrier position: %-2hhu\n", "現在のバリア位置: %-2hhu\n" },
    { "Current eye distance:     %-2d cm\n", "現在の目の距離:   %-2d cm\n" },
    { "Current luminance: %lu (min. %lu, max. %lu)\n\n", "現在の明るさ: %lu (最小%lu / 最大%lu)\n\n" },
    { "Current status:", "現在の状態:" },
    { "D-PAD to move, X to jump, Y to search, A to edit.", "十字: 移動 X: ジャンプ Y: 検索 A: 編集" },
    { "DSP firm. successfully written to /3ds/dspfirm.cdc\non the SD card.", "SDカードの /3ds/dspfirm.cdc に\nDSPファームウェアを保存しました。" },
    { "Debug info", "デバッグ情報" },
    { "Debugger options menu", "デバッガー設定" },
    { "Debugger options...", "デバッガー設定..." },
    { "Delete", "削除" },
    { "Delete this file?", "このファイルを削除しますか？" },
    { "Delete this folder and everything in it?", "このフォルダと中身をすべて削除しますか？" },
    { "Disable L2 cache", "L2キャッシュを無効化" },
    { "Disable debugger", "デバッガーを無効化" },
    { "Done: %lu regions, %lu KB", "完了: %lu領域, %lu KB" },
    { "Dump DSP firmware", "DSPファームウェアを出力" },
    { "ERROR: %08lx", "エラー: %08lx" },
    { "ERROR: %08x", "エラー: %08x" },
    { "Each position corresponds to 5.2mm horizontal eye\nmovement (assuming ideal viewing conditions).\n\n", "1段階につき目の横移動約5.2mmに相当します。\n(理想的な条件の場合)\n\n" },
    { "Enable L2 cache", "L2キャッシュを有効化" },
    { "Enable debugger", "デバッガーを有効化" },
    { "Error", "エラー" },
    { "Error code: 0x%08X", "エラーコード: 0x%08X" },
    { "Error code: 0x%08lX", "エラーコード: 0x%08lX" },
    { "Error code: 0x%08lx", "エラーコード: 0x%08lx" },
    { "Error: 0x%08lx", "エラー: 0x%08lx" },
    { "Estimated:     %lu mA  (%lu.%02lu W)\n", "推定:         %lu mA (%lu.%02lu W)\n" },
    { "Estimates", "推定" },
    { "FOUND", "あり" },
    { "FPS: %lu   Quality: %lu", "FPS: %lu   画質: %lu" },
    { "FTP server", "FTPサーバー" },
    { "Failed to allocate memory.", "メモリを確保できません。" },
    {"Failed to open swap file.", "スワップファイルを開けませんでした。"},
    { "Failed to read calibration data.\nPress B to exit this menu.\n", "補正データを読み取れません。\nB: 戻る\n" },
    { "Failed to write to the SD card", "SDカードへの書き込みに失敗しました" },
    { "Failed.", "失敗しました。" },
    { "File browser", "ファイルブラウザー" },
    { "File size: %lu KiB", "ファイル容量: %lu KiB" },
    { "Fit window:    %s, %lu pts\n\n", "解析範囲:     %s / %lu件\n\n" },
    { "Force-debug next application at launch", "次のアプリ起動時にデバッグ" },
    { "Forced connection successfully disabled.\nNote: auto-connection may remain broken.", "強制接続を解除しました。\n自動接続が復帰しない場合があります。" },
    { "Free: %u MB", "空き: %u MB" },
    { "Gamma:       %13s    \n", "ガンマ: %13s    \n" },
    { "History", "履歴" },
    { "Hold R to change the value faster.\n", "Rを押しながら操作すると大きく変わります。\n" },
    { "If you're currently on Test Menu,", "Test Menuを使用中の場合は、" },
    { "InputRedirection by Stary (PoC by ShinyQuagsire)", "入力転送: Stary (試作: ShinyQuagsire)" },
    { "InputRedirection stopped successfully.", "入力リダイレクトを停止しました。" },
    { "InputRedirection stopped successfully.\n\n", "入力リダイレクトを停止しました。\n\n" },
    { "Inserting or removing a card will reset the status,\nand you'll need to reinsert the cart if you want to\nplay it.\n\n", "カードを抜き差しすると状態が戻ります。\n遊ぶにはカードを挿し直してください。\n\n" },
    { "Invalid plugin file\nNot a valid 3GX plugin format!", "無効なプラグインです。\n3GX形式のファイルではありません。" },
    {"Invalid plugin layout or memory size.", "プラグインの構成またはサイズが不正です。"},
    { "Invalid save/load payloads.", "保存・読み込み用のペイロードが無効です。" },
    { "Invert:      %13s    \n", "反転: %13s    \n" },
    { "Keeps running with the menu closed.\n", "メニューを閉じても動作します。\n" },
    { "Kernel version:     %lu.%lu-%lu\n\n", "カーネル:      %lu.%lu-%lu\n\n" },
    { "L/R page  X reset  Y export  B back", "L/R: ページ X: 初期化 Y: 出力 B: 戻る" },
    { "Language / 言語", "Language / 言語" },
    { "Loading image...  B: cancel", "画像を読み込み中... B: 取消" },
    { "Low SD space", "SDカードの空き容量が不足" },
    { "MCU FW version:     %lu.%lu\n", "MCU FW:        %lu.%lu\n" },
    { "MCU firmware:  %lu.%lu\n", "MCU FW:       %lu.%lu\n" },
    { "MISSING", "なし" },
    { "Matched keywords:", "一致したキーワード:" },
    { "Matched locations", "一致した場所" },
    { "Memory dump", "メモリダンプ" },
    { "Memory dump -- pick a process", "メモリダンプ -- プロセス選択" },
    { "Memory viewer", "メモリビューアー" },
    { "Menu controls", "メニュー操作" },
    { "Menu controls...", "メニュー操作..." },
    { "Miscellaneous options menu", "その他の設定" },
    { "Miscellaneous options...", "その他の設定..." },
    { "Move", "移動" },
    { "Moved.", "移動しました。" },
    { "NWM isn't running.", "NWMが動作していません。" },
    { "Need at least 2 samples (have %lu).\n", "記録が2件以上必要です (現在%lu件)。\n" },
    { "Networking code & basic GDB functionality by Stary", "通信処理・GDBの基本機能: Stary" },
    { "New 3DS menu", "New 3DS設定" },
    { "New 3DS menu...", "New 3DS設定..." },
    { "New folder", "フォルダを作成" },
    { "New folder name", "新しいフォルダ名" },
    { "No cheats found for title %016llX", "チートがありません: %016llX" },
    { "No game is running.", "ゲームが起動していません。" },
    { "No processes", "プロセスがありません" },
    { "No readable regions found", "読み取れる領域がありません" },
    { "No suitable title found", "対応するタイトルが見つかりません" },
    { "No theme images found.", "テーマ画像がありません。" },
    {"No verified swap file is available.", "検証済みのスワップがありません。"},
    { "Nominal pack:  %lu mAh\n", "公称容量:     %lu mAh\n" },
    { "Now", "現在" },
    { "Nullify user time offset", "ユーザー時刻の補正を解除" },
    { "OFF", "無効" },
    { "OK: fine   --: missing   NG: broken", "OK: 正常  --: なし  NG: 破損" },
    { "ON", "有効" },
    { "Off", "無効" },
    { "Oldest %s %% -> newest %s %%\n", "最古 %s %% -> 最新 %s %%\n" },
    { "On", "有効" },
    { "Once you figure out the ideal central position, you\ncan then use it in the calibration submenu.\n\n", "最適な中央位置が分かったら、\n補正メニューでその値を設定できます。\n\n" },
    { "Online ID", "オンライン用ID" },
    { "Online ID regenerated.", "IDを再生成しました。" },
    { "Online ID...", "オンライン用ID..." },
    { "Online connection", "オンライン接続" },
    { "Open a game first, then run this.", "先にゲームを起動してください。" },
    { "Opening process...", "プロセスを開いています..." },
    { "Operation failed (%08lx).", "操作失敗 (%08lx)。" },
    { "Operation failed (%s).", "操作失敗 (%s)。" },
    { "Operation failed (0x%.8lx).", "操作失敗 (0x%.8lx)。" },
    { "Operation failed (0x%08lx).", "操作失敗 (0x%08lx)。" },
    { "Operation failed (0x%08lx).\n\nMake sure that HOME Menu is running and that your\nSD card is inserted.", "操作失敗 (0x%08lx)。\n\nHOMEメニューの起動と\nSDカードの挿入を確認してください。" },
    { "Operation failed (0x%08x).", "操作失敗 (0x%08x)。" },
    { "Operation succeeded.", "完了しました。" },
    { "Operation succeeded.\n\n", "完了しました。\n\n" },
    { "Operation succeeded.\n\nPlease reboot to finalize the changes.", "完了しました。\n\n変更を反映するには再起動してください。" },
    { "Optimal eye distance:     %-2d cm\n", "最適な目の距離:   %-2d cm\n" },
    { "Other features & Japanese translation: @yu_", "その他機能 & 日本語翻訳: @yu_" },
    { "Otherwise, simply exit and wait a few seconds.", "それ以外の場合は、終了して数秒待ってください。" },
    { "Outdated plugin file\nCheck for an updated plugin.", "プラグインの形式が古すぎます。\n新しいプラグインを確認してください。" },
    { "Outdated plugin loader\nCheck for Luma3DS updates.", "プラグインローダーが古すぎます。\n対応するLuma3DSを確認してください。" },
    { "PC viewer: NTR mode", "PC受信ソフト: NTRモード" },
    { "PMIC maker:    %lu\n", "PMICメーカー: %lu\n" },
    { "PMIC vendor:        %hhu\n", "PMICメーカー:  %hhu\n" },
    { "PTM level:     %lu / 5\n", "PTMレベル:    %lu / 5\n" },
    { "Passive mode only.\n\n", "パッシブモードのみ対応。\n\n" },
    { "Paste", "貼り付け" },
    { "Path is too long.", "パスが長すぎます。" },
    {"Path matches do not prove harmful behavior.", "パスの一致だけで危険性は断定できません。"},
    { "Paused (offline)", "一時停止 (オフライン)" },
    { "Please enter the new combo:", "新しい操作キーを押してください:" },
    { "Please enter the new combo: %s", "新しい操作キー: %s" },
    { "Plug events:   %lu\n", "抜き差し回数: %lu\n" },
    { "Plugin Loader: [Disabled]", "プラグイン: [無効]" },
    { "Plugin Loader: [Enabled]", "プラグイン: [有効]" },
    {"Plugin changed, is invalid, or could not be fully read.", "ファイルが変更されたか、正しく読み込めません。"},
    { "Plugin is only compatible with emulators", "このプラグインはエミュレーター専用です" },
    { "Plugin loader", "プラグインローダー" },
    { "Plugin selector", "プラグイン選択" },
    { "Power Off / Reboot", "電源を切る / 再起動" },
    { "Power off", "電源を切る" },
    { "Power off / reboot", "電源を切る / 再起動" },
    { "Press A to confirm.\nPress B to go back.", "A: 決定\nB: 戻る" },
    { "Press A to force a connection to slot, B to go back\n\n", "A: 選択した接続先に強制接続\nB: 戻る\n\n" },
    { "Press A to power off.\nPress Y to reboot.\nPress B to go back.", "A: 電源を切る\nY: 再起動\nB: 戻る" },
    { "Press A to start, B to exit.\n\n", "A: 開始   B: 戻る\n\n" },
    { "Press A to toggle, press B to go back.", "A: 切替   B: 戻る" },
    { "Press A to toggle, press B to go back.\n\n", "A: 切替   B: 戻る\n\n" },
    { "Press B to close", "B: 閉じる" },
    { "Press B to go back.", "B: 戻る" },
    { "Press L or R to switch between heap and code.", "L/R: ヒープ・コードを切替" },
    { "Press [B] to close", "B: 閉じる" },
    { "Press [D-PAD Up + B] to exit the app.", "十字上 + B: アプリを終了" },
    { "Press any button to reboot.", "ボタンを押すと再起動します。" },
    { "Press any button.", "ボタンを押してください。" },
    { "Process list", "プロセス一覧" },
    { "R/L:         +- 0.01\n", "R/L: +-0.01\n" },
    { "Rate:          %s %%/h\n", "増減速度:     %s %%/h\n" },
    { "Ready", "使用できます" },
    { "Reboot", "再起動" },
    { "Receiving file", "ファイルを受信中" },
    { "Region %lu / %lu", "領域 %lu / %lu" },
    { "Remaining:     direction unclear\n", "残り時間:     増減方向が不明\n" },
    { "Remaining:     rate too flat\n", "残り時間:     変化が小さすぎます\n" },
    { "Rename", "名前を変更" },
    { "Repeat delay: %lu ms", "長押し開始: %lu ms" },
    { "Repeat interval: %lu ms", "連続移動間隔: %lu ms" },
    { "Requested: %s", "設定: %s" },
    { "Restore bottom screen color curve", "下画面の色調を標準に戻す" },
    { "Restore top screen color curve", "上画面の色調を標準に戻す" },
    { "Return To HOME Menu", "HOMEメニューに戻る" },
    { "Return to HOME Menu", "HOMEメニューに戻る" },
    { "Right/Left:  +- 1\n", "十字左右: +-1\n" },
    { "Risk : %u%% (%s)", "危険度: %u%% (%s)" },
    { "Rosalina -- Debug info", "Rosalina -- デバッグ情報" },
    { "Rosalina -- Luma3DS credits", "Rosalina -- クレジット" },
    { "Rosalina -- System info", "Rosalina -- 本体情報" },
    { "Rosalina menu", "Rosalinaメニュー" },
    { "Running.\n\n", "動作中。\n\n" },
    { "SD free: %u MB", "SD空き: %u MB" },
    { "SD free: N/A", "SD空き: 不明" },
    { "SELECT to dump memory, START to toggle ASCII view.", "SELECT: 出力 START: ASCII表示切替" },
    { "SELECT: delete  START: rename  L: new dir", "SELECT: 削除 START: 改名 L: 新規フォルダ" },
    { "SELECT: next   A: save   B: cancel", "SELECT: 対象切替  A: 保存  B: 取消" },
    { "SELECT:target  Arrows:hue/val  Y:sat  X:preset", "SELECT: 対象 十字: 色/明度 Y: 彩度 X: 見本" },
    { "SS3D disabled or camera in use.\nPress B to exit this menu.\n", "3Dブレ防止が無効、またはカメラ使用中です。\nB: 戻る\n" },
    { "START: confirm   B: cancel", "START: 決定   B: 取消" },
    { "Samples held:  %lu / %lu\n", "記録数:       %lu / %lu\n" },
    { "Save settings", "設定を保存" },
    { "Saved /luma/battery.csv", "保存先: /luma/battery.csv" },
    { "Screen brightness", "画面の明るさ" },
    { "Screen filters menu", "画面フィルター" },
    { "Screen filters...", "画面フィルター..." },
    { "Screenshot", "スクリーンショット" },
    { "Sending directory list", "一覧を送信中" },
    { "Sending file", "ファイルを送信中" },
    {"Sensitive paths found in this plugin.", "このプラグインに重要なパスがあります。"},
    { "Serves the SD card over FTP so you\n", "SDカードをFTPで共有し、\n" },
    { "Sessions:  %lu\n", "接続数: %lu\n" },
    { "Set clock rate to %luMHz", "クロック周波数を%luMHzに変更" },
    { "Set clock rate to %uMHz", "クロック周波数を%uMHzに変更" },
    { "Shell:         %s\n", "本体の開閉:   %s\n" },
    { "Since reset:   %s, %s %%\n", "リセット後:   %s, %s %%\n" },
    { "Source and destination are the same.", "元の場所と貼り付け先が同じです。" },
    { "Start InputRedirection", "入力リダイレクトを開始" },
    { "Starting InputRedirection...", "入力リダイレクトを開始中..." },
    { "Starting InputRedirection... OK.", "入力リダイレクトを開始しました。" },
    { "Starting debugger...", "デバッガーを起動中..." },
    { "Starting debugger... OK.", "デバッガーを起動しました。" },
    { "Starting...", "開始中..." },
    { "State: %s", "接続状態: %s" },
    { "Stop InputRedirection", "入力リダイレクトを停止" },
    { "Stopped.", "停止中" },
    { "Stopped.\n\n", "停止中。\n\n" },
    { "Stopping...", "停止処理中..." },
    { "Streaming", "配信中" },
    { "Successfully changed the menu combo.", "メニューの操作キーを変更しました。" },
    { "Super-Stable 3D calibration", "3Dブレ防止の補正" },
    {"Swap file size changed.", "スワップのサイズが変更されています。"},
    {"Swap file was changed or corrupted.", "スワップが変更されたか、破損しています。"},
    { "Switch the hb. title to hblauncher_loader", "HB起動先をhblauncher_loaderに戻す" },
    { "Switch the hb. title to the current app.", "現在のアプリをHB起動先に設定" },
    { "Switch version (v9.1/v13.4Y)", "バージョン切替 (v9.1/v13.4Y)" },
    { "System configuration menu", "本体設定" },
    { "System configuration...", "本体設定..." },
    { "System info", "本体情報" },
    { "Take screenshot", "スクリーンショット撮影" },
    { "Take screenshot (slow!)", "スクリーンショット撮影 (低速)" },
    { "Temperature: %12dK    \n", "色温度: %12dK    \n" },
    { "Temporarily disable Super-Stable 3D", "3Dブレ防止を一時的に無効化" },
    { "Test parallax barrier positions", "視差バリアの位置をテスト" },
    { "The HOME button is disabled\ndue to insufficient memory.", "メモリ不足のため\nHOMEボタンは無効です。" },
    { "The current menu combo is:  %s", "現在の操作キー: %s" },
    { "This 3gx could not be fully checked", "この3gxは最後まで検査できません" },
    { "This 3gx may be a destructive plugin", "この3gxはデータを破壊する可能性があります" },
    { "This phase:    %s, %s %%\n", "現在の区間:   %s, %s %%\n" },
    { "This plugin file is corrupted.", "プラグインファイルが破損しています。" },
    { "This plugin requires a loading function.", "このプラグインには読み込み関数が必要です。" },
    { "Time spent converting:    %5lums\n", "変換時間:     %5lums\n" },
    { "Time spent writing files: %5lums\n", "書き込み時間: %5lums\n" },
    { "Time/date updated successfully.", "日時を更新しました。" },
    { "Title ID: %016llX", "タイトルID: %016llX" },
    { "Toggle LEDs", "LEDを切り替え" },
    { "Toggle Power Button", "電源ボタンを切り替え" },
    { "Toggle Wireless", "無線を切り替え" },
    { "Toggle power to card slot", "カードスロットの電源を切り替え" },
    { "Top screen type:    %s\n", "上画面の種類:  %s\n" },
    { "Top screen:\n", "上画面:\n" },
    { "Touch bar / box / swatches to pick", "バー・四角・色見本をタッチして選択" },
    { "Transferred: %lu KiB", "転送量: %lu KiB" },
    { "Transfers: %lu\n\n", "転送数: %lu\n\n" },
    { "Transitions: %s", "画面切替のフェード: %s" },
    { "Unable to load image.\nPNG / BMP / baseline JPEG, up to 1024 x 1024.", "画像を読み込めません。\nPNG・BMP・標準JPEG、1024×1024以下に対応。" },
    { "Unable to save settings.", "設定を保存できませんでした。" },
    { "Unable to start FTP.", "FTPを開始できませんでした。" },
    {"Unsupported loading payload or corrupted plugin.", "未対応の読込処理か、ファイルが破損しています。"},
    {"Unsupported swap load payload or corrupted data.", "未対応のスワップ読込処理か、データが破損しています。"},
    {"Unsupported swap save payload.", "スワップ保存処理に対応していません。"},
    { "Until 10%%:     %s\n", "10%%まで:      %s\n" },
    { "Until empty:   %s\n", "空になるまで: %s\n" },
    { "Until full:    %s\n", "満充電まで:   %s\n" },
    { "Up/Down:     +- 0.1\n", "十字上下: +-0.1\n" },
    { "Up/Down: move  A: select  B: back  L/R: page", "上下: 移動 A: 決定 B: 戻る L/R: ページ" },
    { "Up/Down: select   A: save", "上下: 選択   A: 保存" },
    { "Up/Down: select  Left/Right: change", "十字上下: 項目選択  左右: 変更" },
    { "Update both screens: %s (L to toggle)   \n", "両画面を更新: %s (L: 切替)\n" },
    { "Update time and date via NTP", "NTPで日時を更新" },
    { "Use DPAD Left/Right to change hour offset.\nUse DPAD Up/Down to change minute offset.\nPress A when done.", "十字左右: 時間を変更\n十字上下: 分を変更\nA: 決定" },
    { "Use left/right to adjust the barrier's position.\n\n", "十字左右: 視差バリアの位置を調整\n\n" },
    { "Use left/right to increase/decrease the sel. value.\n", "十字左右: 選択した値を増減\n" },
    { "Version switch chainload", "バージョン切り替え" },
    { "Viewer IP: %s", "PCのIP: %s" },
    { "Voltage range: %lu.%03lu - %lu.%03lu V\n", "電圧範囲:     %lu.%03lu - %lu.%03lu V\n" },
    { "Voltage:       %lu.%03lu V\n", "電圧:         %lu.%03lu V\n" },
    { "WARNING:", "警告:" },
    { "WARNING: \n", "警告:\n" },
    { "Waiting for Wi-Fi.", "Wi-Fiの接続待ちです。" },
    { "Waiting for connection", "接続待ち" },
    { "Waiting for viewer", "受信PCの接続待ち" },
    { "Warning", "警告" },
    { "Wi-Fi must be connected first.\n", "先にWi-Fiに接続してください。\n" },
    { "Wireless streaming", "無線映像転送" },
    { "Wireless streaming...", "無線映像転送..." },
    { "Written: %lu KB   ", "書き込み済み: %lu KB   " },
    { "Wrote /luma/%016llX.txt", "保存先: /luma/%016llX.txt" },
    { "X: back to the warning", "X: 警告に戻る" },
    { "X: defaults  A: save  B: cancel", "X: 初期値  A: 保存  B: 取消" },
    { "X: show where found", "X: 見つかった場所を表示" },
    { "Y:           reload last saved calibration\n", "Y: 保存済みの補正値を再読込\n" },
    { "Y: Toggle volume slider override.\nDPAD/CPAD: Adjust the volume level.\nA: Apply\nB: Go back\n\n", "Y: 音量スライダーの上書きを切替\n十字/スライドパッド: 音量を調整\nA: 適用\nB: 戻る\n\n" },
    { "Y: refresh    B: back", "Y: 更新   B: 戻る" },
    { "[10000K] Aquarium", "[10000K] 水槽" },
    { "[1200K] Ember", "[1200K] 炭火" },
    { "[1900K] Candle", "[1900K] ろうそく" },
    { "[2300K] Warm Incandescent", "[2300K] 暖かい白熱灯" },
    { "[2700K] Incandescent", "[2700K] 白熱灯" },
    { "[3400K] Halogen", "[3400K] ハロゲン灯" },
    { "[4200K] Fluorescent", "[4200K] 蛍光灯" },
    { "[5500K] Daylight", "[5500K] 昼光" },
    { "[6500K] Default temperature", "[6500K] 標準" },
    { "[7500K] Overcast Sky", "[7500K] 曇り空" },
    { "[IPS recommended] Enhance bottom screen colors", "[IPS向け] 下画面の発色を強める" },
    { "[IPS recommended] Enhance top screen colors", "[IPS向け] 上画面の発色を強める" },
    { "[boot.firm for v%s was not found]", "[v%sのboot.firmが見つかりません]" },
    { "[copy] ", "[コピー] " },
    { "[cut] ", "[切取] " },
    { "accent", "強調色" },
    { "bg", "背景" },
    { "can manage files from a PC.\n\n", "PCからファイルを操作できます。\n\n" },
    { "closed", "閉" },
    { "cursor", "カーソル" },
    { "editing: %s    ", "編集対象: %s    " },
    { "exit then press R+RIGHT to toggle the WiFi.", "終了後にR + 十字右で無線を切り替えてください。" },
    { "false", "無効" },
    { "found", "あり" },
    { "little dangerous", "低" },
    { "map.txt lists every region.", "全領域の一覧: map.txt" },
    { "mcu::HWC is not reachable", "mcu::HWCに接続できません" },
    { "moderately dangerous", "中" },
    { "no", "いいえ" },
    { "open", "開" },
    { "plugged in", "接続" },
    { "svcOpenProcess failed", "プロセスを開けませんでした" },
    { "text", "文字" },
    { "true", "有効" },
    { "unknown", "不明" },
    { "unplugged", "未接続" },
    { "very dangerous", "高" },
    { "yes", "はい" },
};

const char *Lang_Translate(const char *text)
{
    if (!g_langLoaded)
        Lang_Load();
    if (g_language != LANG_JP || text == NULL)
        return text;
    u32 first = 0, last = sizeof(g_labels) / sizeof(g_labels[0]);
    while (first < last)
    {
        u32 middle = first + (last - first) / 2;
        int order = strcmp(text, g_labels[middle].english);
        if (order == 0)
            return g_labels[middle].japanese;
        if (order < 0)
            last = middle;
        else
            first = middle + 1;
    }
    return text;
}

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
    if (R_SUCCEEDED(IFile_Read(&file, &total, buf, 2)) && total >= 1)
    {
        if (buf[0] == '0') g_language = LANG_EN;
        if (buf[0] == '1') g_language = LANG_JP;
    }

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
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Language / 言語"));

        u32 posY = 30;

        posY = Draw_DrawFormattedString(10, posY, g_language == LANG_EN ? COLOR_LIME : COLOR_WHITE,
                                        "%c English\n", g_language == LANG_EN ? '>' : ' ');
        posY = Draw_DrawFormattedString(10, posY, g_language == LANG_JP ? COLOR_LIME : COLOR_WHITE,
                                        "%c 日本語\n", g_language == LANG_JP ? '>' : ' ');


        Draw_DrawString(10, 214, COLOR_WHITE, Lang_Translate("Up/Down: select   A: save"));
        Draw_DrawString(10, 225, COLOR_WHITE, Lang_Translate("B: back"));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 pressed = waitInputWithTimeout(1000);

        if (menuShouldExit || (pressed & KEY_B))
            break;

        if (pressed & (KEY_UP | KEY_DOWN))
            g_language = (g_language == LANG_EN) ? LANG_JP : LANG_EN;

        if (pressed & KEY_A)
        {
            Lang_Save();
            return;
        }
    }
    while (!menuShouldExit);

    g_language = previous;
}
