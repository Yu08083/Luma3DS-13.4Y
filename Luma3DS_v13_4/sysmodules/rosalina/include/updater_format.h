#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define UPDATE_MAX_FIRM (4u*1024u*1024u)
#define UPDATE_MAX_JSON 65536u
#define UPDATE_MAX_MANIFEST 2048u
#define UPDATE_MAX_URL 2048u
#define UPDATE_MAX_TAG 64u
typedef struct UpdateManifest {
    char version[UPDATE_MAX_TAG];
    uint32_t size;
    uint8_t sha256[32];
} UpdateManifest;
typedef struct UpdateSection {
    uint32_t offset,address,size,method;
    uint8_t hash[32];
} UpdateSection;
bool Update_ParseRelease(const char *json,size_t length,char tag[UPDATE_MAX_TAG]);
bool Update_ParseManifest(const char *json,size_t length,const char *tag,UpdateManifest *manifest);
bool Update_TrustedUrl(const char *url);
bool Update_AssetUrl(char out[UPDATE_MAX_URL],const char *tag,const char *asset);
bool Update_FirmHeader(const uint8_t header[512],uint32_t fileSize,UpdateSection sections[4],uint32_t *count);
