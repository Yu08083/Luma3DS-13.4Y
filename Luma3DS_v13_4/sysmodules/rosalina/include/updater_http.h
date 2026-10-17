#pragma once
#include "updater.h"

typedef struct UpdateHttpIo {
    void *context;
    int (*read)(void *,void *,size_t);
} UpdateHttpIo;

typedef struct UpdateHttpResponse {
    unsigned status;
    uint32_t size;
    char location[UPDATE_MAX_URL];
} UpdateHttpResponse;

typedef int (*UpdateHttpSink)(void *,const void *,uint32_t);

int UpdateHttp_Read(const UpdateHttpIo *io,uint32_t limit,UpdateHttpSink sink,void *context,UpdateHttpResponse *response);
