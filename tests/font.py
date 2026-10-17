import json
import os
import re
import shutil
import subprocess
from hashlib import sha256
from pathlib import Path

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated/font'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')

glyphs = {}
for block in (root / 'assets/fonts/misaki_gothic_2nd.bdf').read_text(encoding='utf-8').split('STARTCHAR ')[1:]:
    codepoint = int(re.search(r'^ENCODING (\d+)$', block, re.M)[1])
    if codepoint < 128:
        continue
    width, height, left, bottom = map(int, re.search(r'^BBX (.+)$', block, re.M)[1].split())
    rows = [0] * 8
    bitmap = block.split('BITMAP\n')[1].split('ENDCHAR')[0].split()
    for source_y, value in enumerate(bitmap):
        for source_x in range(width):
            x, y = left + source_x, 7 - bottom - height + source_y
            if 0 <= x < 8 and 0 <= y < 8 and int(value, 16) & (128 >> source_x):
                rows[y] |= 128 >> x
    glyphs[codepoint] = rows
codepoints = sorted(glyphs)
assert len(codepoints) == 7075 and codepoints[-1] <= 0xFFFF

filenames = ['ツチノコパンダ.png', '漢字の背景.png', '雪景色と妖怪.jpg', '麒麟と龍神.bmp', 'ｶﾀｶﾅとひらがな.png', '日本語.jpeg']
for filename in filenames:
    assert {ord(character) for character in filename if ord(character) >= 128} <= glyphs.keys()

common = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "text.h"
#include "font.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define GUARD 64
'''

rosalina = r'''
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240
#define SPACING_X 6
#define SPACING_Y 11
#define THEME_NAME_SIZE 128
static u16 framebuffer[320 * 240 + 2 * GUARD];
#define FB_BOTTOM_VRAM_ADDR (framebuffer + GUARD)
static u16 g_menuBgColor;
static u32 g_textColor = 0xF800, g_cursorColor = 0xF800;
'''

boot = r'''
#define SCREEN_TOP_WIDTH 400
#define SCREEN_BOTTOM_WIDTH 320
#define SCREEN_HEIGHT 240
#define SPACING_X 8
#define SPACING_Y 10
static u8 top[400 * 240 * 3 + 2 * GUARD], bottom[320 * 240 * 3 + 2 * GUARD];
typedef struct { u8 *top_left; u8 *bottom; } Framebuffer;
static Framebuffer fbs[] = {{top + GUARD, bottom + GUARD}};
'''

rosalina_adapter = r'''
static void clear_frame(void)
{
    memset(framebuffer, 0xA5, sizeof(framebuffer));
    memset(framebuffer + GUARD, 0, 320 * 240 * sizeof(u16));
}
static void guards(void)
{
    for (u32 index = 0; index < GUARD; index++)
        assert(framebuffer[index] == 0xA5A5 && framebuffer[GUARD + 320 * 240 + index] == 0xA5A5);
}
static u32 pixel(u32 x, u32 y) { return framebuffer[GUARD + x * 240 + 239 - y]; }
static u32 render(const char *text) { themeDrawName(30, text, false, 0); return 30; }
#define INK 0xF800
#define FIRST_X 29
#define HEIGHT 10
'''

boot_adapter = r'''
static void clear_frame(void)
{
    memset(top, 0xA5, sizeof(top));
    memset(bottom, 0xA5, sizeof(bottom));
    memset(top + GUARD, 0, 400 * 240 * 3);
    memset(bottom + GUARD, 0, 320 * 240 * 3);
}
static void guards(void)
{
    for (u32 index = 0; index < GUARD; index++)
    {
        assert(top[index] == 0xA5 && top[GUARD + 400 * 240 * 3 + index] == 0xA5);
        assert(bottom[index] == 0xA5 && bottom[GUARD + 320 * 240 * 3 + index] == 0xA5);
    }
}
static u32 pixel(u32 x, u32 y)
{
    const u8 *p = top + GUARD + (x * 240 + 239 - y) * 3;
    return (u32)p[0] << 16 | (u32)p[1] << 8 | p[2];
}
static u32 render(const char *text) { return drawString(true, 30, 30, 0x123456, text); }
#define INK 0x123456
#define FIRST_X 30
#define HEIGHT 8
'''

main = r'''
static const u8 *expected_glyph(u32 codepoint)
{
    for (u32 index = 0; index < sizeof(expectedCodepoints) / sizeof(expectedCodepoints[0]); index++)
        if (expectedCodepoints[index] == codepoint) return expectedGlyphs[index];
    return NULL;
}
static bool expected_ink(u32 codepoint, u32 x, u32 row)
{
    if (codepoint < 128)
    {
#if ROSALINA
        return (font[codepoint * 10 + row] & (1u << (6 - x))) != 0;
#else
        return (font[codepoint * 8 + row] & (0x80u >> x)) != 0;
#endif
    }
    const u8 *glyph = expected_glyph(codepoint);
    assert(glyph != NULL);
#if ROSALINA
    return row >= 1 && row < 9 && (glyph[row - 1] & (0x80u >> x));
#else
    return (glyph[row] & (0x80u >> x)) != 0;
#endif
}
int main(void)
{
    u32 index = 0;
    for (u32 codepoint = 0; codepoint <= 0xFFFF; codepoint++)
    {
        const u8 *glyph = Text_FindGlyph(codepoint);
        bool present = index < sizeof(expectedCodepoints) / sizeof(expectedCodepoints[0]) && expectedCodepoints[index] == codepoint;
        assert((glyph != NULL) == present);
        if (present) assert(memcmp(glyph, expectedGlyphs[index++], 8) == 0);
    }
    assert(index == 7075);
    const u32 outside[] = {0x10000, 0x1F600, 0x10FFFF, 0x110000, UINT32_MAX};
    for (u32 i = 0; i < sizeof(outside) / sizeof(outside[0]); i++) assert(Text_FindGlyph(outside[i]) == NULL);
    for (u32 name = 0; name < sizeof(filenames) / sizeof(filenames[0]); name++)
    {
        clear_frame();
        assert(render(filenames[name]) == 30);
        assert(Text_MeasureWidth(filenames[name]) == widths[name]);
        const char *cursor = filenames[name];
        u32 offset = 0;
        while (*cursor)
        {
            u32 codepoint = Text_Decode(&cursor);
            u32 width = Text_CharacterWidth(codepoint);
            if (codepoint >= 128) assert(Text_FindGlyph(codepoint) != NULL);
            for (u32 y = 0; y < HEIGHT; y++)
                for (u32 x = 0; x < width; x++)
                    assert(pixel(FIRST_X + offset + x, 30 + y) == (expected_ink(codepoint, x, y) ? INK : 0));
            offset += width;
        }
        assert(offset == widths[name]);
        for (u32 y = 0; y < HEIGHT; y++) assert(pixel(FIRST_X + offset, 30 + y) == 0);
        guards();
    }
    clear_frame();
#if ROSALINA
    assert(Text_CharacterWidth('A') == 6 && Text_CharacterWidth(0x30C4) == 8);
    assert(Text_MeasureWidth("日本語 / English") == 84);
    assert(Draw_DrawString(300, 10, INK, "ツチノコパンダ.png") == 54);
    Draw_DrawString(319, 239, INK, "雪景色と妖怪");
#else
    assert(Text_CharacterWidth('A') == 8 && Text_CharacterWidth(0x30C4) == 8);
    assert(Text_MeasureWidth("日本語 / English") == 104);
    assert(drawString(false, 304, 10, INK, "ツチノコパンダ.png") == 60);
    drawString(false, 319, 239, INK, "雪景色と妖怪");
#endif
    guards();
    printf("{\"glyphs\":7075,\"bmp_lookup_points\":65536,\"outside_lookup_points\":5,\"filename_cases\":%u,\"reported_filename_width\":%u}\n", (unsigned)(sizeof(filenames) / sizeof(filenames[0])), widths[0]);
    return 0;
}
'''

expected = 'static const u16 expectedCodepoints[] = {' + ','.join(str(value) for value in codepoints) + '};\n'
expected += 'static const u8 expectedGlyphs[][8] = {\n' + '\n'.join('{' + ','.join(str(value) for value in glyphs[codepoint]) + '},' for codepoint in codepoints) + '\n};\n'
expected += 'static const char *filenames[] = {' + ','.join(json.dumps(name, ensure_ascii=False) for name in filenames) + '};\n'
records = []
for version in ['Luma3DS_v13_4']:
    for is_rosalina in [True, False]:
        base = root / version / ('sysmodules/rosalina' if is_rosalina else 'arm9/source' if version == 'Luma3DS_v13_4' else 'source')
        include = base / 'include' if is_rosalina else base
        source = base / 'source' if is_rosalina else base
        header = include / 'japanese_font.h'
        data = header.read_bytes()
        assert b'\r' not in data
        table = data.decode().split('static const uint8_t', 1)[0]
        assert [int(value, 16) for value in re.findall(r'0x([0-9A-F]{4})', table)] == codepoints
        draw = (source / 'draw.c').read_text(encoding='utf-8')
        if is_rosalina:
            renderer = draw[draw.index('static u32 drawDirtyFirst'):draw.index('u32 Draw_DrawFormattedString(')]
            theme = (source / 'menus/theme.c').read_text(encoding='utf-8')
            renderer += theme[theme.index('static void themeDrawName('):theme.index('void RosalinaMenu_Theme(')]
            adapter = rosalina_adapter
        else:
            renderer = draw[draw.index('void drawCharacter('):draw.index('u32 drawFormattedString(')]
            adapter = boot_adapter
        widths = [sum(6 if ord(character) < 128 else 8 for character in name) if is_rosalina else len(name) * 8 for name in filenames]
        name = version + ('-rosalina' if is_rosalina else '-boot')
        target = generated / (name + '.c')
        executable = generated / (name + ('.exe' if os.name == 'nt' else ''))
        content = '#define ROSALINA ' + str(int(is_rosalina)) + '\n' + common + (rosalina if is_rosalina else boot) + expected
        content += 'static const u32 widths[] = {' + ','.join(str(width) for width in widths) + '};\n' + renderer + adapter + main
        target.write_bytes(content.encode('utf-8'))
        subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-I' + str(include), str(target), str(source / 'text.c'), '-o', str(executable)], check=True, env=environment)
        result = subprocess.run([str(executable)], check=True, capture_output=True, text=True, env=environment)
        record = json.loads(result.stdout)
        record.update({'target': name, 'font_bytes': len(codepoints) * 10, 'font_sha256': sha256(data).hexdigest(), 'actual_renderer': True, 'actual_theme_filename_renderer': is_rosalina})
        records.append(record)
        print(json.dumps(record))
assert len({record['font_sha256'] for record in records}) == 1
(generated / 'report.json').write_bytes((json.dumps({'targets': records, 'hardware_tested': False}, indent=2) + '\n').encode('utf-8'))
