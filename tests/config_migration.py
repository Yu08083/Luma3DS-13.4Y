import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


root = Path(__file__).resolve().parents[1]
arm9 = root / 'Luma3DS_v13_4/arm9/source'
source = (arm9 / 'config.c').read_text(encoding='utf-8')
body = source[source.index('#define MAKE_LUMA_VERSION_MCU'):source.index('void configMenu(')]
types = (arm9 / 'types.h').read_text(encoding='utf-8')
types = types[types.index('typedef struct ScreenFiltersCfgData'):types.index('} CfgDataMcu;') + len('} CfgDataMcu;')]
header = (arm9 / 'config.h').read_text(encoding='utf-8')
header = header[header.index('#define AUTOBOOT_DEFAULT_TWL_TID'):]
header = header.replace('void configMenu(bool oldPinStatus, u32 oldPinMode);', '')
template_path = root / 'Luma3DS_v13_4/arm9/data/config_template.ini'
template_bytes = template_path.read_bytes()
assert template_bytes == (root / 'Luma3DS_v13_4/sysmodules/rosalina/data/config_template.ini').read_bytes()
assert template_bytes.count(b'\0') == 1 and template_bytes.endswith(b'\0\n')
template = template_bytes.decode('utf-8').split('\0')[0]

legacy = '''[meta]
config_version_major = 3
config_version_minor = 13
[boot]
enable_game_patching = 1
pin_lock_num_digits = 6
app_launch_new_3ds_cpu = clock+l2
splash_position = after payloads
splash_duration_ms = 1500
autoboot_mode = 3ds
[rosalina]
hbldr_3dsx_titleid = 000400000d921e00
rosalina_menu_combo = L+R+X
plugin_loader_enabled = 1
ntp_tz_offset_min = 540
[screen_filters]
screen_filters_top_cct = 4200
screen_filters_bot_cct = 10000
screen_filters_top_color_curve_adj = 1
screen_filters_bot_color_curve_adj = 2
screen_filters_top_gamma = 1.25
screen_filters_bot_gamma = 8
screen_filters_top_contrast = 255
screen_filters_bot_contrast = 0.75
screen_filters_top_brightness = 1
screen_filters_bot_brightness = -1
screen_filters_top_invert = 1
screen_filters_bot_invert = 0
[autoboot]
autoboot_dsi_titleid = 0003000448424c41
autoboot_3ds_app_mem_type = 1
[misc]
force_audio_output = headphones
volume_slider_override = 37
use_dev_unitinfo = 0
enable_dsi_external_filter = 1
disable_arm11_exception_handlers = 0
enable_safe_firm_rosalina = 1
'''

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include "ini.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
#define VERSION_MAJOR 13
#define VERSION_MINOR 5
#define VERSION_BUILD 0
#define HBLDR_DEFAULT_3DSX_TID 0x000400000d921e00ULL
#define I2C_DEV_MCU 0
static char disk[0x4000];
static u8 mcu[200], mcuIndex, led;
static bool writeFails;
static unsigned upgrades, writes;
static char *config_strchrnul(const char *s, int c)
{
    const char *p = strchr(s, c);
    return (char *)(p ? p : s + strlen(s));
}
#define strchrnul config_strchrnul
static u32 fileRead(void *dst, const char *path, u32 size)
{
    assert(!strcmp(path, "config.ini"));
    size_t n = strlen(disk);
    if (n > size) n = size;
    memcpy(dst, disk, n);
    return (u32)n;
}
static bool fileWrite(const void *src, const char *path, u32 size)
{
    assert(!strcmp(path, "config.ini") && size < sizeof(disk));
    if (writeFails) return false;
    memcpy(disk, src, size);
    disk[size] = 0;
    writes++;
    return true;
}
static void I2C_writeReg(u8 dev, u8 reg, u8 value)
{
    assert(dev == 0);
    if (reg == 0x60) mcuIndex = value;
    else { assert(reg == 0x28); led = value; }
}
static void I2C_writeRegBuf(u8 dev, u8 reg, const void *src, u32 size)
{
    assert(dev == 0 && reg == 0x61 && mcuIndex == 192 && mcuIndex + size == 200);
    memcpy(mcu + mcuIndex, src, size);
}
static void I2C_readRegBuf(u8 dev, u8 reg, void *dst, u32 size)
{
    assert(dev == 0 && reg == 0x61 && mcuIndex == 192 && mcuIndex + size == 200);
    memcpy(dst, mcu + mcuIndex, size);
}
static bool doLumaUpgradeProcess(void) { upgrades++; return true; }
static void error(const char *message, ...) { (void)message; abort(); }
'''

main = r'''
static void reset(void)
{
    memset(&configData, 0, sizeof(configData));
    memset(&configDataMcu, 0, sizeof(configDataMcu));
    memset(&oldConfig, 0, sizeof(oldConfig));
    memset(mcu, 0xFF, sizeof(mcu));
    disk[0] = 0;
    led = 0xFF;
    writeFails = false;
    writes = upgrades = 0;
    hasIniParseError = false;
}
static void legacyMcu(bool valid)
{
    u16 version = MAKE_LUMA_VERSION_MCU(13, 4, 0);
    mcu[192] = 0xAA;
    mcu[193] = 0x55;
    mcu[194] = (u8)version;
    mcu[195] = (u8)(version >> 8);
    mcu[196] = 0xE7;
    mcu[197] = 0;
    mcu[198] = 0;
    u8 sum = 0;
    for (unsigned i = 194; i < 199; i++) sum += mcu[i];
    mcu[199] = (u8)~sum;
    if (!valid) mcu[199] ^= 1;
}
static void replace(const char *before, const char *after)
{
    char *at = strstr(disk, before);
    assert(at && strlen(before) == strlen(after));
    memcpy(at, after, strlen(after));
}
static void invalidLegacy(const char *before, const char *after)
{
    reset();
    legacyMcu(true);
    strcpy(disk, legacyIni);
    replace(before, after);
    assert(!readConfig() && configData.rosalinaMenuCombo != 1792 && writes == 0);
}
int main(void)
{
    reset();
    legacyMcu(true);
    strcpy(disk, legacyIni);
    assert(readConfig());
    assert(configData.formatVersionMajor == 3 && configData.formatVersionMinor == 16);
    assert(configData.rosalinaMenuCombo == ((1u << 9) | (1u << 8) | (1u << 10)));
    assert(configData.bootConfig == 0xE7 && CONFIG(PATCHGAMES));
    assert(MULTICONFIG(PIN) == 2 && MULTICONFIG(NEWCPU) == 3 && MULTICONFIG(AUTOBOOTMODE) == 1);
    for (unsigned i = 0; i < 3; i++) {
        assert(configData.topScreenFilter.gammaEnc[i] == 125000000LL);
        assert(configData.bottomScreenFilter.gammaEnc[i] == 400000000LL);
        assert(configData.topScreenFilter.contrastEnc[i] == 800000000LL);
        assert(configData.bottomScreenFilter.contrastEnc[i] == 75000000LL);
    }
    assert(configData.mcuConfigFlags == 11 && configData.volumeSliderOverride == 37);
    assert(configData.ledMasterBrightnessOverride == 255 && configData.volumeSliderOverrideStereoBalance == 0);
    assert(configData.topScreenFilter.invert && configData.ntpTzOffetMinutes == 540);
    assert(configData.autobootCtrAppmemtype == 1 && !configData.autobootCtrUseNs);
    assert(strstr(disk, "config_version_minor = 16"));
    assert(strstr(disk, "screen_filters_top_gamma_r = 1.25"));
    assert(strstr(disk, "screen_filters_top_contrast_b = 8"));
    assert(!strstr(disk, "force_audio_output") && !strstr(disk, "brightness ="));
    assert(mcu[192] == 11 && mcu[193] == 37 && mcu[194] == 0 && mcu[195] == 255 && mcu[198] == 0xE7);
    assert(writes == 1 && upgrades == 1);
    char currentIni[sizeof(disk)];
    strcpy(currentIni, disk);

    memset(&configData, 0, sizeof(configData));
    assert(readConfig() && configData.bootConfig == 0xE7 && configData.mcuConfigFlags == 11);
    assert(configData.rosalinaMenuCombo == 1792 && writes == 1 && upgrades == 1);
    configDataMcu.flags = MCUCFG_FLAGS_AUDIO_ROUTING_MASK | MCUCFG_FLAG_TEMPERATURE_IN_FAHRENHEIT;
    configDataMcu.volumeSliderOverrideStereoBalance = 127;
    configDataMcu.ledMasterBrightnessOverride = 0;
    writeConfigMcu();
    memset(&configData, 0, sizeof(configData));
    assert(readConfig() && configData.mcuConfigFlags == MCUCFG_FLAG_TEMPERATURE_IN_FAHRENHEIT);
    assert(configData.volumeSliderOverrideStereoBalance == 0 && led == 0);
    strcpy(disk, currentIni);
    replace("config_version_minor = 16", "config_version_minor = 15");
    memset(&configData, 0, sizeof(configData));
    assert(!readConfig() && configData.rosalinaMenuCombo != 1792);
    strcpy(disk, currentIni);
    replace("config_version_major = 3", "config_version_major = 4");
    memset(&configData, 0, sizeof(configData));
    assert(!readConfig() && configData.rosalinaMenuCombo != 1792);
    strcpy(disk, currentIni);
    replace("gamma_r = 1.25", "gamma_r = 9.25");
    memset(&configData, 0, sizeof(configData));
    assert(!readConfig() && configData.rosalinaMenuCombo != 1792);

    reset();
    legacyMcu(false);
    strcpy(disk, legacyIni);
    assert(readConfig() && configData.bootConfig == 0 && configData.rosalinaMenuCombo == 1792 && writes == 1);
    reset();
    assert(!readConfig() && configData.formatVersionMinor == 16);
    assert(configData.rosalinaMenuCombo == ((1u << 9) | (1u << 7) | (1u << 2)));
    assert(configData.volumeSliderOverride == 255 && configData.ledMasterBrightnessOverride == 255);

    reset();
    configData.formatVersionMajor = 3;
    configData.formatVersionMinor = 13;
    assert(configIniHandler(&configData, "misc", "force_audio_output", "speakers", 1) == 1);
    assert(configData.mcuConfigFlags == 4);
    assert(configIniHandler(&configData, "misc", "force_audio_output", "off", 1) == 1);
    assert(configData.mcuConfigFlags == 0);
    assert(configIniHandler(&configData, "misc", "volume_slider_override", "-1", 1) == 1);
    assert(configData.volumeSliderOverride == 255);
    configData.formatVersionMinor = 16;
    assert(configIniHandler(&configData, "misc", "force_audio_output", "off", 1) == 0 && hasIniParseError);

    invalidLegacy("gamma = 1.25", "gamma = 9.25");
    invalidLegacy("contrast = 255", "contrast = 256");
    invalidLegacy("brightness = -1", "brightness = -2");
    invalidLegacy("config_version_minor = 13", "config_version_minor = 15");
    invalidLegacy("config_version_major = 3", "config_version_major = 4");
    reset();
    legacyMcu(true);
    strcpy(disk, legacyIni);
    writeFails = true;
    assert(!readConfig() && configData.rosalinaMenuCombo == 1792 && writes == 0);
    assert(configData.formatVersionMinor == 16);
    puts("Config migration: MCU checksums/defaults, legacy settings and combo, RGB ranges, audio routing, strict versions, and failed writes passed");
}
'''

compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = dict(os.environ)
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='luma-config-migration-') as temp:
    directory = Path(temp)
    for name in ('ini.c', 'ini.h'):
        shutil.copyfile(arm9 / name, directory / name)
    generated = directory / 'config_migration.c'
    generated.write_text(prefix + types + '\n' + header + '\nstatic const char config_template_ini[]=' +
                         json.dumps(template) + ';\nstatic const char legacyIni[]=' + json.dumps(legacy) +
                         ';\n' + body + main, encoding='utf-8')
    binary = directory / ('config_migration.exe' if os.name == 'nt' else 'config_migration')
    subprocess.run([compiler, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-format',
                    '-Wno-unused-function', '-Wno-unused-variable', '-DINI_HANDLER_LINENO=1',
                    str(generated), str(directory / 'ini.c'), '-o', str(binary)], check=True, env=env)
    subprocess.run([str(binary)], check=True, env=env)
