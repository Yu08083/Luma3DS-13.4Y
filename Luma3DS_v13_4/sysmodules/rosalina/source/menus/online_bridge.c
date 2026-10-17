#include <3ds.h>
#include <string.h>
#include <string.h>
#include "menus/online_bridge.h"
#include "menus/lang.h"
#include "plugin/online_bridge.h"
#include "menu.h"
#include "draw.h"

void RosalinaMenu_OnlineBridge(void)
{
    OnlineBridgeStatus shown = {0};
    bool wasAvailable = false, redraw = true;
    while(!menuShouldExit)
    {
        OnlineBridgeStatus current = {0};
        bool available = MemoryBlock__OnlineBridgeGet(&current);
        if(available != wasAvailable || memcmp(&shown, &current, sizeof(current))) redraw = true;
        shown = current;
        wasAvailable = available;
        if(redraw)
        {
            Draw_Lock();
            Draw_ClearFramebuffer();
            Draw_DrawString(10, 10, g_accentColor, Lang_Translate("Online connection"));
            if(!available)
                Draw_DrawString(10, 45, g_textColor, Lang_Translate("Companion plugin is not loaded."));
            else
            {
                Draw_DrawFormattedString(10, 45, g_textColor, Lang_Translate("Requested: %s"), Lang_Translate(current.requestedEnabled ? "ON" : "OFF"));
                const char *states[] = {"OFF", "Connecting...", "ON", "Error"};
                Draw_DrawFormattedString(10, 75, g_textColor, Lang_Translate("State: %s"), Lang_Translate(states[current.state <= 3 ? current.state : 3]));
                if(current.requestSeq != current.acknowledgedSeq)
                    Draw_DrawString(10, 110, g_textColor, Lang_Translate("Close the menu to apply."));
                if(current.error)
                    Draw_DrawFormattedString(10, 145, g_textColor, Lang_Translate("Operation failed (0x%08lx)."), (unsigned long)current.error);
            }
            Draw_DrawString(10, 210, g_textColor, Lang_Translate(available ? "A: Toggle   B: Back" : "B: Back"));
            Draw_FlushFramebuffer();
            Draw_Unlock();
            redraw = false;
        }
        u32 keys = waitInputWithTimeout(100);
        if(keys & KEY_B) break;
        if(available && (keys & KEY_A))
        {
            MemoryBlock__OnlineBridgeSet(!current.requestedEnabled);
            redraw = true;
        }
    }
}
