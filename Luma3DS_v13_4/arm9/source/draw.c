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



#include "draw.h"
#include "memory.h"
#include "screen.h"
#include "utils.h"
#include "fs.h"
#include "fmt.h"
#include "font.h"
#include "text.h"
#include "config.h"

bool loadSplash(void)
{
    static const char *topSplashFile = "splash.bin",
                      *bottomSplashFile = "splashbottom.bin";

    bool isTopSplashValid = getFileSize(topSplashFile) == SCREEN_TOP_FBSIZE,
         isBottomSplashValid = getFileSize(bottomSplashFile) == SCREEN_BOTTOM_FBSIZE;


    if(!isTopSplashValid && !isBottomSplashValid) return false;

    initScreens();

    if(isTopSplashValid) isTopSplashValid = fileRead(fbs[1].top_left, topSplashFile, SCREEN_TOP_FBSIZE) == SCREEN_TOP_FBSIZE;
    if(isBottomSplashValid) isBottomSplashValid = fileRead(fbs[1].bottom, bottomSplashFile, SCREEN_BOTTOM_FBSIZE) == SCREEN_BOTTOM_FBSIZE;

    if(!isTopSplashValid && !isBottomSplashValid) return false;

    swapFramebuffers(true);

    wait(configData.splashDurationMsec);

    return true;
}

void drawCharacter(bool isTopScreen, u32 posX, u32 posY, u32 color, char character)
{
    u32 screenWidth = isTopScreen ? SCREEN_TOP_WIDTH : SCREEN_BOTTOM_WIDTH;
    if (posX >= screenWidth || posY >= SCREEN_HEIGHT)
        return;
    u8 *screen = isTopScreen ? fbs[0].top_left : fbs[0].bottom;
    for (u32 y = 0; y < 8 && posY + y < SCREEN_HEIGHT; ++y)
    {
        u8 bits = font[(u8)character * 8 + y];
        for (u32 x = 0; x < 8 && posX + x < screenWidth; ++x)
            if (bits & (0x80u >> x))
            {
                u32 offset = ((posX + x) * SCREEN_HEIGHT + SCREEN_HEIGHT - posY - y - 1) * 3;
                screen[offset] = color >> 16;
                screen[offset + 1] = color >> 8;
                screen[offset + 2] = color;
            }
    }
}

static void drawCodepoint(bool isTopScreen, u32 x, u32 y, u32 color, u32 codepoint)
{
    if (codepoint < 0x80 || codepoint > 0x10FFFF)
    {
        drawCharacter(isTopScreen, x, y, color, (char)(u8)codepoint);
        return;
    }
    u32 screenWidth = isTopScreen ? SCREEN_TOP_WIDTH : SCREEN_BOTTOM_WIDTH;
    if (x >= screenWidth || y >= SCREEN_HEIGHT)
        return;
    const u8 *glyph = Text_FindGlyph(codepoint);
    static const u8 missing[8] = { 0xFE, 0x82, 0x82, 0x82, 0x82, 0x82, 0xFE, 0 };
    if (glyph == NULL)
        glyph = missing;
    u8 *screen = isTopScreen ? fbs[0].top_left : fbs[0].bottom;
    for (u32 row = 0; row < 8 && y + row < SCREEN_HEIGHT; ++row)
        for (u32 column = 0; column < 8 && x + column < screenWidth; ++column)
            if (glyph[row] & (0x80u >> column))
            {
                u32 offset = ((x + column) * SCREEN_HEIGHT + SCREEN_HEIGHT - y - row - 1) * 3;
                screen[offset] = color >> 16;
                screen[offset + 1] = color >> 8;
                screen[offset + 2] = color;
            }
}

u32 drawString(bool isTopScreen, u32 posX, u32 posY, u32 color, const char *string)
{
    u32 width = isTopScreen ? SCREEN_TOP_WIDTH : SCREEN_BOTTOM_WIDTH;
    u32 offset = 0;
    if (posX >= width)
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
        if (offset + SPACING_X > width - posX)
        {
            offset = 0;
            posY += SPACING_Y;
            if (codepoint == ' ')
                continue;
        }
        if (posY >= SCREEN_HEIGHT)
            break;
        drawCodepoint(isTopScreen, posX + offset, posY, color, codepoint);
        offset += SPACING_X;
    }
    return posY;
}

u32 drawFormattedString(bool isTopScreen, u32 posX, u32 posY, u32 color, const char *fmt, ...)
{
    char buf[DRAW_MAX_FORMATTED_STRING_SIZE + 1];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    return drawString(isTopScreen, posX, posY, color, buf);
}
