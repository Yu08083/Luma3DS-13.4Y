#include "ui.h"
#include <stdio.h>
#include <string.h>
#include "../../Luma3DS_v13_4/sysmodules/rosalina/include/font.h"
#include "../../Luma3DS_v13_4/sysmodules/rosalina/include/japanese_font.h"

enum {
    UI_KEY_A = 1u,
    UI_KEY_B = 2u,
    UI_KEY_START = 8u,
    UI_KEY_X = 1u << 10,
    UI_PURPLE = 0xCB9CFFu,
    UI_WHITE = 0xEAEAF0u,
    UI_GRAY = 0xA5A5B5u,
    UI_RED = 0xFF9EA8u
};

static bool valid_surface(const UpdaterUiSurface *surface)
{
    return surface && surface->pixels && surface->width > 0 && surface->width <= 400 &&
           surface->height > 0 && surface->height <= 240 &&
           surface->size >= (size_t)surface->width * surface->height * 3u;
}

static void pixel(const UpdaterUiSurface *surface, int x, int y, uint32_t color)
{
    if (x < 0 || y < 0 || x >= surface->width || y >= surface->height)
        return;
    size_t offset = ((size_t)x * surface->height + surface->height - 1u - (unsigned)y) * 3u;
    surface->pixels[offset] = (uint8_t)color;
    surface->pixels[offset + 1] = (uint8_t)(color >> 8);
    surface->pixels[offset + 2] = (uint8_t)(color >> 16);
}

static void rectangle(const UpdaterUiSurface *surface, int x, int y, int width, int height, uint32_t color)
{
    for (int column = 0; column < width; ++column)
        for (int row = 0; row < height; ++row)
            pixel(surface, x + column, y + row, color);
}

