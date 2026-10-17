#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define THEME_IMAGE_WORK_SIZE 65536
#define THEME_IMAGE_MAX_DIMENSION 1024
#define THEME_IMAGE_MAX_FILE_SIZE (4 * 1024 * 1024)

typedef struct {
    void *context;
    uint32_t size;
    bool (*read)(void *context, uint32_t offset, void *buffer, uint32_t size);
    bool (*cancelled)(void *context);
} ThemeImageSource;

bool ThemeImage_Decode(const ThemeImageSource *source, uint16_t *pixels, void *work, size_t workSize);
void *ThemeImage_Calloc(size_t count, size_t size);
void ThemeImage_Free(void *pointer);
