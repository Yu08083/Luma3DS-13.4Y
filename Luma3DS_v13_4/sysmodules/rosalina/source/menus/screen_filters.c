/*
*   This file is part of Luma3DS
*   Copyright (C) 2017-2018 Sono (https://github.com/MarcuzD), panicbit
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

#include <3ds.h>
#include <math.h>
#include <string.h>
#include <arm_acle.h>
#include "memory.h"
#include "menu.h"
#include "menus/screen_filters.h"
#include "menus/screen_filters_srgb_tables.h"
#include "draw.h"
#include "menus/lang.h"
#include "luma_config.h"
#include "redshift/colorramp.h"

ScreenFilter topScreenFilter;
ScreenFilter bottomScreenFilter;

static inline bool ScreenFiltersMenu_IsDefaultSettingsFilter(const ScreenFilter *filter)
{
    bool ok = true;
    ok = ok && filter->cct == 6500;
    ok = ok && !filter->invert;
    for (u32 ch = 0; ch < 3; ch++)
    {
        ok = ok && filter->gamma[ch] == 1.0f;
        ok = ok && filter->contrast[ch] == 1.0f;
    }
    ok = ok && filter->colorCurveCorrection == 0;
    return ok;
}

static inline bool ScreenFiltersMenu_IsDefaultSettings(void)
{
    return ScreenFiltersMenu_IsDefaultSettingsFilter(&topScreenFilter) && ScreenFiltersMenu_IsDefaultSettingsFilter(&bottomScreenFilter);
}

#define SCREEN_FILTER_NUM_PRIMES                54

#define SCREEN_FILTER_NUM_ODD_COMPOSITES        (128 - SCREEN_FILTER_NUM_PRIMES)

#define SCREEN_FILTER_NUM_LOG2_PRIMES_PADDED    ((SCREEN_FILTER_NUM_PRIMES + 7) / 8 * 8)

typedef struct ScreenFilterOddComposite {
    u8 value;
    u8 factor;
    u8 otherFactor;
} ScreenFilterOddComposite;

static const u8 screenFilterPrimes[SCREEN_FILTER_NUM_PRIMES] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
    67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251,
};

static const float screenFilterLog2PrimesPadded[SCREEN_FILTER_NUM_LOG2_PRIMES_PADDED] = {
    1.0f, 1.5849625f, 2.321928f, 2.807355f, 3.4594316f, 3.7004397f,
    4.087463f, 4.2479277f, 4.523562f, 4.857981f, 4.9541965f, 5.2094536f,
    5.357552f, 5.426265f, 5.554589f, 5.7279205f, 5.882643f, 5.9307375f,
    6.066089f, 6.149747f, 6.1898246f, 6.3037806f, 6.3750396f, 6.4757333f,
    6.5999126f, 6.6582117f, 6.6865005f, 6.741467f, 6.768184f, 6.820179f,
    6.9886847f, 7.033423f, 7.098032f, 7.1189413f, 7.2191687f, 7.2384048f,
    7.2946205f, 7.348728f, 7.383704f, 7.434628f, 7.4838157f, 7.499846f,
    7.577429f, 7.592457f, 7.6220517f, 7.636625f, 7.7210994f, 7.8009f,
    7.8265486f, 7.839204f, 7.8641863f, 7.900867f, 7.9128895f, 7.971544f,
    0.0f, 0.0f,
};

static const ScreenFilterOddComposite screenFilterOddComposites[SCREEN_FILTER_NUM_ODD_COMPOSITES] = {
    {   9,  3,  3 }, {  15,  3,  5 }, {  21,  3,  7 }, {  25,  5,  5 },
    {  27,  3,  9 }, {  33,  3, 11 }, {  35,  5,  7 }, {  39,  3, 13 },
    {  45,  3, 15 }, {  49,  7,  7 }, {  51,  3, 17 }, {  55,  5, 11 },
    {  57,  3, 19 }, {  63,  3, 21 }, {  65,  5, 13 }, {  69,  3, 23 },
    {  75,  3, 25 }, {  77,  7, 11 }, {  81,  3, 27 }, {  85,  5, 17 },
    {  87,  3, 29 }, {  91,  7, 13 }, {  93,  3, 31 }, {  95,  5, 19 },
    {  99,  3, 33 }, { 105,  3, 35 }, { 111,  3, 37 }, { 115,  5, 23 },
    { 117,  3, 39 }, { 119,  7, 17 }, { 121, 11, 11 }, { 123,  3, 41 },
    { 125,  5, 25 }, { 129,  3, 43 }, { 133,  7, 19 }, { 135,  3, 45 },
    { 141,  3, 47 }, { 143, 11, 13 }, { 145,  5, 29 }, { 147,  3, 49 },
    { 153,  3, 51 }, { 155,  5, 31 }, { 159,  3, 53 }, { 161,  7, 23 },
    { 165,  3, 55 }, { 169, 13, 13 }, { 171,  3, 57 }, { 175,  5, 35 },
    { 177,  3, 59 }, { 183,  3, 61 }, { 185,  5, 37 }, { 187, 11, 17 },
    { 189,  3, 63 }, { 195,  3, 65 }, { 201,  3, 67 }, { 203,  7, 29 },
    { 205,  5, 41 }, { 207,  3, 69 }, { 209, 11, 19 }, { 213,  3, 71 },
    { 215,  5, 43 }, { 217,  7, 31 }, { 219,  3, 73 }, { 221, 13, 17 },
    { 225,  3, 75 }, { 231,  3, 77 }, { 235,  5, 47 }, { 237,  3, 79 },
    { 243,  3, 81 }, { 245,  5, 49 }, { 247, 13, 19 }, { 249,  3, 83 },
    { 253, 11, 23 }, { 255,  3, 85 },
};

#define SCREEN_FILTER_ROUND_MAGIC       12582912.0f
#define SCREEN_FILTER_ROUND_MAGIC_BITS  0x4B400000u

#define SCREEN_FILTER_VECTOR_FPSCR      (BIT(25) | BIT(24) | (0u << 22) | (7u << 16))

static inline float ScreenFiltersMenu_FloatFromBits(u32 bits)
{
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

static inline u32 ScreenFiltersMenu_FloatToBits(float f)
{
    u32 bits;
    memcpy(&bits, &f, sizeof(bits));
    return bits;
}

static void ScreenFiltersMenu_FastExp2Vector(u32 *outPBits, u32 *outMBits, const float *log2s, float gamma, u32 numGroups)
{

    const float scalars[8] = {
        gamma, SCREEN_FILTER_ROUND_MAGIC,
        1.0000001192092896f, 0.6931469440460205f, 0.24022120237350464f,
        0.05550713092088699f, 0.009675540961325169f, 0.0013276472454890609f,
    };

    u32 oldFpscr;
    __asm__ __volatile__(
        "vmrs       %[oldFpscr], fpscr\n\t"
        "vldmia     %[scalars], {s0-s7}\n\t"
        "vmsr       fpscr, %[newFpscr]\n"
        "1:\n\t"
        "vldmia     %[log2s]!, {s8-s15}\n\t"
        "vmul.f32   s16, s8, s0\n\t"
        "vadd.f32   s24, s16, s1\n\t"
        "vsub.f32   s8, s24, s1\n\t"
        "vstmia     %[outMBits]!, {s24-s31}\n\t"
        "vsub.f32   s24, s16, s8\n\t"
        "vmov.f32   s16, s6\n\t"
        "vmla.f32   s16, s24, s7\n\t"
        "vmov.f32   s8, s5\n\t"
        "vmla.f32   s8, s16, s24\n\t"
        "vmov.f32   s16, s4\n\t"
        "vmla.f32   s16, s8, s24\n\t"
        "vmov.f32   s8, s3\n\t"
        "vmla.f32   s8, s16, s24\n\t"
        "vmov.f32   s16, s2\n\t"
        "vmla.f32   s16, s8, s24\n\t"
        "vstmia     %[outPBits]!, {s16-s23}\n\t"
        "subs       %[numGroups], %[numGroups], #1\n\t"
        "bne        1b\n\t"
        "vmsr       fpscr, %[oldFpscr]"
        : [oldFpscr] "=&r"(oldFpscr), [log2s] "+r"(log2s),
          [outPBits] "+r"(outPBits), [outMBits] "+r"(outMBits), [numGroups] "+r"(numGroups)
        : [scalars] "r"(scalars), [newFpscr] "r"(SCREEN_FILTER_VECTOR_FPSCR)
        : "cc", "memory",
          "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15"
    );
}

static void ScreenFiltersMenu_ConvertTo8BitVector(u32 *outBits, const float *t, float scale)
{

    const float scalars[2] = { scale, SCREEN_FILTER_ROUND_MAGIC };

    u32 oldFpscr;
    u32 numGroups = 256 / 8;
    __asm__ __volatile__(
        "vmrs       %[oldFpscr], fpscr\n\t"
        "vldmia     %[scalars], {s0-s1}\n\t"
        "vmsr       fpscr, %[newFpscr]\n"
        "1:\n\t"
        "vmov.f32   s16, s1\n\t"
        "vldmia     %[t]!, {s8-s15}\n\t"
        "vmla.f32   s16, s8, s0\n\t"
        "vstmia     %[outBits]!, {s16-s23}\n\t"
        "subs       %[numGroups], %[numGroups], #1\n\t"
        "bne        1b\n\t"
        "vmsr       fpscr, %[oldFpscr]"
        : [oldFpscr] "=&r"(oldFpscr), [t] "+r"(t), [outBits] "+r"(outBits), [numGroups] "+r"(numGroups)
        : [scalars] "r"(scalars), [newFpscr] "r"(SCREEN_FILTER_VECTOR_FPSCR)
        : "cc", "memory", "d0", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11"
    );
}

static void ScreenFiltersMenu_ConvertTo8BitVectorByteLane(u8 *outBytes, const float *t, float scale)
{

    const float scalars[2] = { scale, SCREEN_FILTER_ROUND_MAGIC };

    u32 oldFpscr, lo, hi;
    u32 numGroups = 256 / 8;
    __asm__ __volatile__(
        "vmrs       %[oldFpscr], fpscr\n\t"
        "vldmia     %[scalars], {s0-s1}\n\t"
        "vmsr       fpscr, %[newFpscr]\n"
        "1:\n\t"
        "vmov.f32   s16, s1\n\t"
        "vldmia     %[t]!, {s8-s15}\n\t"
        "vmla.f32   s16, s8, s0\n\t"
        "vmov       %[lo], %[hi], s16, s17\n\t"
        "sub        %[lo], %[lo], %[magicBits]\n\t"
        "sub        %[hi], %[hi], %[magicBits]\n\t"
        "usat       %[lo], #8, %[lo]\n\t"
        "usat       %[hi], #8, %[hi]\n\t"
        "strb       %[lo], [%[outBytes]], #4\n\t"
        "strb       %[hi], [%[outBytes]], #4\n\t"
        "vmov       %[lo], %[hi], s18, s19\n\t"
        "sub        %[lo], %[lo], %[magicBits]\n\t"
        "sub        %[hi], %[hi], %[magicBits]\n\t"
        "usat       %[lo], #8, %[lo]\n\t"
        "usat       %[hi], #8, %[hi]\n\t"
        "strb       %[lo], [%[outBytes]], #4\n\t"
        "strb       %[hi], [%[outBytes]], #4\n\t"
        "vmov       %[lo], %[hi], s20, s21\n\t"
        "sub        %[lo], %[lo], %[magicBits]\n\t"
        "sub        %[hi], %[hi], %[magicBits]\n\t"
        "usat       %[lo], #8, %[lo]\n\t"
        "usat       %[hi], #8, %[hi]\n\t"
        "strb       %[lo], [%[outBytes]], #4\n\t"
        "strb       %[hi], [%[outBytes]], #4\n\t"
        "vmov       %[lo], %[hi], s22, s23\n\t"
        "sub        %[lo], %[lo], %[magicBits]\n\t"
        "sub        %[hi], %[hi], %[magicBits]\n\t"
        "usat       %[lo], #8, %[lo]\n\t"
        "usat       %[hi], #8, %[hi]\n\t"
        "strb       %[lo], [%[outBytes]], #4\n\t"
        "strb       %[hi], [%[outBytes]], #4\n\t"
        "subs       %[numGroups], %[numGroups], #1\n\t"
        "bne        1b\n\t"
        "vmsr       fpscr, %[oldFpscr]"
        : [oldFpscr] "=&r"(oldFpscr), [lo] "=&r"(lo), [hi] "=&r"(hi),
          [t] "+r"(t), [outBytes] "+r"(outBytes), [numGroups] "+r"(numGroups)
        : [scalars] "r"(scalars), [newFpscr] "r"(SCREEN_FILTER_VECTOR_FPSCR),
          [magicBits] "r"(SCREEN_FILTER_ROUND_MAGIC_BITS)
        : "cc", "memory", "d0", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11"
    );
}

static void ScreenFiltersMenu_BuildGammaTable(float *outTable, float gamma)
{
    u32 exp2PBits[SCREEN_FILTER_NUM_LOG2_PRIMES_PADDED];
    u32 exp2MBits[SCREEN_FILTER_NUM_LOG2_PRIMES_PADDED];

    ScreenFiltersMenu_FastExp2Vector(exp2PBits, exp2MBits, screenFilterLog2PrimesPadded, gamma, SCREEN_FILTER_NUM_LOG2_PRIMES_PADDED / 8);
    for (u32 i = 0; i < SCREEN_FILTER_NUM_PRIMES; i++)
        outTable[screenFilterPrimes[i]] = ScreenFiltersMenu_FloatFromBits(exp2PBits[i] + (exp2MBits[i] << 23));

    outTable[0] = 0.0f;
    outTable[1] = 1.0f;

    u32 i = 0;
    for (; i + 4 <= SCREEN_FILTER_NUM_ODD_COMPOSITES; i += 4)
    {
        const ScreenFilterOddComposite *c0 = &screenFilterOddComposites[i + 0];
        const ScreenFilterOddComposite *c1 = &screenFilterOddComposites[i + 1];
        const ScreenFilterOddComposite *c2 = &screenFilterOddComposites[i + 2];
        const ScreenFilterOddComposite *c3 = &screenFilterOddComposites[i + 3];
        float a0 = outTable[c0->factor], b0 = outTable[c0->otherFactor];
        float a1 = outTable[c1->factor], b1 = outTable[c1->otherFactor];
        float a2 = outTable[c2->factor], b2 = outTable[c2->otherFactor];
        float a3 = outTable[c3->factor], b3 = outTable[c3->otherFactor];
        outTable[c0->value] = a0 * b0;
        outTable[c1->value] = a1 * b1;
        outTable[c2->value] = a2 * b2;
        outTable[c3->value] = a3 * b3;
    }

    for (; i < SCREEN_FILTER_NUM_ODD_COMPOSITES; i++)
    {
        const ScreenFilterOddComposite *c = &screenFilterOddComposites[i];
        outTable[c->value] = outTable[c->factor] * outTable[c->otherFactor];
    }

    float twoG = outTable[2];
    outTable[4] = twoG * twoG;
    outTable[6] = twoG * outTable[3];
    for (u32 k = 4; k < 128; k += 4)
    {
        float a0 = outTable[k + 0];
        float a1 = outTable[k + 1];
        float a2 = outTable[k + 2];
        float a3 = outTable[k + 3];
        outTable[2 * k + 0] = twoG * a0;
        outTable[2 * k + 2] = twoG * a1;
        outTable[2 * k + 4] = twoG * a2;
        outTable[2 * k + 6] = twoG * a3;
    }
}

static void ScreenFiltersMenu_ApplyColorSettings(bool top)
{
    const ScreenFilter *filter = top ? &topScreenFilter : &bottomScreenFilter;
    const u8 (*srgbTbl)[3] = filter->colorCurveCorrection == 2 ? ctrToSrgbTableBottom : ctrToSrgbTableTop;

    float wp[3];
    colorramp_get_white_point(wp, filter->cct);

    static float gammaTable[256];
    static u32 packedRedGreen[256];

    u8 *redGreenBytes = (u8 *)packedRedGreen;

    for (u32 ch = 0; ch < 3; ch++)
    {
        float base = CLAMP(filter->contrast[ch], 0.0f, SCREEN_FILTER_MAX_CONTRAST) * wp[ch];
        ScreenFiltersMenu_BuildGammaTable(gammaTable, filter->gamma[ch]);
        float scale = base > 0.0f ? 255.0f * powf(base, filter->gamma[ch]) / gammaTable[255] : 0.0f;

        if (ch < 2)
            ScreenFiltersMenu_ConvertTo8BitVectorByteLane(redGreenBytes + ch, gammaTable, scale);
        else
            ScreenFiltersMenu_ConvertTo8BitVector((u32 *)gammaTable, gammaTable, scale);
    }

    u32 magicBits = SCREEN_FILTER_ROUND_MAGIC_BITS;
    __asm__ ("" : "+r"(magicBits));
    #define BLUE(i) ((u32)__usat((s32)(ScreenFiltersMenu_FloatToBits(gammaTable[i]) - magicBits), 8))

    u32 invertMask = filter->invert ? 0xFFFFFF : 0;

    for (;;)
    {
        u32 vCount = GPU_FB_BOTTOM_VCOUNT;
        u32 vBlank = GPU_FB_BOTTOM_VBLANK;
        if (vCount >= vBlank && vCount < vBlank + 4)
            break;
    }

    vu32 *lutElem;
    if (top)
    {
        GPU_FB_TOP_COL_LUT_INDEX = 0;
        lutElem = &GPU_FB_TOP_COL_LUT_ELEM;
    }
    else
    {
        GPU_FB_BOTTOM_COL_LUT_INDEX = 0;
        lutElem = &GPU_FB_BOTTOM_COL_LUT_ELEM;
    }

    if (filter->colorCurveCorrection != 0)
    {
        for (u32 i = 0; i < 256; i++)
        {
            u32 r = srgbTbl[redGreenBytes[4 * i + 0]][0];
            u32 g = srgbTbl[redGreenBytes[4 * i + 1]][1];
            u32 b = srgbTbl[BLUE(i)][2];
            *lutElem = ((b << 16) | (g << 8) | r) ^ invertMask;
        }
    }
    else
    {

        for (u32 i = 0; i < 256; i += 2)
        {
            u32 rg0 = packedRedGreen[i] & 0xFFFF, b0 = BLUE(i);
            u32 rg1 = packedRedGreen[i + 1] & 0xFFFF, b1 = BLUE(i + 1);
            *lutElem = ((b0 << 16) | rg0) ^ invertMask;
            *lutElem = ((b1 << 16) | rg1) ^ invertMask;
        }
    }

    #undef BLUE
}

static void ScreenFiltersMenu_SetCct(u16 cct)
{
    topScreenFilter.cct = cct;
    bottomScreenFilter.cct = cct;
    ScreenFiltersMenu_ApplyColorSettings(true);
    ScreenFiltersMenu_ApplyColorSettings(false);
}

static void ScreenFiltersMenu_UpdateEntries(void)
{
    if (topScreenFilter.colorCurveCorrection == 0)
    {
        screenFiltersMenu.items[10].title = "[IPS recommended] Enhance top screen colors";
        screenFiltersMenu.items[10].method = &ScreenFiltersMenu_SetTopScreenSrgbColorCurve;
    }
    else
    {
        screenFiltersMenu.items[10].title = "Restore top screen color curve";
        screenFiltersMenu.items[10].method = &ScreenFiltersMenu_RestoreTopScreenColorCurve;
    }

    if (bottomScreenFilter.colorCurveCorrection == 0)
    {
        screenFiltersMenu.items[11].title = "[IPS recommended] Enhance bottom screen colors";
        screenFiltersMenu.items[11].method = &ScreenFiltersMenu_SetBottomScreenSrgbColorCurve;
    }
    else
    {
        screenFiltersMenu.items[11].title = "Restore bottom screen color curve";
        screenFiltersMenu.items[11].method = &ScreenFiltersMenu_RestoreBottomScreenColorCurve;
    }
}
static void ScreenFiltersMenu_SetColorCurveCorrection(bool top, u8 colorCurveCorrection)
{
    u8 flag = top ? MCUCFG_FLAG_ENHANCE_TOP_SCREEN_COLORS : MCUCFG_FLAG_ENHANCE_BOTTOM_SCREEN_COLORS;

    if (top)
        topScreenFilter.colorCurveCorrection = colorCurveCorrection;
    else
        bottomScreenFilter.colorCurveCorrection = colorCurveCorrection;

    ScreenFiltersMenu_ApplyColorSettings(top);
    ScreenFiltersMenu_UpdateEntries();

    LumaConfig_SetMcuConfigFlags(flag, colorCurveCorrection != 0 ? flag : 0);
}

Menu screenFiltersMenu = {
    "Screen filters menu",
    {
        { "[6500K] Default temperature", METHOD, .method = &ScreenFiltersMenu_SetDefault },
        { "[10000K] Aquarium", METHOD, .method = &ScreenFiltersMenu_SetAquarium },
        { "[7500K] Overcast Sky", METHOD, .method = &ScreenFiltersMenu_SetOvercastSky },
        { "[5500K] Daylight", METHOD, .method = &ScreenFiltersMenu_SetDaylight },
        { "[4200K] Fluorescent", METHOD, .method = &ScreenFiltersMenu_SetFluorescent },
        { "[3400K] Halogen", METHOD, .method = &ScreenFiltersMenu_SetHalogen },
        { "[2700K] Incandescent", METHOD, .method = &ScreenFiltersMenu_SetIncandescent },
        { "[2300K] Warm Incandescent", METHOD, .method = &ScreenFiltersMenu_SetWarmIncandescent },
        { "[1900K] Candle", METHOD, .method = &ScreenFiltersMenu_SetCandle },
        { "[1200K] Ember", METHOD, .method = &ScreenFiltersMenu_SetEmber },
        { "[IPS recommended] Enhance top screen colors", METHOD, .method = &ScreenFiltersMenu_SetTopScreenSrgbColorCurve },
        { "[IPS recommended] Enhance bottom screen colors", METHOD, .method = &ScreenFiltersMenu_SetTopScreenSrgbColorCurve },
        { "Advanced configuration...", METHOD, .method = &ScreenFiltersMenu_AdvancedConfiguration },
        {},
    }
};

#define DEF_CCT_SETTER(temp, name)\
void ScreenFiltersMenu_Set##name(void)\
{\
    ScreenFiltersMenu_SetCct(temp);\
}

#define DEF_SRGB_SETTER(top, profile, name)\
void ScreenFiltersMenu_##name(void)\
{\
    ScreenFiltersMenu_SetColorCurveCorrection(top, profile);\
}

void ScreenFiltersMenu_RestoreSettings(void)
{

    if (ScreenFiltersMenu_IsDefaultSettings())
        return;

    svcSleepThread(20 * 1000 * 1000LL);

    svcKernelSetState(0x10000, 2);
    svcSleepThread(5 * 1000 * 100LL);

    ScreenFiltersMenu_ApplyColorSettings(true);
    ScreenFiltersMenu_ApplyColorSettings(false);

    svcKernelSetState(0x10000, 2);
    svcSleepThread(5 * 1000 * 100LL);
}

static float ScreenFiltersMenu_LoadFloatConfig(u32 id, float minVal, float maxVal, float defaultVal)
{
    s64 out = 0;
    svcGetSystemInfo(&out, 0x10000, id);
    float val = (float)(out / FLOAT_CONV_MULT);
    return val >= minVal && val <= maxVal ? val : defaultVal;
}

static void ScreenFiltersMenu_LoadFilterConfig(ScreenFilter *filter, u32 cctId, const u32 *gammaIds, const u32 *contrastIds, u32 invertId)
{
    s64 out = 0;

    svcGetSystemInfo(&out, 0x10000, cctId);
    filter->cct = (u16)out;
    if (filter->cct < 1000 || filter->cct > 25100)
        filter->cct = 6500;

    for (u32 ch = 0; ch < 3; ch++)
    {
        filter->gamma[ch] = ScreenFiltersMenu_LoadFloatConfig(gammaIds[ch], 0.0f, SCREEN_FILTER_MAX_GAMMA, 1.0f);
        filter->contrast[ch] = ScreenFiltersMenu_LoadFloatConfig(contrastIds[ch], 0.0f, SCREEN_FILTER_MAX_CONTRAST, 1.0f);
    }

    svcGetSystemInfo(&out, 0x10000, invertId);
    filter->invert = (bool)out;
}

void ScreenFiltersMenu_LoadConfig(void)
{

    static const u32 topGammaIds[3] = { 0x104, 0x110, 0x111 };
    static const u32 topContrastIds[3] = { 0x105, 0x112, 0x113 };
    static const u32 bottomGammaIds[3] = { 0x109, 0x114, 0x115 };
    static const u32 bottomContrastIds[3] = { 0x10A, 0x116, 0x117 };

    ScreenFiltersMenu_LoadFilterConfig(&topScreenFilter, 0x102, topGammaIds, topContrastIds, 0x107);
    ScreenFiltersMenu_LoadFilterConfig(&bottomScreenFilter, 0x108, bottomGammaIds, bottomContrastIds, 0x10C);

    topScreenFilter.colorCurveCorrection = (lumaMcuConfig.flags & MCUCFG_FLAG_ENHANCE_TOP_SCREEN_COLORS) ? 1 : 0;
    bottomScreenFilter.colorCurveCorrection = (lumaMcuConfig.flags & MCUCFG_FLAG_ENHANCE_BOTTOM_SCREEN_COLORS) ? 2 : 0;

    ScreenFiltersMenu_UpdateEntries();
}

DEF_CCT_SETTER(6500, Default)

DEF_CCT_SETTER(10000, Aquarium)
DEF_CCT_SETTER(7500, OvercastSky)
DEF_CCT_SETTER(5500, Daylight)
DEF_CCT_SETTER(4200, Fluorescent)
DEF_CCT_SETTER(3400, Halogen)
DEF_CCT_SETTER(2700, Incandescent)
DEF_CCT_SETTER(2300, WarmIncandescent)
DEF_CCT_SETTER(1900, Candle)
DEF_CCT_SETTER(1200, Ember)

void ScreenFiltersMenu_SetTopScreenSrgbColorCurve(void)
{
    ScreenFiltersMenu_SetColorCurveCorrection(true, 1);
}

void ScreenFiltersMenu_RestoreTopScreenColorCurve(void)
{
    ScreenFiltersMenu_SetColorCurveCorrection(true, 0);
}

void ScreenFiltersMenu_SetBottomScreenSrgbColorCurve(void)
{
    ScreenFiltersMenu_SetColorCurveCorrection(false, 2);
}

void ScreenFiltersMenu_RestoreBottomScreenColorCurve(void)
{
    ScreenFiltersMenu_SetColorCurveCorrection(false, 0);
}

enum
{
    SCREEN_FILTER_FIELD_TEMPERATURE = 0,
    SCREEN_FILTER_FIELD_GAMMA,
    SCREEN_FILTER_FIELD_CONTRAST,
    SCREEN_FILTER_FIELD_INVERT,

    SCREEN_FILTER_NUM_FIELDS_PER_SCREEN,
};

static bool ScreenFiltersMenu_IsUnified(const ScreenFilter *filter)
{
    return filter->gamma[1] == filter->gamma[0] && filter->gamma[2] == filter->gamma[0] &&
           filter->contrast[1] == filter->contrast[0] && filter->contrast[2] == filter->contrast[0];
}

static void ScreenFiltersMenu_Unify(ScreenFilter *filter)
{

    filter->gamma[1] = filter->gamma[2] = filter->gamma[0];
    filter->contrast[1] = filter->contrast[2] = filter->contrast[0];
}

static void ScreenFiltersMenu_StepChannel(float *vals, int mult, float maxVal, bool split, u32 channel)
{
    u32 ch = split ? channel : 0;
    float val = CLAMP(vals[ch] + 0.01f * mult, 0.0f, maxVal);
    if (split)
        vals[ch] = val;
    else
        vals[0] = vals[1] = vals[2] = val;
}

static void ScreenFiltersMenu_AdvancedConfigurationChangeValue(int pos, int mult, bool sync, bool split, u32 channel)
{
    ScreenFilter *filter = pos >= SCREEN_FILTER_NUM_FIELDS_PER_SCREEN ? &bottomScreenFilter : &topScreenFilter;
    ScreenFilter *otherFilter = pos >= SCREEN_FILTER_NUM_FIELDS_PER_SCREEN ? &topScreenFilter : &bottomScreenFilter;

    int otherMult = sync ? mult : 0;

    switch (pos % SCREEN_FILTER_NUM_FIELDS_PER_SCREEN)
    {
        case SCREEN_FILTER_FIELD_TEMPERATURE:
            filter->cct = CLAMP(filter->cct + 100 * mult, 1000, 25100);
            otherFilter->cct = CLAMP(otherFilter->cct + 100 * otherMult, 1000, 25100);
            break;
        case SCREEN_FILTER_FIELD_GAMMA:
            ScreenFiltersMenu_StepChannel(filter->gamma, mult, SCREEN_FILTER_MAX_GAMMA, split, channel);
            ScreenFiltersMenu_StepChannel(otherFilter->gamma, otherMult, SCREEN_FILTER_MAX_GAMMA, split, channel);
            break;
        case SCREEN_FILTER_FIELD_CONTRAST:
            ScreenFiltersMenu_StepChannel(filter->contrast, mult, SCREEN_FILTER_MAX_CONTRAST, split, channel);
            ScreenFiltersMenu_StepChannel(otherFilter->contrast, otherMult, SCREEN_FILTER_MAX_CONTRAST, split, channel);
            break;
        case SCREEN_FILTER_FIELD_INVERT:
            filter->invert = !filter->invert;
            otherFilter->invert = sync ? !otherFilter->invert : otherFilter->invert;
            break;
    }

    ScreenFiltersMenu_ApplyColorSettings(true);
    ScreenFiltersMenu_ApplyColorSettings(false);
}

static u32 ScreenFiltersMenu_AdvancedConfigurationDrawChannels(u32 posY, const char *label, const float *vals, bool selected, bool split, u32 channel)
{
    char buf[64];

    if (!split)
    {

        floatToString(buf, vals[0], 2, true);
        return Draw_DrawFormattedString(30, posY, COLOR_WHITE, "%-13s%13s           \n", label, buf);
    }

    static const u32 channelColors[3] = { COLOR_RED, COLOR_GREEN, RGB565(0x08, 0x20, 0x1F) };

    Draw_DrawFormattedString(30, posY, COLOR_WHITE, "%-13s", label);
    u32 posX = 30 + 13 * SPACING_X;
    for (u32 ch = 0; ch < 3; ch++)
    {
        Draw_DrawCharacter(posX, posY, channelColors[ch], ch == channel && selected ? '>' : ' ');
        posX += SPACING_X;
        Draw_DrawCharacter(posX, posY, ch == channel ? COLOR_TITLE : COLOR_WHITE, "RGB"[ch]);
        posX += SPACING_X;
        floatToString(buf, vals[ch], 2, true);
        Draw_DrawFormattedString(posX, posY, COLOR_WHITE, " %s ", buf);
        posX += 6 * SPACING_X;
    }

    return posY + SPACING_Y;
}

static u32 ScreenFiltersMenu_AdvancedConfigurationHelper(const ScreenFilter *filter, int offset, int pos, u32 posY, bool split, u32 channel)
{
    Draw_DrawCharacter(10, posY, COLOR_TITLE, pos == offset + SCREEN_FILTER_FIELD_TEMPERATURE ? '>' : ' ');
    posY = Draw_DrawFormattedString(30, posY, COLOR_WHITE, Lang_Translate("Temperature: %12dK    \n"), filter->cct);

    Draw_DrawCharacter(10, posY, COLOR_TITLE, pos == offset + SCREEN_FILTER_FIELD_GAMMA ? '>' : ' ');
    posY = ScreenFiltersMenu_AdvancedConfigurationDrawChannels(posY, Lang_Translate("Gamma:"), filter->gamma, pos == offset + SCREEN_FILTER_FIELD_GAMMA, split, channel);

    Draw_DrawCharacter(10, posY, COLOR_TITLE, pos == offset + SCREEN_FILTER_FIELD_CONTRAST ? '>' : ' ');
    posY = ScreenFiltersMenu_AdvancedConfigurationDrawChannels(posY, Lang_Translate("Contrast:"), filter->contrast, pos == offset + SCREEN_FILTER_FIELD_CONTRAST, split, channel);

    Draw_DrawCharacter(10, posY, COLOR_TITLE, pos == offset + SCREEN_FILTER_FIELD_INVERT ? '>' : ' ');
    posY = Draw_DrawFormattedString(30, posY, COLOR_WHITE, Lang_Translate("Invert:      %13s    \n"), filter->invert ? Lang_Translate("true") : Lang_Translate("false"));

    return posY;
}

void ScreenFiltersMenu_AdvancedConfiguration(void)
{
    u32 posY;
    u32 input = 0;
    u32 held = 0;

    int pos = 0;
    int mult = 1;

    bool sync = true;

    bool split = !ScreenFiltersMenu_IsUnified(&topScreenFilter) || !ScreenFiltersMenu_IsUnified(&bottomScreenFilter);

    u32 channel = 0;

    do
    {
        Draw_Lock();
        Draw_DrawString(10, 10, COLOR_TITLE, Lang_Translate("Screen filters menu"));

        posY = 30;
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Use left/right to increase/decrease the sel. value.\n"));
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Hold R to change the value faster.\n"));
        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Press X to select R/G/B channel (when split).\n")) + SPACING_Y;
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang_Translate("Update both screens: %s (L to toggle)   \n"), sync ? Lang_Translate("yes") : Lang_Translate("no"));
        posY = Draw_DrawFormattedString(10, posY, COLOR_WHITE, Lang_Translate("Per-channel gamma/contrast: %s (Y to toggle)   \n"), split ? Lang_Translate("yes") : Lang_Translate("no")) + SPACING_Y;

        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Top screen:\n"));
        posY = ScreenFiltersMenu_AdvancedConfigurationHelper(&topScreenFilter, 0, pos, posY, split, channel) + SPACING_Y;

        posY = Draw_DrawString(10, posY, COLOR_WHITE, Lang_Translate("Bottom screen:\n"));
        posY = ScreenFiltersMenu_AdvancedConfigurationHelper(&bottomScreenFilter, SCREEN_FILTER_NUM_FIELDS_PER_SCREEN, pos, posY, split, channel) + SPACING_Y;

        input = waitInputWithTimeoutEx(&held, -1);
        mult = (held & KEY_R) ? 10 : 1;

        if (input & KEY_L)
            sync = !sync;
        if (input & KEY_Y)
        {
            split = !split;
            channel = 0;

            if (!split && (!ScreenFiltersMenu_IsUnified(&topScreenFilter) || !ScreenFiltersMenu_IsUnified(&bottomScreenFilter)))
            {
                ScreenFiltersMenu_Unify(&topScreenFilter);
                ScreenFiltersMenu_Unify(&bottomScreenFilter);
                ScreenFiltersMenu_ApplyColorSettings(true);
                ScreenFiltersMenu_ApplyColorSettings(false);
            }
        }
        if ((input & KEY_X) && split)
            channel = (channel + 1) % 3;
        if (input & KEY_LEFT)
            ScreenFiltersMenu_AdvancedConfigurationChangeValue(pos, -mult, sync, split, channel);
        if (input & KEY_RIGHT)
            ScreenFiltersMenu_AdvancedConfigurationChangeValue(pos, mult, sync, split, channel);
        if (input & KEY_UP)
            pos = (2 * SCREEN_FILTER_NUM_FIELDS_PER_SCREEN + pos - 1) % (2 * SCREEN_FILTER_NUM_FIELDS_PER_SCREEN);
        if (input & KEY_DOWN)
            pos = (pos + 1) % (2 * SCREEN_FILTER_NUM_FIELDS_PER_SCREEN);

        Draw_FlushFramebuffer();
        Draw_Unlock();
    }
    while(!(input & (KEY_A | KEY_B)) && !menuShouldExit);
}
