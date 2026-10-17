#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint32_t code[32];
    uint32_t count;
} PluginPayload;

bool PluginPayload_Set(PluginPayload *program, const uint32_t code[32]);
bool PluginPayload_Run(const PluginPayload *program, void *buffer, uint32_t size, const uint32_t arguments[4], uint32_t *checksum);
