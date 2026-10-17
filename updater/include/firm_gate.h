#pragma once
#include "updater.h"

typedef bool (*UpdaterFirmRead)(void *,uint32_t,void *,uint32_t);
UpdateResult UpdaterFirm_Gate(UpdaterFirmRead,void *,uint32_t);
