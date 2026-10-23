#include <string.h>
#include "theme_image.h"
#include "../../../../third_party/theme/pngle.h"
#include "../../../../third_party/theme/picojpeg.h"

typedef struct { uint32_t size, used; } ThemeBlock;
static uint8_t *imageArena;
static size_t imageArenaSize;
static uint8_t imageInput[4096];

void *ThemeImage_Calloc(size_t count, size_t size)
{
    if(!imageArena || !count || size > imageArenaSize / count) return NULL;
    size_t length = (count * size + 7) & ~(size_t)7;
    for(size_t offset = 0; offset + sizeof(ThemeBlock) < imageArenaSize;)
    {
        ThemeBlock *block = (ThemeBlock *)(imageArena + offset);
        if(!block->used && block->size >= length)
        {
            if(block->size >= length + 2 * sizeof(ThemeBlock))
            {
                ThemeBlock *next = (ThemeBlock *)(imageArena + offset + sizeof(ThemeBlock) + length);
                next->size = block->size - (uint32_t)length - sizeof(ThemeBlock);
                next->used = 0;
                block->size = (uint32_t)length;
            }
            block->used = 1;
            void *memory = block + 1;
            memset(memory, 0, length);
            return memory;
        }
        offset += sizeof(ThemeBlock) + block->size;
    }
    return NULL;
}

void ThemeImage_Free(void *pointer)
{
    if(!pointer || !imageArena) return;
    ((ThemeBlock *)pointer - 1)->used = 0;
    for(size_t offset = 0; offset + sizeof(ThemeBlock) < imageArenaSize;)
    {
        ThemeBlock *block = (ThemeBlock *)(imageArena + offset);
        size_t nextOffset = offset + sizeof(ThemeBlock) + block->size;
        if(nextOffset + sizeof(ThemeBlock) < imageArenaSize)
        {
            ThemeBlock *next = (ThemeBlock *)(imageArena + nextOffset);
            if(!block->used && !next->used)
            {
                block->size += sizeof(ThemeBlock) + next->size;
                continue;
            }
        }
        offset = nextOffset;
    }
}

typedef struct {
    const ThemeImageSource *source;
    uint16_t *pixels;
    uint32_t width, height, drawWidth, drawHeight, left, top, scaleX, scaleY;
    uint32_t position, cacheStart, cacheSize;
    bool failed, done;
} ThemeDecode;

static bool imageCancelled(ThemeDecode *image)
{
    return image->source->cancelled && image->source->cancelled(image->source->context);
}

static bool imageRead(ThemeDecode *image, uint32_t offset, void *buffer, uint32_t length)
{
    if(offset > image->source->size || length > image->source->size - offset || imageCancelled(image)) return false;
    return image->source->read(image->source->context, offset, buffer, length);
}

static uint32_t imageU32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint32_t imageBE32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
}

static bool imageSetSize(ThemeDecode *image, uint32_t width, uint32_t height)
{
    if(!width || !height || width > THEME_IMAGE_MAX_DIMENSION || height > THEME_IMAGE_MAX_DIMENSION) return false;
    image->width = width;
    image->height = height;
    image->scaleX = UINT32_MAX / width;
    image->scaleY = UINT32_MAX / height;
    if(width * 240 > height * 320)
    {
        image->drawWidth = 320;
        image->drawHeight = height * 320 / width;
    }
    else
    {
        image->drawHeight = 240;
        image->drawWidth = width * 240 / height;
    }
    if(!image->drawWidth) image->drawWidth = 1;
    if(!image->drawHeight) image->drawHeight = 1;
    image->left = (320 - image->drawWidth) / 2;
    image->top = (240 - image->drawHeight) / 2;
    return true;
}

static uint32_t imageScale(uint32_t value, uint32_t scale)
{
    return ((uint64_t)value * scale + UINT32_MAX) >> 32;
}

