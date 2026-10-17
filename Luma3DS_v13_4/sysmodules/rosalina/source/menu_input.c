#include "menu_input.h"

static uint32_t repeatDelay = 220;
static uint32_t repeatInterval = 70;

void MenuInput_SetTiming(uint32_t delay, uint32_t interval)
{
    repeatDelay = delay < 100 ? 100 : delay > 600 ? 600 : delay;
    repeatInterval = interval < 25 ? 25 : interval > 140 ? 140 : interval;
}

void MenuInput_Reset(MenuInput *input, uint32_t held)
{
    input->held = 0;
    input->blocked = held;
    input->repeatAt = 0;
}

uint32_t MenuInput_Update(MenuInput *input, uint32_t held, uint32_t now, uint32_t directions)
{
    input->blocked &= held;
    held &= ~input->blocked;
    uint32_t pressed = held & ~input->held;
    uint32_t directional = held & directions;
    if (directional != (input->held & directions))
        input->repeatAt = now + repeatDelay;
    else if (directional && (int32_t)(now - input->repeatAt) >= 0)
    {
        pressed |= directional;
        input->repeatAt = now + repeatInterval;
    }
    input->held = held;
    return pressed;
}
