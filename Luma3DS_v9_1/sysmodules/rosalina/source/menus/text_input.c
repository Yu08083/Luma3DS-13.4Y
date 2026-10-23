/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2021 Aurora Wright, TuxSH
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
#include "menus/text_input.h"
#include "menus/lang.h"
#include "draw.h"
#include "fmt.h"

#define KB_COLS 13

static const char *const kbRows[2][4] = {
    {
        "abcdefghijklm",
        "nopqrstuvwxyz",
        "0123456789-_.",
        " ()[]&+,;=~!# ",
    },
    {
        "ABCDEFGHIJKLM",
        "NOPQRSTUVWXYZ",
        "0123456789-_.",
        " ()[]&+,;=~!# ",
    },
};

static u32 kbRowLen(u32 set, u32 row)
{
    u32 n = 0;
    while (kbRows[set][row][n])
        ++n;
    return n;
}

bool TextInput_Prompt(const char *title, char *out, u32 outSize, const char *initial)
{
    char buf[256];
    u32 len = 0;
    u32 set = 0;
    u32 row = 0;
    u32 col = 0;

    if (outSize == 0)
        return false;
    if (outSize > sizeof(buf))
        outSize = sizeof(buf);

    if (initial != NULL)
    {
        while (initial[len] != 0 && len + 1 < outSize)
        {
            buf[len] = initial[len];
            ++len;
        }
    }
    buf[len] = 0;

    for (;;)
    {
        Draw_Lock();
        Draw_ClearFramebuffer();

        Draw_DrawString(10, 4, COLOR_TITLE, title);

        u32 shown = len;
        const char *tail = buf;
        if (shown > 46)
        {
            tail = buf + (len - 46);
            shown = 46;
        }
        Draw_DrawFormattedString(10, 4 + 2 * SPACING_Y, COLOR_WHITE, "%s%s_   ",
                                 len > 46 ? "..." : "", tail);

        u32 y = 4 + 5 * SPACING_Y;
        for (u32 r = 0; r < 4; ++r)
        {
            u32 n = kbRowLen(set, r);
            for (u32 c = 0; c < n; ++c)
            {
                char ch[2] = { kbRows[set][r][c], 0 };
                u32 color = (r == row && c == col) ? COLOR_LIME : COLOR_WHITE;
                Draw_DrawString(20 + c * 18, y, color, ch);
            }
            y += SPACING_Y * 2;
        }

        Draw_DrawString(10, 200, COLOR_WHITE, Lang(STR_A_TYPE_X_BACKSPACE_Y_CASE));
        Draw_DrawString(10, 200 + SPACING_Y, COLOR_WHITE, Lang(STR_START_CONFIRM_B_CANCEL));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 k = waitInput();

        if (terminationRequest)
            return false;

        if (k & BUTTON_B)
            return false;

        if (k & BUTTON_START)
        {
            if (len == 0)
                continue;
            for (u32 i = 0; i <= len; ++i)
                out[i] = buf[i];
            return true;
        }

        if (k & BUTTON_Y)
        {
            set ^= 1;
            continue;
        }

        if (k & BUTTON_X)
        {
            if (len > 0)
                buf[--len] = 0;
            continue;
        }

        if (k & BUTTON_A)
        {
            char ch = kbRows[set][row][col];
            if (ch != 0 && len + 1 < outSize)
            {
                buf[len++] = ch;
                buf[len] = 0;
            }
            continue;
        }

        if (k & BUTTON_DOWN)
            row = (row + 1) % 4;
        else if (k & BUTTON_UP)
            row = (row + 3) % 4;
        else if (k & BUTTON_RIGHT)
            col = col + 1;
        else if (k & BUTTON_LEFT)
            col = (col == 0) ? KB_COLS : col - 1;

        u32 n = kbRowLen(set, row);
        if (n == 0)
            n = 1;
        if (col >= n)
            col = (k & BUTTON_RIGHT) ? 0 : n - 1;
    }
}

bool ConfirmPrompt(const char *title, const char *line1, const char *line2)
{
    for (;;)
    {
        Draw_Lock();
        Draw_ClearFramebuffer();

        Draw_DrawString(10, 4, COLOR_TITLE, title);

        u32 y = 4 + 3 * SPACING_Y;
        if (line1 != NULL)
            y = Draw_DrawFormattedString(10, y, COLOR_WHITE, "%s\n", line1);
        if (line2 != NULL)
            y = Draw_DrawFormattedString(10, y, COLOR_WHITE, "%s\n", line2);

        Draw_DrawString(10, 210, COLOR_RED, Lang(STR_A_CONFIRM_B_CANCEL));

        Draw_FlushFramebuffer();
        Draw_Unlock();

        u32 k = waitInput();

        if (terminationRequest || (k & BUTTON_B))
            return false;
        if (k & BUTTON_A)
            return true;
    }
}
