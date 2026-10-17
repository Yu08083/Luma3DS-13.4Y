import json
import os
import re
import shutil
import struct
import subprocess
import zlib
from pathlib import Path

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
format_pattern = re.compile(r'%(?:[-+ #0]*)(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|ll|[hljztL])?[%diuoxXfFeEgGaAcspn]')
literal = r'"(?:\\.|[^"\\])*"'

main = r'''
static void checkGuards(void)
{
    for (unsigned i = 0; i < GUARD; ++i)
    {
        assert(testFramebuffer[i] == 0xA55A);
        assert(testFramebuffer[GUARD + SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT + i] == 0xA55A);
    }
}

static unsigned encode(uint32_t cp, char s[5])
{
    unsigned n;
    if (cp < 0x80)
        s[0] = (char)cp, n = 1;
    else if (cp < 0x800)
        s[0] = (char)(0xC0 | (cp >> 6)), s[1] = (char)(0x80 | (cp & 63)), n = 2;
    else if (cp < 0x10000)
        s[0] = (char)(0xE0 | (cp >> 12)), s[1] = (char)(0x80 | ((cp >> 6) & 63)), s[2] = (char)(0x80 | (cp & 63)), n = 3;
    else
        s[0] = (char)(0xF0 | (cp >> 18)), s[1] = (char)(0x80 | ((cp >> 12) & 63)), s[2] = (char)(0x80 | ((cp >> 6) & 63)), s[3] = (char)(0x80 | (cp & 63)), n = 4;
    s[n] = 0;
    return n;
}

int main(void)
{
    for (unsigned i = 0; i < sizeof(testFramebuffer) / sizeof(testFramebuffer[0]); ++i)
        testFramebuffer[i] = 0xA55A;
    for (uint32_t cp = 1; cp <= 0x10FFFF; ++cp)
    {
        if (cp >= 0xD800 && cp <= 0xDFFF)
            continue;
        char encoded[5];
        unsigned length = encode(cp, encoded);
        const char *cursor = encoded;
        assert(Text_Decode(&cursor) == cp);
        assert(cursor == encoded + length);
    }
    const char *invalid[] = { "\xE3", "\xE3\x81", "\xC0\xAF", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xFF", "\x80" };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        const char *cursor = invalid[i];
        while (*cursor)
        {
            const char *previous = cursor;
            assert(Text_Decode(&cursor) > 0x10FFFF);
            assert(cursor == previous + 1);
        }
    }
    const char *legacy = "\xF8";
    assert(Text_Decode(&legacy) == 0x1100F8);
    assert(Text_CharacterWidth(0x1100F8) == 6);
    assert(Text_FindGlyph(0x1F600) == NULL);
    assert(Draw_MeasureString("日本語 / English") == 84);
    assert(Draw_MeasureString("日本語\nA\tB") == 24);
    assert(Draw_DrawString(300, 10, 0xF800, "日本語") == 21);
    Draw_DrawCharacter(0, 239, 0xF800, (char)0xFF);
    Draw_DrawString(319, 239, 0xF800, "日本語");
    Draw_DrawString(10, 230, 0xF800, "日本語\n日本語\n日本語");
    Draw_DrawString(0, 0, 0xF800, "\xF0\x9F\x98\x80");
    checkGuards();

    static u16 expectedRowFrame[SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT];
    g_menuBgColor = 0x1234;
    memset(testFramebuffer + GUARD, 0x5A, sizeof(expectedRowFrame));
    Draw_FillRect(0, 30, 320, SPACING_Y, g_menuBgColor);
    Draw_DrawString(30, 30, 0xC4FF, "本体情報");
    Draw_DrawCharacter(10, 30, 0xC4FF, '>');
    Draw_DrawCharacter(304, 30, 0xC4FF, 'v');
    memcpy(expectedRowFrame, testFramebuffer + GUARD, sizeof(expectedRowFrame));
    memset(testFramebuffer + GUARD, 0x5A, sizeof(expectedRowFrame));
    Draw_DrawMenuRow(30, 0xC4FF, "本体情報", 0xC4FF, true, 'v');
    assert(memcmp(expectedRowFrame, testFramebuffer + GUARD, sizeof(expectedRowFrame)) == 0);
    Draw_DrawMenuRow(30, 0xC4FF, "A", 0xC4FF, false, 0);
    for(u32 x = 36; x < SCREEN_BOT_WIDTH; ++x)
        for(u32 y = 30; y < 30 + SPACING_Y; ++y)
            assert(testFramebuffer[GUARD + x * 240 + 239 - y] == g_menuBgColor);
    Draw_DrawMenuRow(229, 0xC4FF, "日本語\n日本語", 0xC4FF, true, 0);
    Draw_DrawMenuRow(239, 0xC4FF, "日本語", 0xC4FF, true, 0);
    checkGuards();
    for(u32 i = 0; i < SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT; ++i)
        expectedRowFrame[i] = (u16)(i * 13 + 0x1000);
    Draw_SetMenuBackground(expectedRowFrame);
    Draw_FillRect(0, 0, 320, 240, g_menuBgColor);
    assert(memcmp(testFramebuffer + GUARD, expectedRowFrame, sizeof(expectedRowFrame)) == 0);
    Draw_DrawString(30, 30, 0xC4FF, "本体情報");
    Draw_DrawCharacter(10, 30, 0xC4FF, '>');
    Draw_DrawCharacter(10, 30, 0xC4FF, ' ');
    for(u32 x = 9; x < 15; ++x)
        for(u32 y = 30; y < 40; ++y)
            assert(testFramebuffer[GUARD + x * 240 + 239 - y] == expectedRowFrame[x * 240 + 239 - y]);
    Draw_DrawMenuRow(30, 0xC4FF, "", 0xC4FF, false, 0);
    assert(memcmp(testFramebuffer + GUARD, expectedRowFrame, sizeof(expectedRowFrame)) == 0);
    Draw_SetMenuBackground(NULL);
    Draw_FillRect(0, 0, 320, 240, g_menuBgColor);
    for(u32 i = 0; i < SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT; ++i)
        assert(testFramebuffer[GUARD + i] == g_menuBgColor);
    checkGuards();

    g_language = LANG_JP;
    for (u32 i = 0; i < sizeof(g_labels) / sizeof(g_labels[0]); ++i)
    {
        assert(strcmp(Lang_Translate(g_labels[i].english), g_labels[i].japanese) == 0);
        if (i > 0)
            assert(strcmp(g_labels[i - 1].english, g_labels[i].english) < 0);
    }
    assert(strcmp(Lang(STR_CHARGE), "残量:         %s %%\n") == 0);
    assert(strcmp(Lang_Translate("Colors..."), "配色...") == 0);
    assert(strcmp(Lang_Translate("unchanged.3gx"), "unchanged.3gx") == 0);
    assert(strcmp(Lang(STR_COUNT), "") == 0);
    g_language = LANG_EN;
    assert(strcmp(Lang(STR_CHARGE), "Charge:        %s %%\n") == 0);
    assert(strcmp(Lang_Translate("Colors..."), "Colors...") == 0);
    g_language = LANG_JP;

    g_menuBgColor = 0;
    memset(testFramebuffer + GUARD, 0, SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT * sizeof(u16));
    Draw_DrawString(10, 10, 0xF800, "Rosalinaメニュー [v13.4Y]");
    const char *items[] = { "スクリーンショット撮影", "画面フィルター...", "チート...", "プラグイン: [有効]", "New 3DS設定...", "プロセス一覧", "デバッガー設定...", "本体設定...", "その他の設定...", "配色...", "Language / 言語", "3gx / txtチェック", "SDカードを確認", "ファイルブラウザー", "設定を保存" };
    for (unsigned i = 0; i < sizeof(items) / sizeof(items[0]); ++i)
        Draw_DrawString(30, 30 + i * 11, 0xFFFF, items[i]);
    Draw_DrawCharacter(10, 30 + 9 * 11, 0xF800, '>');
    Draw_DrawString(10, 220, 0xF800, "Luma3DS v13.4Y");
    const u8 *glyph = Text_FindGlyph(0x65E5);
    assert(glyph != NULL);
    Draw_DrawString(270, 210, 0x07E0, "日");
    for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 8; ++x)
        {
            u16 pixel = testFramebuffer[GUARD + (269 + x) * 240 + 239 - (211 + y)];
            assert(pixel == ((glyph[y] & (0x80 >> x)) ? 0x07E0 : 0));
        }
    checkGuards();
    FILE *image = fopen("renderer.rgb565", "wb");
    assert(image != NULL);
    assert(fwrite(testFramebuffer + GUARD, sizeof(u16), SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT, image) == SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT);
    fclose(image);
    puts("UTF-8 round-trip, legacy glyphs, clipping, Japanese glyphs, EN/JP selection: OK");
    return 0;
}
'''

