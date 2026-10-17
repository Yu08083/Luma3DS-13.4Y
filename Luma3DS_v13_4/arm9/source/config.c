/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2020 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#define _GNU_SOURCE

#include <assert.h>
#include <strings.h>
#include "config.h"
#include "memory.h"
#include "fs.h"
#include "utils.h"
#include "screen.h"
#include "draw.h"
#include "text.h"
#include "boot_language.h"
#include "emunand.h"
#include "buttons.h"
#include "pin.h"
#include "i2c.h"
#include "ini.h"
#include "firm.h"

#include "config_template_ini.h"

#define MAKE_LUMA_VERSION_MCU(major, minor, build) (u16)(((major) & 0xFF) << 8 | ((minor) & 0x1F) << 5 | ((build) & 7))

#define FLOAT_CONV_MULT 100000000ll
#define FLOAT_CONV_PRECISION 8u

CfgData configData;
ConfigurationStatus needConfig;
static CfgData oldConfig;

static CfgDataMcu configDataMcu;
static_assert(sizeof(CfgDataMcu) > 0, "wrong data size");




static const char *singleOptionIniNamesBoot[] = {
    "autoboot_emunand",
    "enable_external_firm_and_modules",
    "enable_game_patching",
    "app_syscore_threads_on_core_2",
    "show_system_settings_string",
    "show_gba_boot_screen",
};

static const char *singleOptionIniNamesMisc[] = {
    "use_dev_unitinfo",
    "enable_dsi_external_filter",
    "disable_arm11_exception_handlers",
    "enable_safe_firm_rosalina",
};

static const char *keyNames[] = {
    "A", "B", "Select", "Start", "Right", "Left", "Up", "Down", "R", "L", "X", "Y",
    "?", "?",
    "ZL", "ZR",
    "?", "?", "?", "?",
    "Touch",
    "?", "?", "?",
    "CStick Right", "CStick Left", "CStick Up", "CStick Down",
    "CPad Right", "CPad Left", "CPad Up", "CPad Down",
};

static int parseBoolOption(bool *out, const char *val)
{
    *out = false;
    if (strlen(val) != 1) {
        return -1;
    }

    if (val[0] == '0') {
        return 0;
    } else if (val[0] == '1') {
        *out = true;
        return 0;
    } else {
        return -1;
    }
}

static int parseDecIntOptionImpl(s64 *out, const char *val, size_t numDigits, s64 minval, s64 maxval)
{
    *out = 0;
    s64 res = 0;
    size_t i = 0;

    s64 sign = 1;
    if (numDigits >= 2) {
        if (val[0] == '+') {
            ++i;
        } else if (val[0] == '-') {
            sign = -1;
            ++i;
        }
    }

    for (; i < numDigits; i++) {
        u64 n = (u64)(val[i] - '0');
        if (n > 9) {
            return -1;
        }

        res = 10*res + n;
    }

    res *= sign;
    if (res <= maxval && res >= minval) {
        *out = res;
        return 0;
    } else {
        return -1;
    }
}

static int parseDecIntOption(s64 *out, const char *val, s64 minval, s64 maxval)
{
    return parseDecIntOptionImpl(out, val, strlen(val), minval, maxval);
}

static int parseDecFloatOption(s64 *out, const char *val, s64 minval, s64 maxval)
{
    s64 sign = 1;

    switch (val[0]) {
        case '\0':
            return -1;
        case '+':
            ++val;
            break;
        case '-':
            sign = -1;
            ++val;
            break;
        default:
            break;
    }


    if (val[0] == '\0') {
        return -1;
    }

    char *point = strchrnul(val, '.');


    s64 intPart = 0;
    s64 fracPart = 0;
    int rc = 0;

    if (point == val) {

        if (val[1] == '\0')
            return -1;
    }
    else {
        rc = parseDecIntOptionImpl(&intPart, val, point - val, INT64_MIN, INT64_MAX);
    }

    if (rc != 0) {
        return -1;
    }

    s64 intPartAbs = sign == -1 ? -intPart : intPart;
    s64 res = 0;
    bool of = __builtin_mul_overflow(intPartAbs, FLOAT_CONV_MULT, &res);

    if (of) {
        return -1;
    }

    s64 mul = FLOAT_CONV_MULT / 10;


    if (point[0] != '\0' && point[1] != '\0') {
        for (char *pos = point + 1; *pos != '\0' && mul > 0; pos++) {
            if (*pos < '0' || *pos > '9') {
                return -1;
            }

            res += (*pos - '0') * mul;
            mul /= 10;
        }
    }


    res = sign * (res + fracPart);

    if (res <= maxval && res >= minval && !of) {
        *out = res;
        return 0;
    } else {
        return -1;
    }
}

static int parseHexIntOption(u64 *out, const char *val, u64 minval, u64 maxval)
{
    *out = 0;
    size_t numDigits = strlen(val);
    u64 res = 0;

    for (size_t i = 0; i < numDigits; i++) {
        char c = val[i];
        if ((u64)(c - '0') <= 9) {
            res = 16*res + (u64)(c - '0');
        } else if ((u64)(c - 'a') <= 5) {
            res = 16*res + (u64)(c - 'a' + 10);
        } else if ((u64)(c - 'A') <= 5) {
            res = 16*res + (u64)(c - 'A' + 10);
        } else {
            return -1;
        }
    }

    if (res <= maxval && res >= minval) {
        *out = res;
        return 0;
    } else {
        return -1;
    }
}