static uint32_t imageChannel(uint8_t value, uint8_t alpha)
{
    return (value * alpha + 1) * 257 >> 16;
}

static void imagePixel(ThemeDecode *image, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
    if(image->failed || x >= image->width || y >= image->height) return;
    if(width > image->width - x) width = image->width - x;
    if(height > image->height - y) height = image->height - y;
    uint32_t left = imageScale(x * image->drawWidth, image->scaleX);
    uint32_t right = imageScale((x + width) * image->drawWidth, image->scaleX);
    if(left == right) return;
    uint32_t top = imageScale(y * image->drawHeight, image->scaleY);
    uint32_t bottom = imageScale((y + height) * image->drawHeight, image->scaleY);
    if(top == bottom) return;
    uint16_t color = (uint16_t)((imageChannel(r, alpha) >> 3) << 11 | (imageChannel(g, alpha) >> 2) << 5 | imageChannel(b, alpha) >> 3);
    for(uint32_t dx = left; dx < right; ++dx)
        for(uint32_t dy = top; dy < bottom; ++dy)
            image->pixels[(image->left + dx) * 240 + 239 - image->top - dy] = color;
}

static void imagePngDraw(pngle_t *png, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4])
{
    ThemeDecode *image = pngle_get_user_data(png);
    imagePixel(image, x, y, w, h, rgba[0], rgba[1], rgba[2], rgba[3]);
}

static void imagePngDone(pngle_t *png)
{
    ThemeDecode *image = pngle_get_user_data(png);
    image->done = true;
}

static bool imagePng(ThemeDecode *image, const uint8_t *header)
{
    if(memcmp(header + 12, "IHDR", 4) || !imageSetSize(image, imageBE32(header + 16), imageBE32(header + 20))) return false;
    pngle_t *png = pngle_new();
    if(!png) return false;
    pngle_set_user_data(png, image);
    pngle_set_draw_callback(png, imagePngDraw);
    pngle_set_done_callback(png, imagePngDone);
    uint32_t offset = 0, remain = 0;
    bool valid = true;
    while(valid && !image->done)
    {
        uint32_t amount = sizeof(imageInput) - remain;
        if(amount > image->source->size - offset) amount = image->source->size - offset;
        if(!amount || !imageRead(image, offset, imageInput + remain, amount)) break;
        offset += amount;
        remain += amount;
        int fed = pngle_feed(png, imageInput, remain);
        if(fed < 0 || (uint32_t)fed > remain) { valid = false; break; }
        remain -= (uint32_t)fed;
        if(remain) memmove(imageInput, imageInput + fed, remain);
    }
    pngle_destroy(png);
    return valid && image->done && !imageCancelled(image);
}

static unsigned char imageJpegRead(unsigned char *buffer, unsigned char size, unsigned char *read, void *context)
{
    ThemeDecode *image = context;
    *read = 0;
    if(imageCancelled(image)) { image->failed = true; return PJPG_STREAM_READ_ERROR; }
    while(*read < size && image->position < image->source->size)
    {
        if(image->position < image->cacheStart || image->position >= image->cacheStart + image->cacheSize)
        {
            image->cacheStart = image->position;
            image->cacheSize = image->source->size - image->position;
            if(image->cacheSize > sizeof(imageInput)) image->cacheSize = sizeof(imageInput);
            if(!imageRead(image, image->cacheStart, imageInput, image->cacheSize)) { image->failed = true; return PJPG_STREAM_READ_ERROR; }
        }
        uint32_t amount = image->cacheStart + image->cacheSize - image->position;
        if(amount > (uint32_t)size - *read) amount = (uint32_t)size - *read;
        memcpy(buffer + *read, imageInput + image->position - image->cacheStart, amount);
        image->position += amount;
        *read += (unsigned char)amount;
    }
    return 0;
}

