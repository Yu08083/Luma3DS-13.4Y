#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t imageSize;
    uint32_t executableSize;
    uint32_t snapshotSize;
} PluginFileLayout;

bool PluginFile_Validate(const void *header, uint32_t fileSize, uint32_t memorySize, uint32_t pluginHeaderSize, PluginFileLayout *layout);
bool PluginFile_ValidateStrings(const void *file, uint32_t fileSize);
