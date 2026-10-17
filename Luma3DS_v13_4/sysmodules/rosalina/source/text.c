#include "text.h"
#include "japanese_font.h"
#include <stddef.h>

uint32_t Text_Decode(const char **text)
{
    const unsigned char *s = (const unsigned char *)*text;
    uint32_t lead = s[0];
    if (lead == 0)
        return 0;
    ++*text;
    if (lead < 0x80)
        return lead;

    uint32_t count, codepoint, minimum;
    if (lead >= 0xC2 && lead <= 0xDF)
        count = 2, codepoint = lead & 0x1F, minimum = 0x80;
    else if (lead >= 0xE0 && lead <= 0xEF)
        count = 3, codepoint = lead & 0x0F, minimum = 0x800;
    else if (lead >= 0xF0 && lead <= 0xF4)
        count = 4, codepoint = lead & 7, minimum = 0x10000;
    else
        return 0x110000 | lead;

    for (uint32_t i = 1; i < count; ++i)
    {
        if ((s[i] & 0xC0) != 0x80)
            return 0x110000 | lead;
        codepoint = (codepoint << 6) | (s[i] & 0x3F);
    }
    if (codepoint < minimum || codepoint > 0x10FFFF ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF))
        return 0x110000 | lead;
    *text += count - 1;
    return codepoint;
}

uint32_t Text_CharacterWidth(uint32_t codepoint)
{
    return codepoint < 0x80 || codepoint > 0x10FFFF ? 6 : 8;
}

uint32_t Text_MeasureWidth(const char *text)
{
    uint32_t width = 0, maximum = 0;
    while (*text)
    {
        uint32_t codepoint = Text_Decode(&text);
        if (codepoint == '\n')
        {
            if (width > maximum)
                maximum = width;
            width = 0;
        }
        else
            width += codepoint == '\t' ? 12 : Text_CharacterWidth(codepoint);
    }
    return width > maximum ? width : maximum;
}

const uint8_t *Text_FindGlyph(uint32_t codepoint)
{
    uint32_t first = 0, last = sizeof(japaneseCodepoints) / sizeof(japaneseCodepoints[0]);
    while (first < last)
    {
        uint32_t middle = first + (last - first) / 2;
        if (japaneseCodepoints[middle] == codepoint)
            return japaneseGlyphs[middle];
        if (japaneseCodepoints[middle] < codepoint)
            first = middle + 1;
        else
            last = middle;
    }
    return NULL;
}
