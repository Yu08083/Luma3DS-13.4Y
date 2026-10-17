/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2020 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include <3ds.h>
#include <stdarg.h>
#include "fmt.h"
#include "draw.h"
#include "font.h"
#include "text.h"
#include "menus/lang.h"
#include "memory.h"
#include "menu.h"
#include "utils.h"
#include "csvc.h"

#define KERNPA2VA(a)            ((a) + (GET_VERSION_MINOR(osGetKernelVersion()) < 44 ? 0xD0000000 : 0xC0000000))

static u32 gpuSavedFramebufferAddr1, gpuSavedFramebufferAddr2, gpuSavedFramebufferFormat, gpuSavedFramebufferStride, gpuSavedFillColor;
static u32 framebufferCacheSize;
static void *framebufferCache;
static RecursiveLock lock;
static u32 gpuSavedFramebufferSelect;

void Draw_Init(void)
{
    RecursiveLock_Init(&lock);
}

void Draw_Lock(void)
{
    RecursiveLock_Lock(&lock);
}

void Draw_Unlock(void)
{
    RecursiveLock_Unlock(&lock);
}

static u32 drawDirtyFirst = SCREEN_BOT_WIDTH;
static u32 drawDirtyEnd;
static u16 drawMenuRowBuffer[SCREEN_BOT_WIDTH * SPACING_Y];
static u32 drawMenuRowY;
static bool drawMenuRowActive;
static const u16 *drawMenuBackground;

void Draw_SetMenuBackground(const u16 *pixels)
{
    drawMenuBackground = pixels;
}

static u16 Draw_BackgroundPixel(s32 x, s32 y)
{
    if(drawMenuBackground && x >= 0 && y >= 0 && x < SCREEN_BOT_WIDTH && y < SCREEN_BOT_HEIGHT)
        return drawMenuBackground[x * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - y - 1];
    return g_menuBgColor;
}

void Draw_InvalidateFramebuffer(void)
{
    drawDirtyFirst = 0;
    drawDirtyEnd = SCREEN_BOT_WIDTH;
}

static void Draw_PutPixel(s32 x, s32 y, u32 color)
{
    if (x < 0 || y < 0 || x >= SCREEN_BOT_WIDTH || y >= SCREEN_BOT_HEIGHT)
        return;
    if (drawMenuRowActive)
    {
        if ((u32)y >= drawMenuRowY && (u32)y - drawMenuRowY < SPACING_Y)
            drawMenuRowBuffer[x * SPACING_Y + SPACING_Y - ((u32)y - drawMenuRowY) - 1] = (u16)color;
        return;
    }
    if((u32)x < drawDirtyFirst) drawDirtyFirst = (u32)x;
    if((u32)x + 1 > drawDirtyEnd) drawDirtyEnd = (u32)x + 1;
    volatile u16 *fb = (volatile u16 *)FB_BOTTOM_VRAM_ADDR;
    fb[x * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - y - 1] = (u16)color;
}

void Draw_FillRect(u32 x, u32 y, u32 width, u32 height, u32 color)
{
    if (x >= SCREEN_BOT_WIDTH || y >= SCREEN_BOT_HEIGHT)
        return;
    if (width > SCREEN_BOT_WIDTH - x)
        width = SCREEN_BOT_WIDTH - x;
    if (height > SCREEN_BOT_HEIGHT - y)
        height = SCREEN_BOT_HEIGHT - y;
    if(x < drawDirtyFirst) drawDirtyFirst = x;
    if(x + width > drawDirtyEnd) drawDirtyEnd = x + width;
    volatile u16 *fb = (volatile u16 *)FB_BOTTOM_VRAM_ADDR;
    for (u32 column = x; column < x + width; ++column)
    {
        volatile u16 *pixel = fb + column * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - y - 1;
        for (u32 row = 0; row < height; ++row)
            *pixel-- = color == g_menuBgColor ? Draw_BackgroundPixel((s32)column, (s32)(y + row)) : (u16)color;
    }
}