static bool imageJpeg(ThemeDecode *image)
{
    uint8_t end[2];
    if(!imageRead(image, image->source->size - 2, end, 2) || end[0] != 0xFF || end[1] != 0xD9) return false;
    pjpeg_image_info_t info;
    if(pjpeg_decode_init(&info, imageJpegRead, image, 0) || !imageSetSize(image, info.m_width, info.m_height)) return false;
    for(int my = 0; my < info.m_MCUSPerCol; ++my)
        for(int mx = 0; mx < info.m_MCUSPerRow; ++mx)
        {
            if(imageCancelled(image) || pjpeg_decode_mcu()) return false;
            for(int y = 0; y < info.m_MCUHeight; ++y)
                for(int x = 0; x < info.m_MCUWidth; ++x)
                {
                    uint32_t offset = (y & 8) * 16 + (x & 8) * 8 + (y & 7) * 8 + (x & 7);
                    uint8_t r = info.m_pMCUBufR[offset];
                    imagePixel(image, mx * info.m_MCUWidth + x, my * info.m_MCUHeight + y, 1, 1, r,
                               info.m_comps == 1 ? r : info.m_pMCUBufG[offset], info.m_comps == 1 ? r : info.m_pMCUBufB[offset], 255);
                }
        }
    return !image->failed;
}

static bool imageBmp(ThemeDecode *image, const uint8_t *header)
{
    uint32_t dib = imageU32(header + 14), data = imageU32(header + 10);
    int32_t width = (int32_t)imageU32(header + 18), height = (int32_t)imageU32(header + 22);
    uint32_t bits = (uint32_t)header[28] | (uint32_t)header[29] << 8;
    bool topDown = height < 0;
    if(dib < 40 || dib > image->source->size - 14 || data < 14 + dib || header[26] != 1 || header[27] != 0 || imageU32(header + 30) != 0 || (bits != 24 && bits != 32)) return false;
    if(width <= 0 || height == INT32_MIN) return false;
    if(topDown) height = -height;
    if(!imageSetSize(image, (uint32_t)width, (uint32_t)height)) return false;
    uint32_t stride = ((uint32_t)width * bits / 8 + 3) & ~3u;
    if(data > image->source->size || (uint32_t)height > (image->source->size - data) / stride) return false;
    for(uint32_t y = 0; y < (uint32_t)height; ++y)
    {
        uint32_t row = topDown ? y : (uint32_t)height - y - 1;
        if(!imageRead(image, data + row * stride, imageInput, stride)) return false;
        for(uint32_t x = 0; x < (uint32_t)width; ++x)
        {
            const uint8_t *p = imageInput + x * bits / 8;
            imagePixel(image, x, y, 1, 1, p[2], p[1], p[0], 255);
        }
    }
    return true;
}

bool ThemeImage_Decode(const ThemeImageSource *source, uint16_t *pixels, void *work, size_t workSize)
{
    if(!source || !source->read || !pixels || !work || workSize < THEME_IMAGE_WORK_SIZE || ((uintptr_t)work & 7) || source->size < 54 || source->size > THEME_IMAGE_MAX_FILE_SIZE) return false;
    imageArena = work;
    imageArenaSize = THEME_IMAGE_WORK_SIZE;
    ThemeBlock *first = (ThemeBlock *)work;
    first->size = THEME_IMAGE_WORK_SIZE - sizeof(ThemeBlock);
    first->used = 0;
    memset(pixels, 0, 320 * 240 * sizeof(uint16_t));
    ThemeDecode image = { .source = source, .pixels = pixels };
    uint8_t header[54];
    bool valid = false;
    if(imageRead(&image, 0, header, sizeof(header)))
    {
        if(!memcmp(header, "\x89PNG\r\n\x1A\n", 8)) valid = imagePng(&image, header);
        else if(header[0] == 0xFF && header[1] == 0xD8) valid = imageJpeg(&image);
        else if(header[0] == 'B' && header[1] == 'M') valid = imageBmp(&image, header);
    }
    imageArena = NULL;
    imageArenaSize = 0;
    return valid;
}
