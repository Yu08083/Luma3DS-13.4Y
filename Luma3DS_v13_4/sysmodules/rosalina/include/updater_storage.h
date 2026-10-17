#pragma once
#include "updater.h"
int UpdateStorage_CurrentMatches(const UpdateManifest *,const UpdateHooks *);
UpdateResult UpdateStorage_Begin(const UpdateManifest *,const UpdateHooks *);
UpdateResult UpdateStorage_Write(const void *,uint32_t);
UpdateResult UpdateStorage_Install(void);
void UpdateStorage_Close(void);
