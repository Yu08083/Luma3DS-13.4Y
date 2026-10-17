#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct UpdateStorageIo {
    void *context;
    bool (*begin)(void *);
    void (*end)(void *);
    int (*exists)(void *,const char *,uint64_t *);
    bool (*open)(void *,const char *,bool,uint32_t *);
    bool (*read)(void *,uint32_t,uint64_t,void *,uint32_t,uint32_t *);
    bool (*write)(void *,uint32_t,uint64_t,const void *,uint32_t,uint32_t *);
    bool (*flush)(void *,uint32_t);
    bool (*close)(void *,uint32_t);
    bool (*rename)(void *,const char *,const char *);
    bool (*remove)(void *,const char *);
} UpdateStorageIo;
#ifdef UPDATE_STORAGE_TEST
void UpdateStorage_TestIo(const UpdateStorageIo *);
#endif
