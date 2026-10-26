#include "menu_capture_ip.h"
#include <string.h>

MenuCaptureIpState MenuCaptureIp_Format(char text[16], bool mainMenu, bool newModel,
    bool running, bool offline, bool sleeping, uint32_t address)
{
    if (!text)
        return MENU_CAPTURE_IP_HIDDEN;
    text[0] = 0;
    if (!mainMenu || !newModel || !running)
        return MENU_CAPTURE_IP_HIDDEN;
    MenuCaptureIpState state = sleeping ? MENU_CAPTURE_IP_SLEEPING : offline ? MENU_CAPTURE_IP_OFFLINE
        : !address || address == UINT32_MAX ? MENU_CAPTURE_IP_WAITING : MENU_CAPTURE_IP_READY;
    if (state != MENU_CAPTURE_IP_READY)
        strcpy(text, "IP: --");
    else
    {
        char *at = text;
        for (unsigned i = 0; i < 4; i++)
        {
            unsigned octet = (address >> (i * 8)) & 255;
            char digits[3];
            unsigned count = 0;
            do { digits[count++] = '0' + octet % 10; octet /= 10; } while (octet);
            while (count) *at++ = digits[--count];
            if (i != 3) *at++ = '.';
        }
        *at = 0;
    }
    return state;
}
