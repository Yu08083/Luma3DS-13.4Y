#pragma once

#include <stdint.h>

uint32_t Text_Decode(const char **text);
uint32_t Text_CharacterWidth(uint32_t codepoint);
uint32_t Text_MeasureWidth(const char *text);
const uint8_t *Text_FindGlyph(uint32_t codepoint);