static int parseKeyComboOption(u32 *out, const char *val)
{
    const char *startpos = val;
    const char *endpos;

    *out = 0;
    u32 keyCombo = 0;
    do {

        char name[17];
        endpos = strchr(startpos, '+');
        size_t n = endpos == NULL ? 16 : endpos - startpos;
        n = n > 16 ? 16 : n;
        strncpy(name, startpos, n);
        name[n] = '\0';

        if (strcmp(name, "?") == 0) {

            return -1;
        }

        bool found = false;
        for (size_t i = 0; i < sizeof(keyNames)/sizeof(keyNames[0]); i++) {
            if (strcasecmp(keyNames[i], name) == 0) {
                found = true;
                keyCombo |= 1u << i;
            }
        }

        if (!found) {
            return -1;
        }

        if (endpos != NULL) {
            startpos = endpos + 1;
        }
    } while(endpos != NULL && *startpos != '\0');

    if (*startpos == '\0') {

        return -1;
    } else {
        *out = keyCombo;
        return 0;
    }
}

static void menuComboToString(char *out, u32 combo)
{
    char *outOrig = out;
    out[0] = 0;
    for(int i = 31; i >= 0; i--)
    {
        if(combo & (1 << i))
        {
            strcpy(out, keyNames[i]);
            out += strlen(keyNames[i]);
            *out++ = '+';
        }
    }

    if (out != outOrig)
        out[-1] = 0;
}

static int encodedFloatToString(char *out, s64 val)
{
    s64 sign = val >= 0 ? 1 : -1;

    s64 intPart = (sign * val) / FLOAT_CONV_MULT;
    s64 fracPart = (sign * val) % FLOAT_CONV_MULT;

    while (fracPart % 10 != 0) {

        fracPart /= 10;
    }

    int n = sprintf(out, "%lld", sign * intPart);
    if (fracPart != 0) {
        n += sprintf(out + n, ".%0*lld", (int)FLOAT_CONV_PRECISION, fracPart);


        int n2 = n - 1;
        while (out[n2] == '0') {
            out[n2--] = '\0';
        }

        n = n2;
    }

    return n;
}

static bool hasIniParseError = false;
static int iniParseErrorLine = 0;

#define CHECK_PARSE_OPTION(res) do { if((res) < 0) { hasIniParseError = true; iniParseErrorLine = lineno; return 0; } } while(false)

