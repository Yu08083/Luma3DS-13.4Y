#pragma once

#include <stdint.h>

typedef struct {
    uint32_t held;
    uint32_t blocked;
    uint32_t repeatAt;
} MenuInput;

void MenuInput_Reset(MenuInput *input, uint32_t held);
uint32_t MenuInput_Update(MenuInput *input, uint32_t held, uint32_t now, uint32_t directions);

void MenuInput_SetTiming(uint32_t delay, uint32_t interval);