void Draw_DrawCharacter(u32 posX, u32 posY, u32 color, char character)
{
    for (u32 y = 0; y < 10; ++y)
    {
        u8 bits = (u8)font[(u8)character * 10 + y];
        for (u32 x = 0; x < SPACING_X; ++x)
            Draw_PutPixel((s32)posX + (s32)x - 1, (s32)posY + (s32)y,
                          bits & (1u << (6 - x)) ? color : Draw_BackgroundPixel((s32)posX + (s32)x - 1, (s32)posY + (s32)y));
    }
}

static void Draw_DrawCodepoint(u32 posX, u32 posY, u32 color, u32 codepoint)
{
    if (Text_CharacterWidth(codepoint) == SPACING_X)
    {
        Draw_DrawCharacter(posX, posY, color, (char)(u8)codepoint);
        return;
    }
    const u8 *glyph = Text_FindGlyph(codepoint);
    static const u8 missing[8] = { 0xFE, 0x82, 0x82, 0x82, 0x82, 0x82, 0xFE, 0 };
    if (glyph == NULL)
        glyph = missing;
    for (u32 y = 0; y < 10; ++y)
    {
        u8 bits = y >= 1 && y < 9 ? glyph[y - 1] : 0;
        for (u32 x = 0; x < 8; ++x)
            Draw_PutPixel((s32)posX + (s32)x - 1, (s32)posY + (s32)y,
                          bits & (0x80u >> x) ? color : Draw_BackgroundPixel((s32)posX + (s32)x - 1, (s32)posY + (s32)y));
    }
}

u32 Draw_MeasureString(const char *string)
{
    return Text_MeasureWidth(string);
}

u32 Draw_DrawString(u32 posX, u32 posY, u32 color, const char *string)
{
    u32 offset = 0;
    if (posX >= SCREEN_BOT_WIDTH)
        return posY;
    while (*string)
    {
        u32 codepoint = Text_Decode(&string);
        if (codepoint == '\n')
        {
            posY += SPACING_Y;
            offset = 0;
            continue;
        }
        if (codepoint == '\t')
        {
            offset += 2 * SPACING_X;
            continue;
        }
        u32 width = Text_CharacterWidth(codepoint);
        if (offset + width > SCREEN_BOT_WIDTH - posX)
        {
            posY += SPACING_Y;
            offset = 0;
            if (codepoint == ' ')
                continue;
        }
        if (posY >= SCREEN_BOT_HEIGHT)
            break;
        Draw_DrawCodepoint(posX + offset, posY, color, codepoint);
        offset += width;
    }
    return posY;
}

void Draw_DrawMenuRow(u32 posY, u32 textColor, const char *text, u32 cursorColor, bool selected, char mark)
{
    if (posY > SCREEN_BOT_HEIGHT - SPACING_Y)
        return;
    for(u32 x = 0; x < SCREEN_BOT_WIDTH; ++x)
        for(u32 y = 0; y < SPACING_Y; ++y)
            drawMenuRowBuffer[x * SPACING_Y + SPACING_Y - y - 1] = Draw_BackgroundPixel((s32)x, (s32)(posY + y));
    drawMenuRowY = posY;
    drawMenuRowActive = true;
    Draw_DrawString(30, posY, textColor, text);
    Draw_DrawCharacter(10, posY, cursorColor, selected ? '>' : ' ');
    if (mark)
        Draw_DrawCharacter(SCREEN_BOT_WIDTH - 16, posY, cursorColor, mark);
    drawMenuRowActive = false;
    volatile u16 *fb = (volatile u16 *)FB_BOTTOM_VRAM_ADDR;
    for (u32 x = 0; x < SCREEN_BOT_WIDTH; ++x)
        memcpy((void *)(fb + x * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - posY - SPACING_Y),
               drawMenuRowBuffer + x * SPACING_Y, SPACING_Y * sizeof(u16));
    Draw_InvalidateFramebuffer();
}

u32 Draw_DrawFormattedString(u32 posX, u32 posY, u32 color, const char *fmt, ...)
{
    char buf[DRAW_MAX_FORMATTED_STRING_SIZE + 1];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    return Draw_DrawString(posX, posY, color, buf);
}


