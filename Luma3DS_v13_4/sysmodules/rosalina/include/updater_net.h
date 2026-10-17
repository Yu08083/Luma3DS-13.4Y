#pragma once
#include "updater_http.h"
typedef struct UpdateNetDiagnostics {
    int tlsError;
    uint32_t verifyFlags,serviceResult;
    unsigned httpStatus;
    int64_t utc;
    bool clockAdjusted;
} UpdateNetDiagnostics;
int UpdateNet_Get(const char *,uint32_t,UpdateHttpSink,void *,const UpdateHooks *);
void UpdateNet_Diagnostics(UpdateNetDiagnostics *);
