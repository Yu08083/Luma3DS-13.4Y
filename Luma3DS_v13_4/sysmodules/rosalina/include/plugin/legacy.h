#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PLUGIN_LEGACY_MAGIC UINT64_C(0x3130303024584733)
#define PLUGIN_LEGACY_HEADER_SIZE 0x3Cu

bool PluginLegacy_ValidateHeader(const void *header, uint32_t fileSize, uint32_t memorySize);
bool PluginLegacy_Validate(const void *file, uint32_t fileSize, uint32_t memorySize);
bool PluginLegacy_Convert(void *file, uint32_t fileSize, uint32_t memorySize, uint32_t *convertedSize);