void Draw_DrawScrollMarks(u32 posY, u32 rows, u32 first, u32 total, u32 color)
{
    if (first > 0)
        Draw_DrawCharacter(SCROLL_MARK_X, posY, color, '\x1E');
    if (first + rows < total)
        Draw_DrawCharacter(SCROLL_MARK_X, posY + (rows - 1) * SPACING_Y, color, '\x1F');
}

void Draw_FillFramebuffer(u32 value)
{
    Draw_InvalidateFramebuffer();
    memset(FB_BOTTOM_VRAM_ADDR, value, FB_BOTTOM_SIZE);
}

void Draw_ClearFramebuffer(void)
{
    Draw_InvalidateFramebuffer();
    if(drawMenuBackground)
    {
        memcpy(FB_BOTTOM_VRAM_ADDR, drawMenuBackground, FB_BOTTOM_SIZE);
        return;
    }
    volatile u16 *fb = (volatile u16 *)FB_BOTTOM_VRAM_ADDR;
    for (u32 i = 0; i < FB_BOTTOM_SIZE / 2; ++i)
        fb[i] = g_menuBgColor;
}

Result Draw_AllocateFramebufferCache(u32 size)
{

    u32 addr = 0x0D000000;
    u32 tmp;

    size = (size + 0xFFF) >> 12 << 12;

    if (framebufferCache != NULL)
        __builtin_trap();

    Result res = svcControlMemoryEx(&tmp, addr, 0, size, MEMOP_ALLOC | MEMOP_REGION_SYSTEM, MEMPERM_READWRITE, true);
    if (R_FAILED(res))
    {
        framebufferCache = NULL;
        framebufferCacheSize = 0;
    }
    else
    {
        framebufferCache = (u32 *)addr;
        framebufferCacheSize = size;
    }

    return res;
}

Result Draw_AllocateFramebufferCacheForScreenshot(u32 size)
{
    u32 remaining = (u32)osGetMemRegionFree(MEMREGION_SYSTEM);
    u32 sz = remaining < size ? remaining : size;
    return Draw_AllocateFramebufferCache(sz);
}

void Draw_FreeFramebufferCache(void)
{
    u32 tmp;
    if (framebufferCache != NULL)
        svcControlMemory(&tmp, (u32)framebufferCache, 0, framebufferCacheSize, MEMOP_FREE, 0);
    framebufferCacheSize = 0;
    framebufferCache = NULL;
}

void *Draw_GetFramebufferCache(void)
{
    return framebufferCache;
}

u32 Draw_GetFramebufferCacheSize(void)
{
    return framebufferCacheSize;
}

u32 Draw_SetupFramebuffer(void)
{
    while((GPU_PSC0_CNT | GPU_PSC1_CNT | GPU_TRANSFER_CNT | GPU_CMDLIST_CNT) & 1);
    Draw_InvalidateFramebuffer();
    Draw_FlushFramebuffer();
    memcpy(framebufferCache, FB_BOTTOM_VRAM_ADDR, FB_BOTTOM_SIZE);
    gpuSavedFramebufferAddr1 = GPU_FB_BOTTOM_ADDR_1;
    gpuSavedFramebufferAddr2 = GPU_FB_BOTTOM_ADDR_2;
    gpuSavedFramebufferFormat = GPU_FB_BOTTOM_FMT;
    gpuSavedFramebufferStride = GPU_FB_BOTTOM_STRIDE;
    gpuSavedFramebufferSelect = GPU_FB_BOTTOM_SEL & 1u;
    gpuSavedFillColor = LCD_BOT_FILLCOLOR;

    LCD_BOT_FILLCOLOR = LCD_FILLCOLOR_ENABLE;
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    GPU_FB_BOTTOM_ADDR_1 = GPU_FB_BOTTOM_ADDR_2 = FB_BOTTOM_VRAM_PA;
    GPU_FB_BOTTOM_FMT = (gpuSavedFramebufferFormat & ~7u) | 2u | (3u << 8);
    GPU_FB_BOTTOM_STRIDE = SCREEN_BOT_HEIGHT * sizeof(u16);
    __dmb();
    LCD_BOT_FILLCOLOR = 0;
    return framebufferCacheSize;
}

