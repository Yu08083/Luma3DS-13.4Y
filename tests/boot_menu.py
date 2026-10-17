import os
import re
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated'
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "text.h"
typedef uint32_t u32;
typedef uint64_t u64;
typedef enum { FIRMWARE_SYSNAND, FIRMWARE_EMUNAND } FirmwareSource;
static struct { u32 config, multiConfig; } configData;
#define CONFIG(a) (((configData.config >> (a)) & 1) != 0)
#define MULTICONFIG(a) ((configData.multiConfig >> (2 * (a))) & 3)
#define ISN3DS true
#define HID_PAD 0
#define SCREEN_HEIGHT 240
#define SPACING_X 8
#define SPACING_Y 10
#define PIN_FILE "pin.bin"
#define CONFIG_TITLE "Luma3DS configuration"
#define NUMCONFIGURABLE 6
#define BRIGHTNESS 1
static bool isSdMode = true;
static const u32 bootType = 0;
static unsigned inputIndex, clearCount, saves;
static char configValue = '1';
static char selectedLabel[256], description[2048];
static u32 fileRead(void *dst, const char *path, u32 size)
{
    assert(strcmp(path, "lang.cfg") == 0 && size >= 2);
    ((char *)dst)[0] = configValue;
    return 1;
}
static bool fileWrite(const void *src, const char *path, u32 size)
{
    assert(strcmp(path, "lang.cfg") == 0 && size == 2);
    configValue = ((const char *)src)[0];
    return true;
}
static void initScreens(void) {}
static void clearScreens(bool alternate)
{
    assert(!alternate);
    ++clearCount;
    selectedLabel[0] = description[0] = 0;
}
static u32 drawString(bool top, u32 x, u32 y, u32 color, const char *text)
{
    assert(x < (top ? 400 : 320) && y < 240);
    if(top && y > 30 && color == COLOR_RED) strcpy(selectedLabel, text);
    if(!top && y == 10 && color != COLOR_BLACK) strcpy(description, text);
    return y;
}
static void drawCharacter(bool top, u32 x, u32 y, u32 color, char c)
{
    assert(top && x < 400 && y < 240 && c == 'x');
    (void)color;
}
static void drawFormattedString(bool top, u32 x, u32 y, u32 color, const char *fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    drawString(top, x, y, color, buffer);
}
static void updateBrightness(u32 index) { assert(index <= 3); }
static void writeConfig(bool options) { assert(options); ++saves; }
static void newPin(bool old, u32 mode) { (void)old; (void)mode; assert(false); }
static bool fileDelete(const char *path) { (void)path; assert(false); return false; }
static void error(const char *text) { (void)text; assert(false); }
static void wait(u64 amount) { (void)amount; }
'''

inputs = r'''
static u32 waitInput(bool menu)
{
    assert(menu && inputIndex < 8);
    switch(inputIndex++)
    {
        case 0:
            assert(BootLanguage_IsJapanese() && clearCount == 1);
            return BUTTON_A;
        case 1: return BUTTON_UP;
        case 2:
            assert(strcmp(selectedLabel, LAST_JP) == 0);
            return BUTTON_A;
        case 3: return BUTTON_X;
        case 4:
            assert(!BootLanguage_IsJapanese() && clearCount == 2);
            assert(strcmp(selectedLabel, LAST_EN) == 0);
            assert(strcmp(description, LAST_DESC_EN) == 0);
            return BUTTON_UP;
        case 5:
            assert(strcmp(selectedLabel, PREVIOUS_EN) == 0);
            return BUTTON_X;
        case 6:
            assert(BootLanguage_IsJapanese() && clearCount == 3);
            assert(strcmp(selectedLabel, PREVIOUS_JP) == 0);
            assert(strcmp(description, PREVIOUS_DESC_JP) == 0);
            return BUTTON_DOWN;
        case 7:
            assert(strcmp(selectedLabel, LAST_JP) == 0);
            return BUTTON_START;
    }
    assert(false);
    return 0;
}
'''
main = r'''
int main(void)
{
    configMenu(false, 0);
    assert(inputIndex == 8 && saves == 1);
    assert((configData.config & 1) == 1);
    assert(MULTICONFIG(5) == 1);
    assert(BootLanguage_IsJapanese() && configValue == '1');
    puts("Boot menu live language switch preserves selection and unsaved settings: OK");
    return 0;
}
'''

for version in ('Luma3DS_v13_4',):
    newer = version == 'Luma3DS_v13_4'
    base = root / version / ('arm9/source' if newer else 'source')
    config = (base / 'config.c').read_text(encoding='utf-8')
    defines = '\n'.join(line for line in (base / 'buttons.h').read_text(encoding='utf-8').splitlines()
                        if re.match(r'#define (BUTTON_|DPAD_BUTTONS|MENU_BUTTONS|PIN_BUTTONS)', line))
    defines += '\n' + '\n'.join(line for line in (base / 'draw.h').read_text(encoding='utf-8').splitlines()
                               if line.startswith('#define COLOR_'))
    defines += '\n#define PIN ' + ('3' if newer else '4') + '\n'
    constants = {}
    for name, index, key in [('multiOptionsText', 5, 'LAST'), ('multiOptionsText', 4, 'PREVIOUS'),
                             ('optionsDescription', 5, 'LAST_DESC'), ('optionsDescription', 4, 'PREVIOUS_DESC')]:
        for suffix, language in [('EN', 'English'), ('JP', 'Japanese')]:
            if (key, suffix) in {('PREVIOUS', 'EN'), ('PREVIOUS', 'JP'), ('LAST', 'EN'), ('LAST', 'JP'), ('LAST_DESC', 'EN'), ('PREVIOUS_DESC', 'JP')}:
                body = re.search(r'static const char \*' + name + language + r'\[\]\s*=\s*\{(.*?)\};', config, re.S)[1]
                entries = re.findall(r'((?:"(?:\\.|[^"\\])*"\s*)+)(?:,|$)', body)
                constants[key + '_' + suffix] = ' '.join(re.findall(r'"(?:\\.|[^"\\])*"', entries[index]))
    data = '\n'.join('#define ' + k + ' ' + v for k, v in constants.items()) + '\n'
    if newer:
        data += 'static void locateEmuNand(FirmwareSource *source, u32 *index, bool flag) { (void)source; (void)index; (void)flag; }\n'
        data += 'static void loadHomebrewFirm(u32 index) { (void)index; assert(false); }\n'
    else:
        data += 'static void locateEmuNand(FirmwareSource *source) { (void)source; }\n'
    language = re.sub(r'^#include[^\n]*\n', '', (base / 'boot_language.c').read_text(encoding='utf-8'), flags=re.M)
    target = generated / (version + '_boot_menu.c')
    target.write_text(defines + '\n' + prefix + data + language + inputs + config[config.index('void configMenu('):] + main, encoding='utf-8', newline='\n')
    executable = target.with_suffix('.exe' if os.name == 'nt' else '')
    subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-parentheses', '-O2', '-I' + str(base), str(target), str(base / 'text.c'), '-o', str(executable)], check=True, env=env)
    subprocess.run([str(executable)], check=True, env=env)
