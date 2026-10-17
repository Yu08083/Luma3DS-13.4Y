#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "app.h"

typedef enum UpdaterUiAction {
    UPDATER_UI_NONE,
    UPDATER_UI_CHECK,
    UPDATER_UI_INSTALL,
    UPDATER_UI_CANCEL,
    UPDATER_UI_EXIT
} UpdaterUiAction;

typedef struct UpdaterUiSurface {
    uint8_t *pixels;
    size_t size;
    uint16_t width;
    uint16_t height;
} UpdaterUiSurface;

typedef struct UpdaterUiFrame {
    UpdaterUiSurface surface;
    uint64_t signature;
    bool valid;
} UpdaterUiFrame;

typedef struct UpdaterUi {
    bool japanese;
    bool confirming;
    bool cancel_requested;
    bool dismissed;
    bool last_busy;
    char confirm_version[64];
    UpdaterUiFrame frames[4];
} UpdaterUi;

void UpdaterUi_Init(UpdaterUi *ui, bool japanese);
void UpdaterUi_Invalidate(UpdaterUi *ui);
UpdaterUiAction UpdaterUi_Input(UpdaterUi *ui, const UpdaterAppView *view, uint32_t keys_down);
void UpdaterUi_Draw(UpdaterUi *ui, const UpdaterAppView *view,
                    const UpdaterUiSurface *top, const UpdaterUiSurface *bottom);