void Draw_RestoreFramebuffer(void)
{
    LCD_BOT_FILLCOLOR = LCD_FILLCOLOR_ENABLE;
    __dmb();
    memcpy(FB_BOTTOM_VRAM_ADDR, framebufferCache, FB_BOTTOM_SIZE);
    Draw_InvalidateFramebuffer();
    Draw_FlushFramebuffer();
    GPU_FB_BOTTOM_ADDR_1 = gpuSavedFramebufferAddr1;
    GPU_FB_BOTTOM_ADDR_2 = gpuSavedFramebufferAddr2;
    GPU_FB_BOTTOM_FMT = gpuSavedFramebufferFormat;
    GPU_FB_BOTTOM_STRIDE = gpuSavedFramebufferStride;
    GPU_FB_BOTTOM_SEL = gpuSavedFramebufferSelect;
    __dmb();
    svcSleepThread(20 * 1000 * 1000LL);
    LCD_BOT_FILLCOLOR = gpuSavedFillColor;
}

void Draw_RestoreFramebufferAnimated(bool animate)
{
    u32 brightness = LCD_BOT_BRIGHTNESS;
    u32 level = brightness & 0x3FFu;
    if(animate && level)
    {
        for(u32 step = 3; ; --step)
        {
            LCD_BOT_BRIGHTNESS = (brightness & ~0x3FFu) | (level * step / 4);
            svcSleepThread(4 * 1000 * 1000LL);
            if(step == 0) break;
        }
    }
    Draw_RestoreFramebuffer();
    if(animate && level)
    {
        for(u32 step = 1; step <= 4; ++step)
        {
            LCD_BOT_BRIGHTNESS = (brightness & ~0x3FFu) | (level * step / 4);
            svcSleepThread(4 * 1000 * 1000LL);
        }
    }
    LCD_BOT_BRIGHTNESS = brightness;
}

void Draw_FlushFramebuffer(void)
{
    if(drawDirtyEnd <= drawDirtyFirst) return;
    u32 offset = (drawDirtyFirst * SCREEN_BOT_HEIGHT * sizeof(u16)) & ~63u;
    u32 end = (drawDirtyEnd * SCREEN_BOT_HEIGHT * sizeof(u16) + 63u) & ~63u;
    u32 size = end - offset;
    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)FB_BOTTOM_VRAM_ADDR + offset, size);
    drawDirtyFirst = SCREEN_BOT_WIDTH;
    drawDirtyEnd = 0;
}

u32 Draw_GetCurrentFramebufferAddress(bool top, bool left)
{
    if(GPU_FB_BOTTOM_SEL & 1)
    {
        if(left)
            return top ? GPU_FB_TOP_LEFT_ADDR_2 : GPU_FB_BOTTOM_ADDR_2;
        else
            return top ? GPU_FB_TOP_RIGHT_ADDR_2 : GPU_FB_BOTTOM_ADDR_2;
    }
    else
    {
        if(left)
            return top ? GPU_FB_TOP_LEFT_ADDR_1 : GPU_FB_BOTTOM_ADDR_1;
        else
            return top ? GPU_FB_TOP_RIGHT_ADDR_1 : GPU_FB_BOTTOM_ADDR_1;
    }
}

void Draw_GetCurrentScreenInfo(u32 *width, bool *is3d, bool top)
{
    if (top)
    {
        bool isNormal2d = (GPU_FB_TOP_FMT & BIT(6)) != 0;
        *is3d = (GPU_FB_TOP_FMT & BIT(5)) != 0;
        *width = !(*is3d) && !isNormal2d ? 800 : 400;
    }
    else
    {
        *is3d = false;
        *width = 320;
    }
}

static inline void Draw_WriteUnaligned(u8 *dst, u32 tmp, u32 size)
{
    memcpy(dst, &tmp, size);
}

