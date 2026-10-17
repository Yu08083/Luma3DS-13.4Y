#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ONLINE_ID_PATH "/luma/level256_id.bin"

typedef struct {
    uint32_t magic;
    uint32_t generation;
    uint64_t value;
    uint32_t checksum;
    uint32_t reserved;
} OnlineIdRecord;

static inline uint32_t OnlineId_Checksum(const OnlineIdRecord *record)
{
    const uint8_t *bytes = (const uint8_t *)record;
    uint32_t hash = 2166136261u;
    for(size_t i = 0; i < 16; ++i) hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
}

static inline bool OnlineId_Valid(const OnlineIdRecord *record)
{
    return record->magic == 0x3159324Cu && record->value && !record->reserved && record->checksum == OnlineId_Checksum(record);
}

static inline int OnlineId_Select(const OnlineIdRecord records[2])
{
    bool a = OnlineId_Valid(&records[0]), b = OnlineId_Valid(&records[1]);
    if(!a) return b ? 1 : -1;
    if(!b) return 0;
    return (int32_t)(records[1].generation - records[0].generation) > 0 ? 1 : 0;
}
