import ast
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
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
literal = r'"(?:\\.|[^"\\])*"'

def array(source, name):
    body = re.search(r'static const char \*' + name + r'\[\]\s*=\s*\{(.*?)\};', source, re.S)[1]
    entries = []
    for match in re.finditer(r'((?:' + literal + r'\s*)+)(?:,|$)', body):
        entries.append(''.join(ast.literal_eval(part) for part in re.findall(literal, match[1])))
    return entries

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "text.h"
typedef uint8_t u8;
typedef uint32_t u32;
#define SCREEN_TOP_WIDTH 400
#define SCREEN_BOTTOM_WIDTH 320
#define SCREEN_HEIGHT 240
#define SPACING_X 8
#define SPACING_Y 10
#define GUARD 64
static u8 top[400 * 240 * 3 + 2 * GUARD];
static u8 bottom[320 * 240 * 3 + 2 * GUARD];
typedef struct { u8 *top_left; u8 *bottom; } Framebuffer;
static Framebuffer fbs[] = { {top + GUARD, bottom + GUARD} };
static char configValue;
static bool writeFails;
static bool fileWrite(const void *src, const char *path, u32 size)
{
    assert(strcmp(path, "lang.cfg") == 0 && size == 2);
    assert(((const char *)src)[1] == '\n');
    if (writeFails) return false;
    configValue = ((const char *)src)[0];
    return true;
}
static u32 fileRead(void *dst, const char *path, u32 size)
{
    assert(strcmp(path, "lang.cfg") == 0 && size >= 2);
    if (!configValue) return 0;
    ((char *)dst)[0] = configValue;
    ((char *)dst)[1] = '\n';
    return 2;
}
'''
main = r'''
static void checkGuards(void)
{
    for (unsigned i = 0; i < GUARD; ++i)
    {
        assert(top[i] == 0xA5 && top[GUARD + 400 * 240 * 3 + i] == 0xA5);
        assert(bottom[i] == 0xA5 && bottom[GUARD + 320 * 240 * 3 + i] == 0xA5);
    }
}
int main(void)
{
    memset(top, 0xA5, sizeof(top));
    memset(bottom, 0xA5, sizeof(bottom));
    memset(top + GUARD, 0, 400 * 240 * 3);
    memset(bottom + GUARD, 0, 320 * 240 * 3);
    assert(BootLanguage_IsJapanese());
    configValue = '0';
    BootLanguage_Load();
    assert(!BootLanguage_IsJapanese());
    assert(strcmp(BootLanguage_Text("Press A to select, START to save"), "Press A to select, START to save") == 0);
    configValue = '1';
    BootLanguage_Load();
    assert(BootLanguage_IsJapanese());
    assert(strcmp(BootLanguage_Text("Press A to select, START to save"), "十字: 選択  A: 切替  START: 保存して終了") == 0);
    configValue = 0;
    BootLanguage_Load();
    assert(BootLanguage_IsJapanese());
    assert(strstr(BootLanguage_Selector(), "[日本語]") != NULL);
    assert(BootLanguage_Toggle());
    assert(!BootLanguage_IsJapanese() && configValue == '0');
    assert(strstr(BootLanguage_Selector(), "[English]") != NULL);
    BootLanguage_Load();
    assert(!BootLanguage_IsJapanese());
    writeFails = true;
    assert(!BootLanguage_Toggle());
    assert(!BootLanguage_IsJapanese() && configValue == '0');
    writeFails = false;
    assert(BootLanguage_Toggle());
    assert(BootLanguage_IsJapanese() && configValue == '1');
    const u8 *glyph = Text_FindGlyph(0x753B);
    assert(glyph != NULL);
    drawString(true, 10, 10, 0x123456, "画");
    for (u32 y = 0; y < 8; ++y)
        for (u32 x = 0; x < 8; ++x)
        {
            u32 offset = GUARD + ((10 + x) * 240 + 239 - 10 - y) * 3;
            bool ink = (glyph[y] & (0x80u >> x)) != 0;
            assert(top[offset] == (ink ? 0x12 : 0));
            assert(top[offset + 1] == (ink ? 0x34 : 0));
            assert(top[offset + 2] == (ink ? 0x56 : 0));
        }
    assert(drawString(false, 304, 10, 0xFFFFFF, "画面設定") == 20);
    drawString(false, 319, 239, 0xFFFFFF, "画面設定");
    drawCharacter(true, 399, 239, 0xFFFFFF, (char)0xFF);
    drawCharacter(false, 0, 0, 0xFFFFFF, (char)0x82);
    checkGuards();
    memset(top + GUARD, 0, 400 * 240 * 3);
    memset(bottom + GUARD, 0, 320 * 240 * 3);
    drawString(true, 10, 10, TITLE_COLOR, TITLE);
    drawString(true, 10, 20, TITLE_COLOR, "十字: 選択  A: 切替  START: 保存して終了");
    drawString(true, 10, 30, TITLE_COLOR, BootLanguage_Selector());
    u32 y = 40;
    for (u32 i = 0; i < sizeof(multi) / sizeof(multi[0]); ++i)
    {
        drawString(true, 10, y, TEXT_COLOR, multi[i]);
        const char *cursor = multi[i];
        for (u32 column = 0; *cursor; ++column)
            if (Text_Decode(&cursor) == '(')
            {
                drawCharacter(true, 10 + (column + 1) * 8, y, TEXT_COLOR, 'x');
                break;
            }
        y += 10;
    }
    y += 5;
    for (u32 i = 0; i < sizeof(single) / sizeof(single[0]); ++i)
        y = drawString(true, 10, y, TEXT_COLOR, single[i]) + 10;
    drawString(false, 10, 10, TEXT_COLOR, description);
    FILE *file = fopen(OUTPUT, "wb");
    assert(file != NULL);
    fwrite(top + GUARD, 1, 400 * 240 * 3, file);
    fwrite(bottom + GUARD, 1, 320 * 240 * 3, file);
    fclose(file);
    checkGuards();
    puts("Boot Japanese/English settings, UTF-8 rendering, checkbox alignment, screen clipping: OK");
    return 0;
}
'''

for version in ('Luma3DS_v13_4',):
    newer = version == 'Luma3DS_v13_4'
    base = root / version / ('arm9/source' if newer else 'source')
    config = (base / 'config.c').read_text(encoding='utf-8')
    arrays = {}
    for name in ('multiOptionsText', 'singleOptionsText', 'optionsDescription'):
        english = array(config, name + 'English')
        japanese = array(config, name + 'Japanese')
        assert len(english) == len(japanese) > 0, (version, name, len(english), len(japanese))
        arrays[name] = japanese
        if name == 'multiOptionsText':
            assert all(en.count('( )') == jp.count('( )') for en, jp in zip(english, japanese))
        if name != 'optionsDescription':
            assert all(len(line) <= 48 for text in japanese for line in text.split('\n')), (version, name)
        else:
            assert all(sum(max(1, (len(line) + 37) // 38) for line in text.split('\n')) <= 20 for text in japanese), (version, 'description height')
    assert len(arrays['optionsDescription']) == len(arrays['multiOptionsText']) + len(arrays['singleOptionsText'])
    font = (base / 'japanese_font.h').read_text()
    codepoints = {int(cp, 16) for cp in re.findall(r'0x([0-9A-F]{4})', font.split('static const uint8_t')[0])}
    for text in sum(arrays.values(), []):
        assert {ord(c) for c in text if ord(c) >= 128} <= codepoints
    draw = (base / 'draw.c').read_text(encoding='utf-8')
    draw = draw[draw.index('void drawCharacter('):draw.index('u32 drawFormattedString(')]
    lang = re.sub(r'^#include[^\n]*\n', '', (base / 'boot_language.c').read_text(encoding='utf-8'), flags=re.M)
    data = '\n#include "font.h"\n'
    data += '\n#define TITLE_COLOR ' + ('0xFF9CC3' if newer else '0xFF9900')
    data += '\n#define TEXT_COLOR ' + ('0xFF9CC3' if newer else '0xFFFFFF')
    data += '\n#define TITLE ' + json.dumps('Luma3DS v13.4Y 設定' if newer else 'Luma3DS v9.1 設定', ensure_ascii=False)
    data += '\n#define OUTPUT "' + version + '_boot.rgb"\n'
    data += 'static const char *multi[] = { ' + ', '.join(json.dumps(s, ensure_ascii=False) for s in arrays['multiOptionsText']) + ' };\n'
    data += 'static const char *single[] = { ' + ', '.join(json.dumps(s, ensure_ascii=False) for s in arrays['singleOptionsText']) + ' };\n'
    data += 'static const char *description = ' + json.dumps(arrays['optionsDescription'][3], ensure_ascii=False) + ';\n'
    target = generated / (version + '_boot.c')
    target.write_text(prefix + data + lang + draw + main, encoding='utf-8', newline='\n')
    executable = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-I' + str(base), str(target), str(base / 'text.c'), '-o', str(executable)], check=True, env=env)
    subprocess.run([str(executable)], cwd=generated, check=True, env=env)
    rgb = (generated / (version + '_boot.rgb')).read_bytes()
    rows = bytearray()
    for y in range(240):
        line = bytearray()
        for x in range(720):
            if x < 400:
                offset = (x * 240 + 239 - y) * 3
            else:
                offset = 400 * 240 * 3 + ((x - 400) * 240 + 239 - y) * 3
            line.extend(rgb[offset:offset + 3][::-1] * 2)
        rows.extend((b'\0' + line) * 2)
    def chunk(kind, value):
        return struct.pack('>I', len(value)) + kind + value + struct.pack('>I', zlib.crc32(kind + value))
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 1440, 480, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')
    (generated / (version + '_boot.png')).write_bytes(png)
    print(version, 'boot options and descriptions verified')