for version in ('Luma3DS_v13_4',):
    base = root / version / 'sysmodules/rosalina'
    source = (base / 'source/draw.c').read_text(encoding='utf-8')
    renderer = source[source.index('static u32 drawDirtyFirst'):source.index('u32 Draw_DrawFormattedString(')]
    lang = (base / 'source/menus/lang.c').read_text(encoding='utf-8')
    lang_header = (base / 'include/menus/lang.h').read_text(encoding='utf-8')
    lang_types = re.search(r'typedef enum LangCode.*?} LangCode;', lang_header, re.S)[0] + '\n' + re.search(r'enum StrId.*?};', lang_header, re.S)[0]
    lang_code = lang[lang.index('static const char *const g_strings'):lang.index('\nvoid Lang_Load(void)')]
    translation_count = 0
    for match in re.finditer(r'\{\s*(' + literal + r'),\s*(' + literal + r')\s*\}', lang_code):
        english, japanese = map(json.loads, match.groups())
        assert format_pattern.findall(english) == format_pattern.findall(japanese), (version, english, japanese)
        translation_count += 1
    codepoints = {int(value, 16) for value in re.findall(r'0x([0-9A-F]{4})', (base / 'include/japanese_font.h').read_text().split('static const uint8_t')[0])}
    for path in (base / 'source').rglob('*.c'):
        file_source = path.read_text(encoding='utf-8-sig')
        if 'Lang_Translate(' in file_source:
            assert '#include "menus/lang.h"' in file_source, str(path)
        for text in re.findall(literal, file_source):
            assert {ord(c) for c in text if ord(c) >= 128} <= codepoints, str(path)
    menu = (base / 'source/menu.c').read_text(encoding='utf-8')
    footer_calls = re.findall(r'Draw_DrawFormattedString\([^\n]+"Luma3DS %s[^\n]+', menu)
    assert len(footer_calls) == 2 and all('g_accentColor' in call for call in footer_calls)
    source_path = generated / (version + '.c')
    source_path.write_text('''#include <assert.h>
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
#define SCREEN_BOT_WIDTH 320
#define SCREEN_BOT_HEIGHT 240
#define SPACING_X 6
#define SPACING_Y 11
#define GUARD 64
static u16 testFramebuffer[SCREEN_BOT_WIDTH * SCREEN_BOT_HEIGHT + 2 * GUARD];
#define FB_BOTTOM_VRAM_ADDR (testFramebuffer + GUARD)
static u16 g_menuBgColor = 0x1234;
''' + lang_types + '''
LangCode g_language = LANG_EN;
static bool g_langLoaded = false;
void Lang_Load(void) { g_langLoaded = true; }
''' + lang_code + '\n' + renderer + main, encoding='utf-8', newline='\n')
    executable = generated / (version + ('.exe' if os.name == 'nt' else ''))
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-I' + str(base / 'include'), str(source_path), str(base / 'source/text.c'), '-o', str(executable)], check=True, env=env)
    subprocess.run([str(executable)], cwd=generated, check=True, env=env)
    print(version, translation_count, 'format contracts and font coverage OK')

data = (generated / 'renderer.rgb565').read_bytes()
rows = bytearray()
for y in range(240):
    line = bytearray()
    for x in range(320):
        value = struct.unpack_from('<H', data, (x * 240 + 239 - y) * 2)[0]
        rgb = bytes(((value >> 11) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31))
        line.extend(rgb * 3)
    rows.extend((b'\0' + line) * 3)
def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 960, 720, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(bytes(rows))) + chunk(b'IEND', b'')
(generated / 'japanese-preview.png').write_bytes(png)
