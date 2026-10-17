/*
*   This file is part of Luma3DS (Level256 fork)
*   Copyright (C) 2026 Level256
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
*/

#include <3ds.h>
#include "draw.h"
#include "fmt.h"
#include "menu.h"
#include "menus/lang.h"
#include "menus/streaming.h"
#include "minisoc.h"
#include "remote_play.h"

typedef struct StreamingView {
    bool running;
    bool offline;
    RemotePlayState state;
    u32 consoleIp;
    u32 viewerIp;
    u32 fps;
    u32 quality;
    Result result;
    LangCode language;
    u32 textColor;
    u32 accentColor;
    u32 cursorColor;
} StreamingView;

static void formatIp(char buffer[16], u32 address)
{
    sprintf(buffer, "%lu.%lu.%lu.%lu", (unsigned long)(address & 255),
            (unsigned long)((address >> 8) & 255), (unsigned long)((address >> 16) & 255),
            (unsigned long)(address >> 24));
}

static StreamingView captureStatus(Result lastResult)
{
    RemotePlayStatus status = {0};
    RemotePlay_GetStatus(&status);
    StreamingView view = {0};
    view.running = RemotePlay_IsRunning();
    view.offline = view.running && status.noNetwork;
    view.state = view.running ? status.state : REMOTEPLAY_OFF;
    if(view.running && !view.offline)
    {
        u32 address = (u32)socGethostid();
        if(address != UINT32_MAX)
            view.consoleIp = address;
    }
    if(view.running && status.state == REMOTEPLAY_STREAMING && !view.offline)
    {
        view.viewerIp = status.viewerIp;
        view.fps = status.fps;
        view.quality = status.quality;
    }
    view.result = lastResult;
    view.language = g_language;
    view.textColor = g_textColor;
    view.accentColor = g_accentColor;
    view.cursorColor = g_cursorColor;
    return view;
}

static const char *stateText(const StreamingView *view)
{
    if(view->offline)
        return "Paused (offline)";
    if(!view->running)
        return "OFF";
    return view->state == REMOTEPLAY_STREAMING ? "Streaming" : "Waiting for viewer";
}

static bool sameStatus(const StreamingView *first, const StreamingView *second)
{
    return first->running == second->running && first->offline == second->offline &&
           first->state == second->state && first->consoleIp == second->consoleIp &&
           first->viewerIp == second->viewerIp && first->fps == second->fps &&
           first->quality == second->quality && first->result == second->result &&
           first->language == second->language && first->textColor == second->textColor &&
           first->accentColor == second->accentColor && first->cursorColor == second->cursorColor;
}

static void drawStatus(const StreamingView *view)
{
    char address[16];
    Draw_DrawString(10, 10, g_accentColor, Lang_Translate("Screen capture (wireless)"));
    Draw_DrawFormattedString(10, 38, g_textColor, Lang_Translate("State: %s"), Lang_Translate(stateText(view)));
    if(view->consoleIp)
    {
        formatIp(address, view->consoleIp);
        Draw_DrawFormattedString(10, 62, g_textColor, Lang_Translate("3DS IP: %s"), address);
    }
    if(view->viewerIp)
    {
        formatIp(address, view->viewerIp);
        Draw_DrawFormattedString(10, 86, g_textColor, Lang_Translate("Viewer IP: %s"), address);
    }
    if(view->running && !view->offline && view->state == REMOTEPLAY_STREAMING)
        Draw_DrawFormattedString(10, 110, g_textColor, Lang_Translate("FPS: %lu   Quality: %lu"),
                                 (unsigned long)view->fps, (unsigned long)view->quality);
    if(R_FAILED(view->result))
        Draw_DrawFormattedString(10, 142, g_textColor, Lang_Translate("Operation failed (0x%08lx)."),
                                 (unsigned long)(u32)view->result);
    if(view->offline)
        Draw_DrawString(10, 166, g_textColor, Lang_Translate("Waiting for Wi-Fi."));
    else if(view->running)
        Draw_DrawString(10, 166, g_textColor, Lang_Translate("PC viewer: NTR mode"));
    Draw_DrawString(10, 210, g_cursorColor,
                    Lang_Translate(view->running ? "A: stop      B: back" : "A: start     B: back"));
}

void N3DSMenu_Streaming(void)
{
    Result lastResult = 0;
    StreamingView shown = {0};
    bool redraw = true;
    while(!menuShouldExit)
    {
        StreamingView current = captureStatus(lastResult);
        if(!sameStatus(&shown, &current))
            redraw = true;
        shown = current;
        if(redraw)
        {
            Draw_Lock();
            Draw_ClearFramebuffer();
            drawStatus(&current);
            Draw_FlushFramebuffer();
            Draw_Unlock();
            redraw = false;
        }
        u32 keys = waitInputWithTimeout(500);
        if(keys & KEY_B)
            break;
        if(keys & KEY_A)
        {
            bool running = RemotePlay_IsRunning();
            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_DrawString(10, 10, g_accentColor, Lang_Translate("Screen capture (wireless)"));
            Draw_DrawString(10, 38, g_textColor, Lang_Translate(running ? "Stopping..." : "Starting..."));
            Draw_FlushFramebuffer();
            Draw_Unlock();
            lastResult = running ? RemotePlay_Stop() : RemotePlay_Start();
            redraw = true;
        }
    }
    if(RemotePlay_IsRunning())
        RemotePlay_SaveViewer();
}