void Draw_CreateBitmapHeader(u8 *dst, u32 width, u32 heigth)
{
    static const u8 bmpHeaderTemplate[54] = {

        0x42, 0x4D, 0xCC, 0xCC, 0xCC, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x40  , 0x00, 0x00, 0x00,


        0x28, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x01, 0x00, 0x18, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    memcpy(dst, bmpHeaderTemplate, 54);
    memset(dst + 54, 0, 64 - 54);
    Draw_WriteUnaligned(dst + 2, 64 + 3 * width * heigth, 4);
    Draw_WriteUnaligned(dst + 0x12, width, 4);
    Draw_WriteUnaligned(dst + 0x16, heigth, 4);
    Draw_WriteUnaligned(dst + 0x22, 3 * width * heigth, 4);
}

static inline void Draw_ConvertPixelToBGR8(u8 *dst, const u8 *src, GSPGPU_FramebufferFormat srcFormat)
{
    u8 red, green, blue;
    switch(srcFormat)
    {
        case GSP_RGBA8_OES:
        {
            u32 px = *(u32 *)src;
            dst[0] = (px >>  8) & 0xFF;
            dst[1] = (px >> 16) & 0xFF;
            dst[2] = (px >> 24) & 0xFF;
            break;
        }
        case GSP_BGR8_OES:
        {
            dst[2] = src[2];
            dst[1] = src[1];
            dst[0] = src[0];
            break;
        }
        case GSP_RGB565_OES:
        {

            u16 px = *(u16 *)src;
            blue = px & 0x1F;
            green = (px >> 5) & 0x3F;
            red = (px >> 11) & 0x1F;

            dst[0] = (blue  << 3) | (blue  >> 2);
            dst[1] = (green << 2) | (green >> 4);
            dst[2] = (red   << 3) | (red   >> 2);

            break;
        }
        case GSP_RGB5_A1_OES:
        {
            u16 px = *(u16 *)src;
            blue = (px >> 1) & 0x1F;
            green = (px >> 6) & 0x1F;
            red = (px >> 11) & 0x1F;

            dst[0] = (blue  << 3) | (blue  >> 2);
            dst[1] = (green << 3) | (green >> 2);
            dst[2] = (red   << 3) | (red   >> 2);

            break;
        }
        case GSP_RGBA4_OES:
        {
            u16 px = *(u32 *)src;
            blue = (px >> 4) & 0xF;
            green = (px >> 8) & 0xF;
            red = (px >> 12) & 0xF;

            dst[0] = (blue  << 4) | (blue  >> 0);
            dst[1] = (green << 4) | (green >> 0);
            dst[2] = (red   << 4) | (red   >> 0);

            break;
        }
        default: break;
    }
}

typedef struct FrameBufferConvertArgs {
    u8 *buf;
    u32 width;
    u8 startingLine;
    u8 numLines;
    u8 scaleFactorY;
    bool top;
    bool left;
} FrameBufferConvertArgs;

static void Draw_ConvertFrameBufferLinesKernel(const FrameBufferConvertArgs *args)
{
    static const u8 formatSizes[] = { 4, 3, 2, 2, 2 };

    GSPGPU_FramebufferFormat fmt = args->top ? (GSPGPU_FramebufferFormat)(GPU_FB_TOP_FMT & 7) : (GSPGPU_FramebufferFormat)(GPU_FB_BOTTOM_FMT & 7);
    u32 width = args->width;
    u32 stride = args->top ? GPU_FB_TOP_STRIDE : GPU_FB_BOTTOM_STRIDE;

    u32 pa = Draw_GetCurrentFramebufferAddress(args->top, args->left);
    u8 *addr = (u8 *)KERNPA2VA(pa);

    for (u32 y = args->startingLine; y < args->startingLine + args->numLines; y++)
    {
        for (u8 i = 0; i < args->scaleFactorY; i++)
        {
            for(u32 x = 0; x < width; x++)
            {
                __builtin_prefetch(addr + x * stride + y * formatSizes[fmt], 0, 3);
                Draw_ConvertPixelToBGR8(args->buf + (x + width * (args->scaleFactorY * y + i)) * 3 , addr + x * stride + y * formatSizes[fmt], fmt);
            }
        }
    }
}

void Draw_ConvertFrameBufferLines(u8 *buf, u32 width, u32 startingLine, u32 numLines, u32 scaleFactorY, bool top, bool left)
{
    FrameBufferConvertArgs args = { buf, width, (u8)startingLine, (u8)numLines, (u8)scaleFactorY, top, left };
    svcCustomBackdoor(Draw_ConvertFrameBufferLinesKernel, &args);
}