static int configIniHandler(void* user, const char* section, const char* name, const char* value, int lineno)
{
    CfgData *cfg = (CfgData *)user;
    if (strcmp(section, "meta") == 0) {
        if (strcmp(name, "config_version_major") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 0, 0xFFFF));
            cfg->formatVersionMajor = (u16)opt;
            return 1;
        } else if (strcmp(name, "config_version_minor") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 0, 0xFFFF));
            cfg->formatVersionMinor = (u16)opt;
            return 1;
        } else {
            CHECK_PARSE_OPTION(-1);
        }
    } else if (strcmp(section, "boot") == 0) {

        for (size_t i = 0; i < sizeof(singleOptionIniNamesBoot)/sizeof(singleOptionIniNamesBoot[0]); i++) {
            if (strcmp(name, singleOptionIniNamesBoot[i]) == 0) {
                bool opt;
                CHECK_PARSE_OPTION(parseBoolOption(&opt, value));
                cfg->config |= (u32)opt << i;
                return 1;
            }
        }



        if (strcmp(name, "default_emunand_number") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 1, 4));
            cfg->multiConfig |= (opt - 1) << (2 * (u32)DEFAULTEMU);
            return 1;
        } else if (strcmp(name, "brightness_level") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 1, 4));
            cfg->multiConfig |= (4 - opt) << (2 * (u32)BRIGHTNESS);
            return 1;
        } else if (strcmp(name, "splash_position") == 0) {
            if (strcasecmp(value, "off") == 0) {
                cfg->multiConfig |= 0 << (2 * (u32)SPLASH);
                return 1;
            } else if (strcasecmp(value, "before payloads") == 0) {
                cfg->multiConfig |= 1 << (2 * (u32)SPLASH);
                return 1;
            } else if (strcasecmp(value, "after payloads") == 0) {
                cfg->multiConfig |= 2 << (2 * (u32)SPLASH);
                return 1;
            } else {
                CHECK_PARSE_OPTION(-1);
            }
        } else if (strcmp(name, "splash_duration_ms") == 0) {

            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 0, 0xFFFFFFFFu));
            cfg->splashDurationMsec = (u32)opt;
            return 1;
        }
        else if (strcmp(name, "pin_lock_num_digits") == 0) {
            s64 opt;
            u32 encodedOpt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 0, 8));

            switch (opt) {
                case 0: encodedOpt = 0; break;
                case 4: encodedOpt = 1; break;
                case 6: encodedOpt = 2; break;
                case 8: encodedOpt = 3; break;
                default: {
                    CHECK_PARSE_OPTION(-1);
                }
            }
            cfg->multiConfig |= encodedOpt << (2 * (u32)PIN);
            return 1;
        } else if (strcmp(name, "app_launch_new_3ds_cpu") == 0) {
            if (strcasecmp(value, "off") == 0) {
                cfg->multiConfig |= 0 << (2 * (u32)NEWCPU);
                return 1;
            } else if (strcasecmp(value, "clock") == 0) {
                cfg->multiConfig |= 1 << (2 * (u32)NEWCPU);
                return 1;
            } else if (strcasecmp(value, "l2") == 0) {
                cfg->multiConfig |= 2 << (2 * (u32)NEWCPU);
                return 1;
            } else if (strcasecmp(value, "clock+l2") == 0) {
                cfg->multiConfig |= 3 << (2 * (u32)NEWCPU);
                return 1;
            } else {
                CHECK_PARSE_OPTION(-1);
            }
        } else if (strcmp(name, "autoboot_mode") == 0) {
            if (strcasecmp(value, "off") == 0) {
                cfg->multiConfig |= 0 << (2 * (u32)AUTOBOOTMODE);
                return 1;
            } else if (strcasecmp(value, "3ds") == 0) {
                cfg->multiConfig |= 1 << (2 * (u32)AUTOBOOTMODE);
                return 1;
            } else if (strcasecmp(value, "dsi") == 0) {
                cfg->multiConfig |= 2 << (2 * (u32)AUTOBOOTMODE);
                return 1;
            } else {
                CHECK_PARSE_OPTION(-1);
            }
        } else {
            CHECK_PARSE_OPTION(-1);
        }
    } else if (strcmp(section, "rosalina") == 0) {

        if (strcmp(name, "hbldr_3dsx_titleid") == 0) {
            u64 opt;
            CHECK_PARSE_OPTION(parseHexIntOption(&opt, value, 0, 0xFFFFFFFFFFFFFFFFull));
            cfg->hbldr3dsxTitleId = opt;
            return 1;
        } else if (strcmp(name, "rosalina_menu_combo") == 0) {
            u32 opt;
            CHECK_PARSE_OPTION(parseKeyComboOption(&opt, value));
            cfg->rosalinaMenuCombo = opt;
            return 1;
        } else if (strcmp(name, "plugin_loader_enabled") == 0) {
            bool opt;
            CHECK_PARSE_OPTION(parseBoolOption(&opt, value));
            cfg->pluginLoaderFlags = opt ? cfg->pluginLoaderFlags | 1 : cfg->pluginLoaderFlags & ~1;
            return 1;
        } else if (strcmp(name, "ntp_tz_offset_min") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, -779, 899));
            cfg->ntpTzOffetMinutes = (s16)opt;
            return 1;
        } else {
            CHECK_PARSE_OPTION(-1);
        }
    } else if (strcmp(section, "screen_filters") == 0) {
        if (strcmp(name, "screen_filters_top_cct") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 1000, 25100));
            cfg->topScreenFilter.cct = (u32)opt;
            return 1;
        } else if (strcmp(name, "screen_filters_top_gamma") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecFloatOption(&opt, value, 0, 8 * FLOAT_CONV_MULT));
            cfg->topScreenFilter.gammaEnc = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_top_contrast") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecFloatOption(&opt, value, 0, 255 * FLOAT_CONV_MULT));
            cfg->topScreenFilter.contrastEnc = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_top_brightness") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecFloatOption(&opt, value, -1 * FLOAT_CONV_MULT, 1 * FLOAT_CONV_MULT));
            cfg->topScreenFilter.brightnessEnc = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_top_invert") == 0) {
            bool opt;
            CHECK_PARSE_OPTION(parseBoolOption(&opt, value));
            cfg->topScreenFilter.invert = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_top_color_curve_adj") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 0, 2));
            cfg->topScreenFilter.colorCurveCorrection = (u8)opt;
            return 1;
        } else if (strcmp(name, "screen_filters_bot_cct") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 1000, 25100));
            cfg->bottomScreenFilter.cct = (u32)opt;
            return 1;
        } else if (strcmp(name, "screen_filters_bot_gamma") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecFloatOption(&opt, value, 0, 8 * FLOAT_CONV_MULT));
            cfg->bottomScreenFilter.gammaEnc = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_bot_contrast") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecFloatOption(&opt, value, 0, 255 * FLOAT_CONV_MULT));
            cfg->bottomScreenFilter.contrastEnc = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_bot_brightness") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecFloatOption(&opt, value, -1 * FLOAT_CONV_MULT, 1 * FLOAT_CONV_MULT));
            cfg->bottomScreenFilter.brightnessEnc = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_bot_invert") == 0) {
            bool opt;
            CHECK_PARSE_OPTION(parseBoolOption(&opt, value));
            cfg->bottomScreenFilter.invert = opt;
            return 1;
        } else if (strcmp(name, "screen_filters_bot_color_curve_adj") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 0, 2));
            cfg->bottomScreenFilter.colorCurveCorrection = (u8)opt;
            return 1;
        } else {
            CHECK_PARSE_OPTION(-1);
        }
    } else if (strcmp(section, "autoboot") == 0) {
        if (strcmp(name, "autoboot_dsi_titleid") == 0) {
            u64 opt;
            CHECK_PARSE_OPTION(parseHexIntOption(&opt, value, 0, 0xFFFFFFFFFFFFFFFFull));
            cfg->autobootTwlTitleId = opt;
            return 1;
        } else if (strcmp(name, "autoboot_3ds_app_mem_type") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, 0, 4));
            cfg->autobootCtrAppmemtype = (u8)opt;
            return 1;
        } else {
            CHECK_PARSE_OPTION(-1);
        }
    } else if (strcmp(section, "misc") == 0) {
        for (size_t i = 0; i < sizeof(singleOptionIniNamesMisc)/sizeof(singleOptionIniNamesMisc[0]); i++) {
            if (strcmp(name, singleOptionIniNamesMisc[i]) == 0) {
                bool opt;
                CHECK_PARSE_OPTION(parseBoolOption(&opt, value));
                cfg->config |= (u32)opt << (i + (u32)PATCHUNITINFO);
                return 1;
            }
        }

        if (strcmp(name, "force_audio_output") == 0) {
            if (strcasecmp(value, "off") == 0) {
                cfg->multiConfig |= 0 << (2 * (u32)FORCEAUDIOOUTPUT);
                return 1;
            } else if (strcasecmp(value, "headphones") == 0) {
                cfg->multiConfig |= 1 << (2 * (u32)FORCEAUDIOOUTPUT);
                return 1;
            } else if (strcasecmp(value, "speakers") == 0) {
                cfg->multiConfig |= 2 << (2 * (u32)FORCEAUDIOOUTPUT);
                return 1;
            } else {
                CHECK_PARSE_OPTION(-1);
            }
        } else if (strcmp(name, "volume_slider_override") == 0) {
            s64 opt;
            CHECK_PARSE_OPTION(parseDecIntOption(&opt, value, -1, 100));
            cfg->volumeSliderOverride = (s8)opt;
            return 1;
        } else {
            CHECK_PARSE_OPTION(-1);
        }
    } else {
        CHECK_PARSE_OPTION(-1);
    }
}

