import os
import re
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated'
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "text.h"
#include "font.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240
#define SPACING_X 6
#define SPACING_Y 11
#define FB_BOTTOM_SIZE (320 * 240 * 2)
#define FB_BOTTOM_VRAM_PA 0x1848F000u
#define FB_BOTTOM_VRAM_ADDR ((void *)(uintptr_t)0x10000000u)
#define CUR_PROCESS_HANDLE 1
#define LCD_FILLCOLOR_ENABLE (1u << 24)
static u32 GPU_PSC0_CNT, GPU_PSC1_CNT, GPU_TRANSFER_CNT, GPU_CMDLIST_CNT;
static u32 GPU_FB_BOTTOM_ADDR_1, GPU_FB_BOTTOM_ADDR_2, GPU_FB_BOTTOM_FMT, GPU_FB_BOTTOM_STRIDE, GPU_FB_BOTTOM_SEL;
static u32 LCD_BOT_FILLCOLOR, LCD_BOT_BRIGHTNESS;
static u32 gpuSavedFramebufferAddr1, gpuSavedFramebufferAddr2, gpuSavedFramebufferFormat, gpuSavedFramebufferStride;
static u32 gpuSavedFramebufferSelect, gpuSavedFillColor;
static u8 backup[FB_BOTTOM_SIZE], vram[FB_BOTTOM_SIZE];
static void *framebufferCache = backup;
static const u32 framebufferCacheSize = FB_BOTTOM_SIZE;
static u16 g_menuBgColor;
static u32 flushCount, flushBytes, lastOffset, sleepMs;
static bool restoring;
static bool checkRowCopy;
static u32 rowCopies;
#define __dmb() ((void)0)
static void svcSleepThread(int64_t ns) { assert((LCD_BOT_BRIGHTNESS & ~0x3FFu) == (0x100A080u & ~0x3FFu)); sleepMs += (u32)(ns / 1000000); }
static void svcFlushProcessDataCache(u32 process, u32 address, u32 size)
{
    assert(process == CUR_PROCESS_HANDLE);
    assert(address >= 0x10000000u && address + size <= 0x10000000u + FB_BOTTOM_SIZE);
    assert(address % 64 == 0 && size % 64 == 0);
    if(restoring) assert(LCD_BOT_FILLCOLOR & LCD_FILLCOLOR_ENABLE);
    ++flushCount;
    flushBytes += size;
    lastOffset = address - 0x10000000u;
}
static void *checkedCopy(void *dst, const void *src, size_t size)
{
    if(checkRowCopy)
    {
        assert(size == SPACING_Y * sizeof(u16));
        assert(dst == vram + (rowCopies * SCREEN_BOT_HEIGHT + SCREEN_BOT_HEIGHT - 30 - SPACING_Y) * sizeof(u16));
        if(rowCopies == 0)
            for(u32 i = 0; i < FB_BOTTOM_SIZE; ++i) assert(vram[i] == 0x5A);
        ++rowCopies;
    }
    if(dst == FB_BOTTOM_VRAM_ADDR)
    {
        if(restoring) assert(LCD_BOT_FILLCOLOR & LCD_FILLCOLOR_ENABLE);
        dst = vram;
    }
    if(src == FB_BOTTOM_VRAM_ADDR) src = vram;
    return memcpy(dst, src, size);
}
void Draw_ClearFramebuffer(void);
void Draw_FlushFramebuffer(void);
#define memcpy checkedCopy
'''
main = r'''
int main(void)
{
    for(u32 i = 0; i < FB_BOTTOM_SIZE; ++i) vram[i] = (u8)i;
    GPU_FB_BOTTOM_ADDR_1 = 0x18000000;
    GPU_FB_BOTTOM_ADDR_2 = 0x18100000;
    GPU_FB_BOTTOM_FMT = 0x12340001;
    GPU_FB_BOTTOM_STRIDE = 720;
    GPU_FB_BOTTOM_SEL = 0x70011;
    LCD_BOT_FILLCOLOR = 0x00112233;
    LCD_BOT_BRIGHTNESS = 0x100A080;
    Draw_SetupFramebuffer();
    for(u32 i = 0; i < FB_BOTTOM_SIZE; ++i) assert(backup[i] == (u8)i);
    assert(GPU_FB_BOTTOM_ADDR_1 == FB_BOTTOM_VRAM_PA && GPU_FB_BOTTOM_STRIDE == 480);
    flushCount = flushBytes = sleepMs = 0;
    Draw_DrawCharacter(10, 30, 0xFFFF, '>');
    Draw_FlushFramebuffer();
    assert(flushCount == 1 && flushBytes <= 7 * 480 && lastOffset <= 9 * 480);
    Draw_FlushFramebuffer();
    assert(flushCount == 1);
    memset(vram, 0x5A, sizeof(vram));
    checkRowCopy = true;
    Draw_DrawMenuRow(30, 0xC4FF, "本体情報", 0xC4FF, true, 'v');
    checkRowCopy = false;
    assert(rowCopies == SCREEN_BOT_WIDTH);
    Draw_FlushFramebuffer();
    static u16 theme[320 * 240];
    for(u32 i = 0; i < 320 * 240; ++i) theme[i] = (u16)(i * 17);
    Draw_SetMenuBackground(theme);
    Draw_ClearFramebuffer();
    assert(memcmp(vram, theme, FB_BOTTOM_SIZE) == 0);
    Draw_SetMenuBackground(NULL);
    Draw_ClearFramebuffer();
    for(u32 i = 0; i < FB_BOTTOM_SIZE; ++i) assert(vram[i] == 0);
    memset(vram, 0xCC, sizeof(vram));
    restoring = true;
    Draw_RestoreFramebufferAnimated(true);
    restoring = false;
    for(u32 i = 0; i < FB_BOTTOM_SIZE; ++i) assert(vram[i] == (u8)i);
    assert(GPU_FB_BOTTOM_ADDR_1 == 0x18000000 && GPU_FB_BOTTOM_ADDR_2 == 0x18100000);
    assert(GPU_FB_BOTTOM_FMT == 0x12340001 && GPU_FB_BOTTOM_STRIDE == 720 && GPU_FB_BOTTOM_SEL == 1);
    assert(LCD_BOT_FILLCOLOR == 0x00112233 && LCD_BOT_BRIGHTNESS == 0x100A080);
    assert(sleepMs == 52);
    Draw_SetupFramebuffer();
    sleepMs = 0;
    restoring = true;
    Draw_RestoreFramebufferAnimated(false);
    restoring = false;
    assert(sleepMs == 20 && LCD_BOT_BRIGHTNESS == 0x100A080);
    assert(GPU_FB_BOTTOM_FMT == 0x12340001 && LCD_BOT_FILLCOLOR == 0x00112233);
    puts("Display dirty columns, aligned cache flush, masked restore, 52 ms transition and exact register restoration: OK");
    return 0;
}
'''

for version in ('Luma3DS_v13_4',):
    newer = version == 'Luma3DS_v13_4'
    base = root / version / 'sysmodules/rosalina'
    source = (base / 'source/draw.c').read_text(encoding='utf-8')
    render = source[source.index('static u32 drawDirtyFirst'):source.index('u32 Draw_DrawFormattedString(')]
    render = render.replace('(volatile u16 *)FB_BOTTOM_VRAM_ADDR', '(volatile u16 *)vram')
    fills = source[source.index('void Draw_FillFramebuffer('):source.index('Result Draw_AllocateFramebufferCache(') if newer else source.index('void Draw_SetupFramebuffer(')]
    fills = fills.replace('memset(FB_BOTTOM_VRAM_ADDR', 'memset(vram').replace('memset32(FB_BOTTOM_VRAM_ADDR, value, FB_BOTTOM_SIZE)', 'memset(vram, value, FB_BOTTOM_SIZE)').replace('(volatile u16 *)FB_BOTTOM_VRAM_ADDR', '(volatile u16 *)vram')
    setup = source[source.index('u32 Draw_SetupFramebuffer(' if newer else 'void Draw_SetupFramebuffer('):source.index('u32 Draw_GetCurrentFramebufferAddress(')]
    setup = setup.replace('(u32)FB_BOTTOM_VRAM_ADDR', '(u32)(uintptr_t)FB_BOTTOM_VRAM_ADDR')
    target = generated / (version + '_display.c')
    target.write_text(prefix + render + fills + setup + main, encoding='utf-8', newline='\n')
    exe = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-const-variable', '-O2', '-I' + str(base / 'include'), str(target), str(base / 'source/text.c'), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
