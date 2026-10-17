#pragma once
#include "updater.h"

typedef struct UpdaterAppView {
    bool busy,available,installing,cancelling;
    UpdateResult result;
    UpdateStage stage;
    uint32_t done,total;
    char version[UPDATE_MAX_TAG];
} UpdaterAppView;

void UpdaterApp_Init(void);
bool UpdaterApp_RequestCheck(void);
bool UpdaterApp_RequestInstall(void);
void UpdaterApp_Cancel(void);
void UpdaterApp_Snapshot(UpdaterAppView *);
void UpdaterApp_Shutdown(void);