static uint32_t decode(const unsigned char *text, size_t length, size_t *used)
{
    uint32_t first = text[0];
    *used = 1;
    if (first < 0x80)
        return first;
    unsigned count = first >= 0xC2 && first <= 0xDF ? 2 :
                     first >= 0xE0 && first <= 0xEF ? 3 :
                     first >= 0xF0 && first <= 0xF4 ? 4 : 0;
    if (!count || length < count)
        return 0xFFFD;
    uint32_t value = first & ((1u << (7u - count)) - 1u);
    for (unsigned i = 1; i < count; ++i)
    {
        if ((text[i] & 0xC0u) != 0x80u)
            return 0xFFFD;
        value = (value << 6) | (text[i] & 0x3Fu);
    }
    if ((count == 3 && value < 0x800) || (count == 4 && value < 0x10000) ||
        value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
        return 0xFFFD;
    *used = count;
    return value;
}

static const uint8_t *glyph(uint32_t codepoint)
{
    size_t low = 0;
    size_t high = sizeof(japaneseCodepoints) / sizeof(japaneseCodepoints[0]);
    while (low < high)
    {
        size_t middle = low + (high - low) / 2;
        if (japaneseCodepoints[middle] < codepoint)
            low = middle + 1;
        else
            high = middle;
    }
    if (low < sizeof(japaneseCodepoints) / sizeof(japaneseCodepoints[0]) && japaneseCodepoints[low] == codepoint)
        return japaneseGlyphs[low];
    static const uint8_t missing[8] = {0xFE, 0x82, 0x82, 0x82, 0x82, 0x82, 0xFE, 0};
    return missing;
}

static void text(const UpdaterUiSurface *surface, int x, int y, uint32_t color,
                 const char *value, size_t limit, unsigned scale)
{
    size_t offset = 0;
    while (offset < limit && value[offset])
    {
        size_t used;
        uint32_t codepoint = decode((const unsigned char *)value + offset, limit - offset, &used);
        offset += used;
        if (codepoint < 0x20 || codepoint == 0x7F)
            codepoint = ' ';
        unsigned width = codepoint < 0x80 ? 6 : 8;
        if (x + (int)(width * scale) > surface->width - 12)
            break;
        const uint8_t *rows = codepoint < 0x80 ? font + codepoint * 10 : glyph(codepoint);
        for (unsigned row = 0; row < 10; ++row)
        {
            unsigned bits = codepoint < 0x80 ? rows[row] : row >= 1 && row <= 8 ? rows[row - 1] : 0;
            for (unsigned column = 0; column < width; ++column)
            {
                unsigned mask = codepoint < 0x80 ? 1u << (6u - column) : 0x80u >> column;
                if (bits & mask)
                    rectangle(surface, x + (int)(column * scale), y + (int)(row * scale), scale, scale, color);
            }
        }
        x += (int)(width * scale);
    }
}

static void label(const UpdaterUiSurface *surface, int x, int y, uint32_t color,
                  const char *value, unsigned scale)
{
    text(surface, x, y, color, value, strlen(value), scale);
}

static const char *local(const UpdaterUi *ui, const char *english, const char *japanese)
{
    return ui->japanese ? japanese : english;
}

static bool critical(const UpdaterAppView *view)
{
    return view->busy && (view->stage == UPDATE_STAGE_INSTALL || view->stage == UPDATE_STAGE_ROLLBACK);
}

static void synchronize(UpdaterUi *ui, const UpdaterAppView *view)
{
    if (ui->confirming && (view->busy || !view->available ||
        memcmp(ui->confirm_version, view->version, sizeof(ui->confirm_version))))
        ui->confirming = false;
    if (view->busy != ui->last_busy)
    {
        ui->cancel_requested = false;
        ui->dismissed = false;
        ui->last_busy = view->busy;
    }
}

void UpdaterUi_Init(UpdaterUi *ui, bool japanese)
{
    if (!ui)
        return;
    memset(ui, 0, sizeof(*ui));
    ui->japanese = japanese;
}

void UpdaterUi_Invalidate(UpdaterUi *ui)
{
    if (ui)
        memset(ui->frames, 0, sizeof(ui->frames));
}

UpdaterUiAction UpdaterUi_Input(UpdaterUi *ui, const UpdaterAppView *view, uint32_t keys_down)
{
    if (!ui || !view)
        return UPDATER_UI_NONE;
    synchronize(ui, view);
    if (keys_down & UI_KEY_START)
    {
        ui->confirming = false;
        return UPDATER_UI_EXIT;
    }
    if (keys_down & UI_KEY_X)
    {
        ui->japanese = !ui->japanese;
        return UPDATER_UI_NONE;
    }
    if (keys_down & UI_KEY_B)
    {
        if (view->busy)
        {
            if (!critical(view) && !ui->cancel_requested && !view->cancelling)
            {
                ui->cancel_requested = true;
                return UPDATER_UI_CANCEL;
            }
        }
        else if (ui->confirming)
            ui->confirming = false;
        else if (view->stage == UPDATE_STAGE_DONE && view->result < 0)
            ui->dismissed = true;
        return UPDATER_UI_NONE;
    }
    if (!(keys_down & UI_KEY_A) || view->busy)
        return UPDATER_UI_NONE;
    if (view->available && !ui->dismissed)
    {
        if (!ui->confirming)
        {
            memcpy(ui->confirm_version, view->version, sizeof(ui->confirm_version));
            ui->confirming = true;
            return UPDATER_UI_NONE;
        }
        ui->confirming = false;
        return UPDATER_UI_INSTALL;
    }
    ui->dismissed = false;
    return UPDATER_UI_CHECK;
}

static const char *status(const UpdaterUi *ui, const UpdaterAppView *view)
{
    if (view->busy)
    {
        if ((ui->cancel_requested || view->cancelling) && !critical(view))
            return local(ui, "Cancelling...", "中止しています...");
        switch (view->stage)
        {
            case UPDATE_STAGE_DOWNLOAD: return local(ui, "Downloading...", "ダウンロード中...");
            case UPDATE_STAGE_VERIFY: return local(ui, "Verifying...", "確認中...");
            case UPDATE_STAGE_BACKUP: return local(ui, "Backing up...", "バックアップ中...");
            case UPDATE_STAGE_INSTALL: return local(ui, "Installing...", "更新中...");
            case UPDATE_STAGE_ROLLBACK: return local(ui, "Restoring...", "復元中...");
            case UPDATE_STAGE_CLOCK: return local(ui, "Checking network time...", "日時を補正中...");
            default: return local(ui, "Checking for updates...", "更新を確認中...");
        }
    }
    if (ui->confirming)
        return local(ui, "Install this update?", "更新しますか？");
    if (ui->dismissed || view->stage != UPDATE_STAGE_DONE)
        return local(ui, "Check for updates", "更新を確認");
    switch (view->result)
    {
        case UPDATE_CURRENT: return local(ui, "Up to date", "最新版です");
        case UPDATE_OK: return view->installing ? local(ui, "Update complete", "更新しました") :
                                                  local(ui, "Update available", "更新があります");
        case UPDATE_CANCELLED: return local(ui, "Cancelled", "中止しました");
        case UPDATE_ERR_NETWORK: return local(ui, "Connection failed", "接続できませんでした");
        case UPDATE_ERR_HTTP: return local(ui, "Download failed", "ダウンロードに失敗しました");
        case UPDATE_ERR_URL: return local(ui, "Invalid update address", "更新先が正しくありません");
        case UPDATE_ERR_METADATA: return local(ui, "Invalid update info", "更新情報が正しくありません");
        case UPDATE_ERR_SIZE: return local(ui, "Invalid download size", "ファイルサイズが違います");
        case UPDATE_ERR_HASH: return local(ui, "File verification failed", "ファイルの確認に失敗しました");
        case UPDATE_ERR_FIRM: return local(ui, "Invalid update file", "更新ファイルが正しくありません");
        case UPDATE_ERR_SD: return local(ui, "SD card error", "SDカードを読み書きできません");
        case UPDATE_ERR_BACKUP: return local(ui, "Backup failed", "バックアップに失敗しました");
        case UPDATE_ERR_ROLLBACK: return local(ui, "Restore failed", "復元に失敗しました");
        case UPDATE_ERR_TLS: return local(ui, "Secure connection failed", "安全な接続を確立できません");
        case UPDATE_ERR_BUSY: return local(ui, "Update is busy", "更新処理を開始できません");
        case UPDATE_ERR_CLOCK: return local(ui, "Network time unavailable", "日時を補正できませんでした");
        default: return local(ui, "Update failed", "更新に失敗しました");
    }
}

static uint64_t signature(const UpdaterUi *ui, const UpdaterAppView *view)
{
    uint64_t value = UINT64_C(14695981039346656037);
    uint32_t fields[] = {ui->japanese, ui->confirming, ui->cancel_requested, ui->dismissed,
                         view->busy, view->available, view->installing, view->cancelling, (uint32_t)view->result,
                         view->stage, view->done, view->total, (uint32_t)view->network.tlsError,
                         view->network.verifyFlags, view->network.serviceResult, view->network.httpStatus};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
    {
        value ^= fields[i];
        value *= UINT64_C(1099511628211);
    }
    for (size_t i = 0; i < sizeof(view->version); ++i)
    {
        value ^= (unsigned char)view->version[i];
        value *= UINT64_C(1099511628211);
        if (!view->version[i])
            break;
    }
    return value;
}

static bool changed(UpdaterUi *ui, const UpdaterUiSurface *surface, unsigned first, uint64_t value)
{
    UpdaterUiFrame *slot = &ui->frames[first];
    for (unsigned i = first; i < first + 2; ++i)
    {
        UpdaterUiFrame *frame = &ui->frames[i];
        if (!frame->valid)
            slot = frame;
        else if (frame->surface.pixels == surface->pixels)
        {
            if (frame->surface.size == surface->size && frame->surface.width == surface->width &&
                frame->surface.height == surface->height && frame->signature == value)
                return false;
            slot = frame;
            break;
        }
    }
    slot->surface = *surface;
    slot->signature = value;
    slot->valid = true;
    return true;
}

static void draw_top(const UpdaterUi *ui, const UpdaterAppView *view, const UpdaterUiSurface *surface)
{
    memset(surface->pixels, 0, (size_t)surface->width * surface->height * 3u);
    label(surface, 24, 24, UI_PURPLE, "Luma3DS 13.4Y Updater", 2);
    if (view->version[0])
    {
        label(surface, 24, 88, UI_GRAY, local(ui, "Latest", "最新バージョン"), 2);
        text(surface, 24, 120, UI_WHITE, view->version, sizeof(view->version), 2);
    }
    label(surface, 24, 188, UI_GRAY, "v1.0.1", 1);
    label(surface, 24, 208, UI_GRAY, "@yu_", 1);
}

static void draw_bottom(const UpdaterUi *ui, const UpdaterAppView *view, const UpdaterUiSurface *surface)
{
    memset(surface->pixels, 0, (size_t)surface->width * surface->height * 3u);
    bool error = !view->busy && !ui->dismissed && view->stage == UPDATE_STAGE_DONE && view->result < UPDATE_CANCELLED;
    label(surface, 16, 24, error ? UI_RED : UI_PURPLE, status(ui, view), 2);
    if (error)
    {
        char code[24];
        snprintf(code, sizeof(code), "%s: %d", local(ui, "Error", "エラー"), (int)view->result);
        label(surface, 16, 60, UI_GRAY, code, 1);
        char detail[64];
        if(view->network.httpStatus)
        {
            snprintf(detail,sizeof(detail),"HTTP: %u",view->network.httpStatus);
            label(surface,16,80,UI_GRAY,detail,1);
        }
        if(view->network.tlsError||view->network.verifyFlags||view->network.serviceResult)
        {
            snprintf(detail,sizeof(detail),"TLS: %08lX  Cert: %08lX",(unsigned long)(uint32_t)view->network.tlsError,(unsigned long)view->network.verifyFlags);
            label(surface,16,100,UI_GRAY,detail,1);
            if(view->network.serviceResult)
            {
                snprintf(detail,sizeof(detail),"Service: %08lX",(unsigned long)view->network.serviceResult);
                label(surface,16,120,UI_GRAY,detail,1);
            }
        }
        if(view->result==UPDATE_ERR_CLOCK)
            label(surface,16,144,UI_GRAY,local(ui,"Check the system date and retry","本体の日付を確認して再試行"),1);
    }
    if (view->busy && view->total)
    {
        uint64_t done = view->done < view->total ? view->done : view->total;
        unsigned percent = (unsigned)(done * 100u / view->total);
        unsigned width = surface->width > 32 ? surface->width - 32u : 0;
        rectangle(surface, 16, 102, width, 6, 0x252530);
        rectangle(surface, 16, 102, (int)(done * width / view->total), 6, UI_PURPLE);
        char progress[16];
        snprintf(progress, sizeof(progress), "%u%%", percent);
        label(surface, 16, 124, UI_WHITE, progress, 2);
    }
    if (!view->busy)
    {
        const char *action = ui->confirming ? local(ui, "A: Update", "A: 更新") :
                             view->available && !ui->dismissed ? local(ui, "A: Continue", "A: 確認") :
                                                                 local(ui, "A: Check", "A: 確認");
        label(surface, 16, 174, UI_WHITE, action, 2);
        if (ui->confirming || error)
            label(surface, 16, 198, UI_GRAY, local(ui, "B: Back", "B: 戻る"), 1);
    }
    else if (!critical(view) && !ui->cancel_requested && !view->cancelling)
        label(surface, 16, 174, UI_WHITE, local(ui, "B: Cancel", "B: 中止"), 2);
    label(surface, 16, 222, UI_GRAY, local(ui, "X: 日本語", "X: English"), 1);
    label(surface, surface->width > 100 ? surface->width - 100 : 0, 222, UI_GRAY,
          local(ui, "START: Exit", "START: 終了"), 1);
}

void UpdaterUi_Draw(UpdaterUi *ui, const UpdaterAppView *view,
                    const UpdaterUiSurface *top, const UpdaterUiSurface *bottom)
{
    if (!ui || !view)
        return;
    synchronize(ui, view);
    uint64_t value = signature(ui, view);
    if (valid_surface(top) && changed(ui, top, 0, value))
        draw_top(ui, view, top);
    if (valid_surface(bottom) && changed(ui, bottom, 2, value))
        draw_bottom(ui, view, bottom);
}