static size_t saveLumaIniConfigToStr(char *out)
{
    const CfgData *cfg = &configData;

    char rosalinaMenuComboStr[128];

    const char *splashPosStr;
    const char *n3dsCpuStr;
    const char *autobootModeStr;
    const char *forceAudioOutputStr;

    switch (MULTICONFIG(SPLASH)) {
        default: case 0: splashPosStr = "off"; break;
        case 1: splashPosStr = "before payloads"; break;
        case 2: splashPosStr = "after payloads"; break;
    }

    switch (MULTICONFIG(NEWCPU)) {
        default: case 0: n3dsCpuStr = "off"; break;
        case 1: n3dsCpuStr = "clock"; break;
        case 2: n3dsCpuStr = "l2"; break;
        case 3: n3dsCpuStr = "clock+l2"; break;
    }

    switch (MULTICONFIG(AUTOBOOTMODE)) {
        default: case 0: autobootModeStr = "off"; break;
        case 1: autobootModeStr = "3ds"; break;
        case 2: autobootModeStr = "dsi"; break;
    }

    switch (MULTICONFIG(FORCEAUDIOOUTPUT)) {
        default: case 0: forceAudioOutputStr = "off"; break;
        case 1: forceAudioOutputStr = "headphones"; break;
        case 2: forceAudioOutputStr = "speakers"; break;
    }

    menuComboToString(rosalinaMenuComboStr, cfg->rosalinaMenuCombo);

    static const int pinOptionToDigits[] = { 0, 4, 6, 8 };
    int pinNumDigits = pinOptionToDigits[MULTICONFIG(PIN)];

    char topScreenFilterGammaStr[32];
    char topScreenFilterContrastStr[32];
    char topScreenFilterBrightnessStr[32];
    encodedFloatToString(topScreenFilterGammaStr, cfg->topScreenFilter.gammaEnc);
    encodedFloatToString(topScreenFilterContrastStr, cfg->topScreenFilter.contrastEnc);
    encodedFloatToString(topScreenFilterBrightnessStr, cfg->topScreenFilter.brightnessEnc);

    char bottomScreenFilterGammaStr[32];
    char bottomScreenFilterContrastStr[32];
    char bottomScreenFilterBrightnessStr[32];
    encodedFloatToString(bottomScreenFilterGammaStr, cfg->bottomScreenFilter.gammaEnc);
    encodedFloatToString(bottomScreenFilterContrastStr, cfg->bottomScreenFilter.contrastEnc);
    encodedFloatToString(bottomScreenFilterBrightnessStr, cfg->bottomScreenFilter.brightnessEnc);

    int n = sprintf(
        out, (const char *)config_template_ini,

        (int)CONFIG_VERSIONMAJOR, (int)CONFIG_VERSIONMINOR,
        (int)CONFIG(AUTOBOOTEMU), (int)CONFIG(LOADEXTFIRMSANDMODULES),
        (int)CONFIG(PATCHGAMES), (int)CONFIG(REDIRECTAPPTHREADS),
        (int)CONFIG(PATCHVERSTRING), (int)CONFIG(SHOWGBABOOT),

        1 + (int)MULTICONFIG(DEFAULTEMU), 4 - (int)MULTICONFIG(BRIGHTNESS),
        splashPosStr, (unsigned int)cfg->splashDurationMsec,
        pinNumDigits, n3dsCpuStr,
        autobootModeStr,

        cfg->hbldr3dsxTitleId, rosalinaMenuComboStr, (int)(cfg->pluginLoaderFlags & 1),
        (int)cfg->ntpTzOffetMinutes,

        (int)cfg->topScreenFilter.cct, (int)cfg->bottomScreenFilter.cct,
        (int)cfg->topScreenFilter.colorCurveCorrection, (int)cfg->bottomScreenFilter.colorCurveCorrection,
        topScreenFilterGammaStr, bottomScreenFilterGammaStr,
        topScreenFilterContrastStr, bottomScreenFilterContrastStr,
        topScreenFilterBrightnessStr, bottomScreenFilterBrightnessStr,
        (int)cfg->topScreenFilter.invert, (int)cfg->bottomScreenFilter.invert,

        cfg->autobootTwlTitleId, (int)cfg->autobootCtrAppmemtype,

        forceAudioOutputStr,
        cfg->volumeSliderOverride,

        (int)CONFIG(PATCHUNITINFO), (int)CONFIG(ENABLEDSIEXTFILTER),
        (int)CONFIG(DISABLEARM11EXCHANDLERS), (int)CONFIG(ENABLESAFEFIRMROSALINA)
    );

    return n < 0 ? 0 : (size_t)n;
}

static char tmpIniBuffer[0x2000 + 0x400];

static bool readLumaIniConfig(void)
{
    u32 rd = fileRead(tmpIniBuffer, "config.ini", sizeof(tmpIniBuffer) - 1);
    if (rd == 0) return false;

    tmpIniBuffer[rd] = '\0';

    return ini_parse_string(tmpIniBuffer, &configIniHandler, &configData) >= 0 && !hasIniParseError;
}

static bool writeLumaIniConfig(void)
{
    size_t n = saveLumaIniConfigToStr(tmpIniBuffer);


    if (n + 1 >= sizeof(tmpIniBuffer)) {
        error("Configuration data buffer overflow, please report this issue");
        __builtin_unreachable();
    }

    return n != 0 && fileWrite(tmpIniBuffer, "config.ini", n);
}



static void writeConfigMcu(void)
{
    u8 data[sizeof(CfgDataMcu)];


    configDataMcu.lumaVersion = MAKE_LUMA_VERSION_MCU(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);


    configDataMcu.bootCfg = configData.bootConfig;

    memcpy(data, &configDataMcu, sizeof(CfgDataMcu));


    u8 checksum = 0;
    for (u32 i = 0; i < sizeof(CfgDataMcu) - 1; i++)
        checksum += data[i];
    checksum = ~checksum;
    data[sizeof(CfgDataMcu) - 1] = checksum;
    configDataMcu.checksum = checksum;

    I2C_writeReg(I2C_DEV_MCU, 0x60, 200 - sizeof(CfgDataMcu));
    I2C_writeRegBuf(I2C_DEV_MCU, 0x61, data, sizeof(CfgDataMcu));
}

