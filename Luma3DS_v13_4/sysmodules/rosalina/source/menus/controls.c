#include <3ds.h>
#include "menus/controls.h"
#include "menus/lang.h"
#include "menu.h"
#include "menu_input.h"
#include "draw.h"
#include "ifile.h"
#include "fmt.h"
#include "memory.h"

#define CONTROLS_PATH "/luma/menu.cfg"

u32 g_menuRepeatDelay = 220;
u32 g_menuRepeatInterval = 70;
bool g_menuTransitions = true;
static bool controlsLoaded;

void MenuControls_Apply(void)
{
    MenuInput_SetTiming(g_menuRepeatDelay, g_menuRepeatInterval);
    hidSetRepeatParameters(g_menuRepeatDelay, g_menuRepeatInterval);
}

void MenuControls_Load(void)
{
    if(controlsLoaded) return;
    controlsLoaded = true;
    IFile file;
    if(R_SUCCEEDED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, CONTROLS_PATH), FS_OPEN_READ)))
    {
        char text[32] = {0};
        u64 read = 0;
        u32 values[3] = {0}, count = 0, offset = 0;
        if(R_SUCCEEDED(IFile_Read(&file, &read, text, sizeof(text) - 1)))
        {
            while(count < 3 && offset < read)
            {
                while(text[offset] == ' ' || text[offset] == '\n' || text[offset] == '\r') ++offset;
                if(text[offset] < '0' || text[offset] > '9') break;
                u32 value = 0;
                while(text[offset] >= '0' && text[offset] <= '9' && value <= 10000)
                    value = value * 10 + (u32)(text[offset++] - '0');
                values[count++] = value;
            }
            if(count == 3 && values[0] >= 100 && values[0] <= 600 && values[1] >= 25 && values[1] <= 140 && values[2] <= 1)
            {
                g_menuRepeatDelay = values[0];
                g_menuRepeatInterval = values[1];
                g_menuTransitions = values[2] != 0;
            }
        }
        IFile_Close(&file);
    }
    MenuControls_Apply();
}

static bool controlsSave(u32 delay, u32 interval, bool transitions)
{
    IFile file;
    if(R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, CONTROLS_PATH), FS_OPEN_CREATE | FS_OPEN_WRITE))) return false;
    char text[32];
    u32 length = (u32)sprintf(text, "%lu %lu %u\n", (unsigned long)delay, (unsigned long)interval, transitions ? 1 : 0);
    u64 written = 0;
    Result result = IFile_Write(&file, &written, text, length, FS_WRITE_FLUSH);
    if(R_SUCCEEDED(result) && written == length)
        result = FSFILE_SetSize(file.handle, length);
    IFile_Close(&file);
    return R_SUCCEEDED(result) && written == length;
}

static u32 controlsStep(u32 value, const u32 *choices, u32 count, bool increase)
{
    if(increase)
    {
        for(u32 i = 0; i < count; ++i)
            if(choices[i] > value) return choices[i];
        return choices[count - 1];
    }
    for(u32 i = count; i > 0; --i)
        if(choices[i - 1] < value) return choices[i - 1];
    return choices[0];
}

void RosalinaMenu_Controls(void)
{
    MenuControls_Load();
    u32 delay = g_menuRepeatDelay, interval = g_menuRepeatInterval, selected = 0;
    bool transitions = g_menuTransitions, redraw = true, saveFailed = false;
    static const u32 delays[] = {100, 150, 220, 300, 450, 600};
    static const u32 intervals[] = {25, 40, 55, 70, 100, 140};
    while(!menuShouldExit)
    {
        if(redraw)
        {
            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_DrawString(10, 10, g_accentColor, Lang_Translate("Menu controls"));
            Draw_DrawFormattedString(30, 40, g_textColor, Lang_Translate("Repeat delay: %lu ms"), (unsigned long)delay);
            Draw_DrawFormattedString(30, 62, g_textColor, Lang_Translate("Repeat interval: %lu ms"), (unsigned long)interval);
            Draw_DrawFormattedString(30, 84, g_textColor, Lang_Translate("Transitions: %s"), Lang_Translate(transitions ? "On" : "Off"));
            Draw_DrawCharacter(10, 40 + selected * 22, g_cursorColor, '>');
            Draw_DrawString(10, 120, g_textColor, Lang_Translate("Up/Down: select  Left/Right: change"));
            Draw_DrawString(10, 142, g_textColor, Lang_Translate("X: defaults  A: save  B: cancel"));
            if(saveFailed) Draw_DrawString(10, 175, COLOR_RED, Lang_Translate("Unable to save settings."));
            Draw_FlushFramebuffer();
            Draw_Unlock();
            redraw = false;
        }
        u32 pressed = waitInputWithTimeout(100);
        if(pressed & KEY_B) break;
        if(pressed & KEY_A)
        {
            if(controlsSave(delay, interval, transitions))
            {
                g_menuRepeatDelay = delay;
                g_menuRepeatInterval = interval;
                g_menuTransitions = transitions;
                MenuControls_Apply();
                return;
            }
            saveFailed = redraw = true;
        }
        else if(pressed & KEY_UP) { selected = (selected + 2) % 3; redraw = true; }
        else if(pressed & KEY_DOWN) { selected = (selected + 1) % 3; redraw = true; }
        else if(pressed & KEY_X) { delay = 220; interval = 70; transitions = true; redraw = true; }
        else if(pressed & (KEY_LEFT | KEY_RIGHT))
        {
            bool increase = (pressed & KEY_RIGHT) != 0;
            if(selected == 0) delay = controlsStep(delay, delays, sizeof(delays) / sizeof(delays[0]), increase);
            else if(selected == 1) interval = controlsStep(interval, intervals, sizeof(intervals) / sizeof(intervals[0]), increase);
            else transitions = !transitions;
            redraw = true;
        }
    }
}
