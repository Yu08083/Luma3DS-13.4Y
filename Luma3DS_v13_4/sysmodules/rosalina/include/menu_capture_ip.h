#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MENU_CAPTURE_IP_HIDDEN,
    MENU_CAPTURE_IP_READY,
    MENU_CAPTURE_IP_WAITING,
    MENU_CAPTURE_IP_OFFLINE,
    MENU_CAPTURE_IP_SLEEPING,
} MenuCaptureIpState;

MenuCaptureIpState MenuCaptureIp_Format(char text[16], bool mainMenu, bool newModel,
    bool running, bool offline, bool sleeping, uint32_t address);