static bool readConfigMcu(void)
{
    u8 data[sizeof(CfgDataMcu)];
    u16 curVer = MAKE_LUMA_VERSION_MCU(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);


    I2C_writeReg(I2C_DEV_MCU, 0x60, 200 - sizeof(CfgDataMcu));
    I2C_readRegBuf(I2C_DEV_MCU, 0x61, data, sizeof(CfgDataMcu));
    memcpy(&configDataMcu, data, sizeof(CfgDataMcu));

    u8 checksum = 0;
    for (u32 i = 0; i < sizeof(CfgDataMcu) - 1; i++)
        checksum += data[i];
    checksum = ~checksum;

    if (checksum != configDataMcu.checksum || configDataMcu.lumaVersion < MAKE_LUMA_VERSION_MCU(10, 3, 0))
    {

        memset(&configDataMcu, 0, sizeof(CfgDataMcu));
        configData.bootConfig = 0;

        doLumaUpgradeProcess();
        writeConfigMcu();

        return false;
    }

    if (configDataMcu.lumaVersion < curVer)
    {

        doLumaUpgradeProcess();
        writeConfigMcu();
    }

    return true;
}

bool readConfig(void)
{
    bool retMcu, ret;

    retMcu = readConfigMcu();
    ret = readLumaIniConfig();
    if(!retMcu || !ret ||
       configData.formatVersionMajor != CONFIG_VERSIONMAJOR ||
       configData.formatVersionMinor != CONFIG_VERSIONMINOR)
    {
        memset(&configData, 0, sizeof(CfgData));
        configData.formatVersionMajor = CONFIG_VERSIONMAJOR;
        configData.formatVersionMinor = CONFIG_VERSIONMINOR;
        configData.config |= 1u << PATCHVERSTRING;
        configData.splashDurationMsec = 3000;
        configData.volumeSliderOverride = -1;
        configData.hbldr3dsxTitleId = HBLDR_DEFAULT_3DSX_TID;
        configData.rosalinaMenuCombo = 1u << 9 | 1u << 7 | 1u << 2;
        configData.topScreenFilter.cct = 6500;
        configData.topScreenFilter.gammaEnc = 1 * FLOAT_CONV_MULT;
        configData.topScreenFilter.contrastEnc = 1 * FLOAT_CONV_MULT;
        configData.bottomScreenFilter = configData.topScreenFilter;
        configData.autobootTwlTitleId = AUTOBOOT_DEFAULT_TWL_TID;
        ret = false;
    }
    else
        ret = true;

    configData.bootConfig = configDataMcu.bootCfg;
    oldConfig = configData;

    return ret;
}

void writeConfig(bool isConfigOptions)
{
    bool updateMcu, updateIni;

    if (needConfig == CREATE_CONFIGURATION)
    {
        updateMcu = !isConfigOptions;
        updateIni = isConfigOptions;
        needConfig = MODIFY_CONFIGURATION;
    }
    else
    {
        updateMcu = !isConfigOptions && configData.bootConfig != oldConfig.bootConfig;
        updateIni = isConfigOptions && (configData.config != oldConfig.config || configData.multiConfig != oldConfig.multiConfig);
    }

    if (updateMcu)
        writeConfigMcu();

    if(updateIni && !writeLumaIniConfig())
        error("Error writing the configuration file");
}

void configMenu(bool oldPinStatus, u32 oldPinMode)
{
    BootLanguage_Load();
    static const char *multiOptionsTextEnglish[]  = { "Default EmuNAND: 1( ) 2( ) 3( ) 4( )",
                                               "Screen brightness: 4( ) 3( ) 2( ) 1( )",
                                               "Splash: Off( ) Before( ) After( ) payloads",
                                               "PIN lock: Off( ) 4( ) 6( ) 8( ) digits",
                                               "New 3DS CPU: Off( ) Clock( ) L2( ) Clock+L2( )",
                                               "Hbmenu autoboot: Off( ) 3DS( ) DSi( )",
                                             };
    static const char *multiOptionsTextJapanese[] = {
        "標準EmuNAND: 1( ) 2( ) 3( ) 4( )",
        "画面の明るさ: 4( ) 3( ) 2( ) 1( )",
        "起動画面: 無効( ) ペイロード前( ) 後( )",
        "PINロック: 無効( ) 4( ) 6( ) 8( ) 桁",
        "New 3DS CPU: 無効( ) Clock( ) L2( ) Clock+L2( )",
        "HB自動起動: 無効( ) 3DS( ) DSi( )"
    };
    const char *const *multiOptionsText = BootLanguage_IsJapanese() ? multiOptionsTextJapanese : multiOptionsTextEnglish;

    static const char *singleOptionsTextEnglish[] = { "( ) Autoboot EmuNAND",
                                               "( ) Enable loading external FIRMs and modules",
                                               "( ) Enable game patching",
                                               "( ) Redirect app. syscore threads to core2",
                                               "( ) Show NAND or user string in System Settings",
                                               "( ) Show GBA boot screen in patched AGB_FIRM",


                                               "\nBoot chainloader",
                                               "Save and exit"
                                             };
    static const char *singleOptionsTextJapanese[] = {
        "( ) EmuNANDを自動起動",
        "( ) 外部FIRMとシステムモジュールを読み込む",
        "( ) ゲームパッチを有効化",
        "( ) アプリのcore1用スレッドをcore2へ移す",
        "( ) 本体設定にNAND名・独自文字列を表示",
        "( ) GBAソフトの起動画面を表示",
        "\nチェーンローダーを起動",
        "保存して終了"
    };
    const char *const *singleOptionsText = BootLanguage_IsJapanese() ? singleOptionsTextJapanese : singleOptionsTextEnglish;

    static const char *optionsDescriptionEnglish[]  = { "Select the default EmuNAND.\n\n"
                                                 "It will be booted when no directional\n"
                                                 "pad buttons are pressed (Up/Right/Down\n"
                                                 "/Left equal EmuNANDs 1/2/3/4).",

                                                 "Select the screen brightness.",

                                                 "Enable splash screen support.\n\n"
                                                 "\t* 'Before payloads' displays it\n"
                                                 "before booting payloads\n"
                                                 "(intended for splashes that display\n"
                                                 "button hints).\n\n"
                                                 "\t* 'After payloads' displays it\n"
                                                 "afterwards.\n\n"
                                                 "Edit the duration in config.ini (3s\n"
                                                 "default).",

                                                 "Activate a PIN lock.\n\n"
                                                 "The PIN will be asked each time\n"
                                                 "Luma3DS boots.\n\n"
                                                 "4, 6 or 8 digits can be selected.\n\n"
                                                 "The ABXY buttons and the directional\n"
                                                 "pad buttons can be used as keys.\n\n"
                                                 "A message can also be displayed\n"
                                                 "(refer to the wiki for instructions).",

                                                 "Select the New 3DS CPU mode.\n\n"
                                                 "This won't apply to\n"
                                                 "New 3DS exclusive/enhanced games.\n\n"
                                                 "'Clock+L2' can cause issues with some\n"
                                                 "games.",

                                                 "Enable autobooting into homebrew menu,\n"
                                                 "either into 3DS or DSi mode.\n\n"
                                                 "Autobooting into a gamecard title is\n"
                                                 "not supported.\n\n"
                                                 "Refer to the \"autoboot\" section in the\n"
                                                 "configuration file to configure\n"
                                                 "this feature.",

                                                 "If enabled, an EmuNAND\n"
                                                 "will be launched on boot.\n\n"
                                                 "Otherwise, SysNAND will.\n\n"
                                                 "Hold L on boot to switch NAND.\n\n"
                                                 "To use a different EmuNAND from the\n"
                                                 "default, hold a directional pad button\n"
                                                 "(Up/Right/Down/Left equal EmuNANDs\n"
                                                 "1/2/3/4).",

                                                 "Enable loading external FIRMs and\n"
                                                 "system modules.\n\n"
                                                 "This isn't needed in most cases.\n\n"
                                                 "Refer to the wiki for instructions.",

                                                 "Enable overriding the region and\n"
                                                 "language configuration and the usage\n"
                                                 "of patched code binaries, exHeaders,\n"
                                                 "IPS code patches and LayeredFS\n"
                                                 "for specific games.\n\n"
                                                 "Also makes certain DLCs for out-of-\n"
                                                 "region games work.\n\n"
                                                 "Refer to the wiki for instructions.",

                                                 "Redirect app. threads that would spawn\n"
                                                 "on core1, to core2 (which is an extra\n"
                                                 "CPU core for applications that usually\n"
                                                 "remains unused).\n\n"
                                                 "This improves the performance of very\n"
                                                 "demanding games (like Pok\x82mon US/UM)\n"
                                                 "by about 10%. Can break some games\n"
                                                 "and other applications.\n",

                                                 "Enable showing the current NAND:\n\n"
                                                 "\t* Sys  = SysNAND\n"
                                                 "\t* Emu  = EmuNAND 1\n"
                                                 "\t* EmuX = EmuNAND X\n\n"
                                                 "or a user-defined custom string in\n"
                                                 "System Settings.\n\n"
                                                 "Refer to the wiki for instructions.",

                                                 "Enable showing the GBA boot screen\n"
                                                 "when booting GBA games.",


                                                "Boot to the Luma3DS chainloader menu.",

                                                 "Save the changes and exit. To discard\n"
                                                 "any changes press the POWER button.\n"
                                                 "Use START as a shortcut to this entry."
                                               };
    static const char *optionsDescriptionJapanese[] = {
        "標準で起動するEmuNANDを選びます。\n\n起動時に十字キーを押さなければ、\nここで選んだEmuNANDを使います。\n\n十字上/右/下/左で、\nEmuNAND 1/2/3/4を指定できます。",
        "画面の明るさを選びます。\n\n4が最も明るく、1が最も暗い設定です。",
        "起動画面の表示タイミングを選びます。\n\n無効: 起動画面を表示しません。\n\nペイロード前: ペイロードを起動する前に\n表示します。操作キー案内の画像などに\n使えます。\n\n後: ペイロードの選択後に表示します。\n\n表示時間はconfig.iniで変更できます。\n標準は3秒です。",
        "Luma3DS起動時にPIN入力を求めます。\n\n4桁・6桁・8桁から選びます。\nABXYと十字キーを入力に使えます。\n\n無効を選ぶとPINロックを解除します。\n\n独自のメッセージ表示にも対応しています。",
        "New 3DSのCPUモードを選びます。\n\nClock: CPUの動作周波数を変更\nL2: L2キャッシュを有効化\nClock+L2: 両方を有効化\n\nNew 3DS専用・対応ソフトには適用されません。\nClock+L2で不具合が出るソフトもあります。",
        "3DSモードまたはDSiモードで、\nHomebrew Menuを自動起動します。\n\nゲームカードの自動起動には対応しません。\n設定ファイルのautoboot項目で指定します。",
        "有効ならEmuNANDを自動起動します。\n無効ならSysNANDを起動します。\n\n起動時にLを押すと、NANDを切り替えます。\n\n標準以外のEmuNANDを使う場合は、\n十字上/右/下/左を押して起動します。\nそれぞれEmuNAND 1/2/3/4に対応します。",
        "外部のFIRMやシステムモジュールを\n読み込めるようにします。\n\n通常の利用では不要です。\n配置方法などはLuma3DSの資料を確認してください。",
        "ゲームごとのリージョン・言語変更、\n変更済みコード、exHeader、IPSパッチ、\nLayeredFSを有効にします。\n\n一部の他地域向けDLCにも対応します。\n使い方はLuma3DSの資料を確認してください。",
        "アプリのcore1用スレッドを、\n通常は空いているcore2へ移します。\n\n負荷が高いゲームで性能が改善する場合が\nありますが、ソフトによっては不具合が\n発生します。",
        "本体設定に現在のNAND名を表示します。\n\nSys: SysNAND\nEmu: EmuNAND 1\nEmuX: EmuNAND X\n\n独自の文字列を表示することもできます。",
        "GBAソフトの起動時に、\nGBAの起動画面を表示します。",
        "Luma3DSのチェーンローダーを開き、\n起動するペイロードを選びます。",
        "変更を保存して終了します。\nSTARTでもこの項目を実行できます。\n\n保存せず終了する場合は、\n本体の電源ボタンを押してください。"
    };
    const char *const *optionsDescription = BootLanguage_IsJapanese() ? optionsDescriptionJapanese : optionsDescriptionEnglish;

    FirmwareSource nandType = FIRMWARE_SYSNAND;
    if(isSdMode)
    {

        u32 emuIndex = 0;
        nandType = FIRMWARE_EMUNAND;
        locateEmuNand(&nandType, &emuIndex, false);
    }

    struct multiOption {
        u32 posXs[4];
        u32 posY;
        u32 enabled;
        bool visible;
    } multiOptions[] = {
        { .visible = nandType == FIRMWARE_EMUNAND },
        { .visible = true },
        { .visible = true },
        { .visible = true },
        { .visible = ISN3DS },
        { .visible = true },

    };

    struct singleOption {
        u32 posY;
        bool enabled;
        bool visible;
    } singleOptions[] = {
        { .visible = nandType == FIRMWARE_EMUNAND },
        { .visible = true },
        { .visible = true },
        { .visible = ISN3DS },
        { .visible = true },
        { .visible = true },
        { .visible = true },
        { .visible = true },
    };


    u32 multiOptionsAmount = sizeof(multiOptions) / sizeof(struct multiOption),
        singleOptionsAmount = sizeof(singleOptions) / sizeof(struct singleOption),
        totalIndexes = multiOptionsAmount + singleOptionsAmount - 1,
        selectedOption = 0,
        singleSelected = 0;
    bool isMultiOption = false;

    for(u32 i = 0; i < multiOptionsAmount; i++)
        multiOptions[i].enabled = MULTICONFIG(i);
    for(u32 i = 0; i < singleOptionsAmount; i++)
        singleOptions[i].enabled = CONFIG(i);
    singleSelected = 0;
    while(!singleOptions[singleSelected].visible) ++singleSelected;
    selectedOption = multiOptionsAmount + singleSelected;

    initScreens();
    static const char *bootTypes[] = { "B9S", "B9S (ntrboot)", "FIRM0", "FIRM1" };
    const char selected = 'x';
    bool redraw = true;
    bool languageSaveFailed = false;
    bool languageHeld = false;

    bool startPressed = false;

    while(true)
    {
        if(redraw)
        {
            multiOptionsText = BootLanguage_IsJapanese() ? multiOptionsTextJapanese : multiOptionsTextEnglish;
            singleOptionsText = BootLanguage_IsJapanese() ? singleOptionsTextJapanese : singleOptionsTextEnglish;
            optionsDescription = BootLanguage_IsJapanese() ? optionsDescriptionJapanese : optionsDescriptionEnglish;
            clearScreens(false);
            drawString(true, 10, 10, COLOR_TITLE, BootLanguage_IsJapanese() ? "Luma3DS v13.4Y 設定" : CONFIG_TITLE);
            drawString(true, 10, 20, COLOR_TITLE, BootLanguage_Text("Use the DPAD and A to change settings"));
            drawString(true, 10, 30, COLOR_TITLE, BootLanguage_Selector());
            drawFormattedString(false, 10, SCREEN_HEIGHT - 2 * SPACING_Y, COLOR_YELLOW, BootLanguage_Text("Booted from %s via %s"), isSdMode ? "SD" : "CTRNAND", bootTypes[(u32)bootType]);

            u32 endPos = 30;
            for(u32 i = 0; i < multiOptionsAmount; i++)
            {
                u32 optionNum = 0;
                const char *label = multiOptionsText[i];
                for(u32 column = 0; *label && optionNum < 4; ++column)
                    if(Text_Decode(&label) == '(')
                        multiOptions[i].posXs[optionNum++] = column + 1;
                while(optionNum < 4) multiOptions[i].posXs[optionNum++] = 0;
                if(!multiOptions[i].visible) continue;
                u32 color = selectedOption == i ? COLOR_RED : COLOR_TEXT;
                multiOptions[i].posY = endPos + SPACING_Y;
                endPos = drawString(true, 10, multiOptions[i].posY, color, multiOptionsText[i]);
                drawCharacter(true, 10 + multiOptions[i].posXs[multiOptions[i].enabled] * SPACING_X, multiOptions[i].posY, color, selected);
            }
            endPos += SPACING_Y / 2;
            for(u32 i = 0; i < singleOptionsAmount; i++)
            {
                if(!singleOptions[i].visible) continue;
                u32 color = selectedOption == i + multiOptionsAmount ? COLOR_RED : COLOR_TEXT;
                singleOptions[i].posY = endPos + SPACING_Y;
                endPos = drawString(true, 10, singleOptions[i].posY, color, singleOptionsText[i]);
                if(singleOptions[i].enabled && singleOptionsText[i][0] == '(')
                    drawCharacter(true, 10 + SPACING_X, singleOptions[i].posY, color, selected);
            }
            drawString(false, 10, 10, COLOR_TEXT, optionsDescription[selectedOption]);
            redraw = false;
        }

        if(languageHeld)
        {
            while(HID_PAD & BUTTON_X) wait(1);
            languageHeld = false;
        }

        u32 pressed = 0;
        if (!startPressed)
        do
        {
            pressed = waitInput(true) & (MENU_BUTTONS | BUTTON_X);
        }
        while(!pressed);


        if(pressed & BUTTON_X)
        {
            languageSaveFailed = !BootLanguage_Toggle();
            if(languageSaveFailed)
                drawString(false, 10, 210, COLOR_RED, BootLanguage_Text("Unable to save language"));
            else
                redraw = true;
            languageHeld = true;
            continue;
        }
        if(languageSaveFailed)
        {
            drawString(false, 10, 210, COLOR_BLACK, BootLanguage_Text("Unable to save language"));
            languageSaveFailed = false;
        }

        if(pressed & BUTTON_START)
        {
            startPressed = true;

            pressed = BUTTON_RIGHT;
        }

        if(pressed & DPAD_BUTTONS)
        {

            u32 oldSelectedOption = selectedOption;

            while(true)
            {
                switch(pressed & DPAD_BUTTONS)
                {
                    case BUTTON_UP:
                        selectedOption = !selectedOption ? totalIndexes : selectedOption - 1;
                        break;
                    case BUTTON_DOWN:
                        selectedOption = selectedOption == totalIndexes ? 0 : selectedOption + 1;
                        break;
                    case BUTTON_LEFT:
                        pressed = BUTTON_DOWN;
                        selectedOption = 0;
                        break;
                    case BUTTON_RIGHT:
                        pressed = BUTTON_UP;
                        selectedOption = totalIndexes;
                        break;
                    default:
                        break;
                }

                if(selectedOption < multiOptionsAmount)
                {
                    if(!multiOptions[selectedOption].visible) continue;

                    isMultiOption = true;
                    break;
                }
                else
                {
                    singleSelected = selectedOption - multiOptionsAmount;

                    if(!singleOptions[singleSelected].visible) continue;

                    isMultiOption = false;
                    break;
                }
            }

            if(selectedOption == oldSelectedOption && !startPressed) continue;


            if(oldSelectedOption < multiOptionsAmount)
            {
                drawString(true, 10, multiOptions[oldSelectedOption].posY, COLOR_TEXT, multiOptionsText[oldSelectedOption]);
                drawCharacter(true, 10 + multiOptions[oldSelectedOption].posXs[multiOptions[oldSelectedOption].enabled] * SPACING_X, multiOptions[oldSelectedOption].posY, COLOR_TEXT, selected);
            }
            else
            {
                u32 singleOldSelected = oldSelectedOption - multiOptionsAmount;
                drawString(true, 10, singleOptions[singleOldSelected].posY, COLOR_TEXT, singleOptionsText[singleOldSelected]);
                if(singleOptions[singleOldSelected].enabled) drawCharacter(true, 10 + SPACING_X, singleOptions[singleOldSelected].posY, COLOR_TEXT, selected);
            }

            if(isMultiOption) drawString(true, 10, multiOptions[selectedOption].posY, COLOR_RED, multiOptionsText[selectedOption]);
            else drawString(true, 10, singleOptions[singleSelected].posY, COLOR_RED, singleOptionsText[singleSelected]);

            drawString(false, 10, 10, COLOR_BLACK, optionsDescription[oldSelectedOption]);
            drawString(false, 10, 10, COLOR_TEXT, optionsDescription[selectedOption]);
        }
        else if (pressed & BUTTON_A || startPressed)
        {
            if(isMultiOption)
            {
                u32 oldEnabled = multiOptions[selectedOption].enabled;
                drawCharacter(true, 10 + multiOptions[selectedOption].posXs[oldEnabled] * SPACING_X, multiOptions[selectedOption].posY, COLOR_BLACK, selected);
                multiOptions[selectedOption].enabled = (oldEnabled == 3 || !multiOptions[selectedOption].posXs[oldEnabled + 1]) ? 0 : oldEnabled + 1;

                if(selectedOption == BRIGHTNESS) updateBrightness(multiOptions[BRIGHTNESS].enabled);
            }
            else
            {

                if (singleSelected == singleOptionsAmount - 1)
                {
                    drawString(true, 10, singleOptions[singleSelected].posY, COLOR_GREEN, singleOptionsText[singleSelected]);
                    startPressed = false;
                    break;
                }
                else if (singleSelected == singleOptionsAmount - 2) {
                    loadHomebrewFirm(0);
                    break;
                }
                else
                {
                    bool oldEnabled = singleOptions[singleSelected].enabled;
                    singleOptions[singleSelected].enabled = !oldEnabled;
                    if(oldEnabled) drawCharacter(true, 10 + SPACING_X, singleOptions[singleSelected].posY, COLOR_BLACK, selected);
                }
            }
        }


        if(isMultiOption) drawCharacter(true, 10 + multiOptions[selectedOption].posXs[multiOptions[selectedOption].enabled] * SPACING_X, multiOptions[selectedOption].posY, COLOR_RED, selected);
        else if(singleOptions[singleSelected].enabled && singleOptionsText[singleSelected][0] == '(') drawCharacter(true, 10 + SPACING_X, singleOptions[singleSelected].posY, COLOR_RED, selected);
    }


    configData.multiConfig = 0;
    for(u32 i = 0; i < multiOptionsAmount; i++)
        configData.multiConfig |= multiOptions[i].enabled << (i * 2);

    configData.config &= ~((1 << (u32)NUMCONFIGURABLE) - 1);
    for(u32 i = 0; i < singleOptionsAmount; i++)
        configData.config |= (singleOptions[i].enabled ? 1 : 0) << i;

    writeConfig(true);

    u32 newPinMode = MULTICONFIG(PIN);

    if(newPinMode != 0) newPin(oldPinStatus && newPinMode == oldPinMode, newPinMode);
    else if(oldPinStatus)
    {
        if(!fileDelete(PIN_FILE))
            error("Unable to delete PIN file");
    }

    while(HID_PAD & PIN_BUTTONS);
    wait(2000ULL);
}
