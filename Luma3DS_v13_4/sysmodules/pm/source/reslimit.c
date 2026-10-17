#include <3ds.h>
#include "reslimit.h"
#include "util.h"
#include "manager.h"
#include "luma.h"

#define CPUTIME_MULTI_MASK      BIT(7)
#define CPUTIME_SINGLE_MASK     0

typedef s64 ReslimitValues[10];

static const ResourceLimitType g_reslimitInitOrder[10] = {
    RESLIMIT_COMMIT,
    RESLIMIT_PRIORITY,
    RESLIMIT_THREAD,
    RESLIMIT_EVENT,
    RESLIMIT_MUTEX,
    RESLIMIT_SEMAPHORE,
    RESLIMIT_TIMER,
    RESLIMIT_SHAREDMEMORY,
    RESLIMIT_ADDRESSARBITER,
    RESLIMIT_CPUTIME,
};

static u32 g_currentAppMemLimit = 0, g_defaultAppMemLimit;

static ReslimitValues g_o3dsReslimitValues[4] = {

    {
        0x4000000,
        0x18,
        32,
        32,
        32,
        8,
        8,
        16,
        2,
        0,
    },


    {
        0x2606000,
        4,
        14,
        8,
        8,
        4,
        4,
        8,
        3,
        10000,
    },


    {
        0x0602000,
        4,
        14,
        8,
        8,
        4,
        4,
        8,
        1,
        10000,
    },


    {
        0x1682000,
        4,
        202,
        248,
        35,
        64,
        43,
        30,
        43,
        1000,
    },
};

static ReslimitValues g_n3dsReslimitValues[4] = {

    {
        0x7C00000,
        0x18,
        32,
        32,
        32,
        8,
        8,
        16,
        2,
        0,
    },


    {
        0x5E06000,
        4,
        29,
        11,
        8,
        4,
        4,
        8,
        3,
        10000,
    },


    {
        0x0602000,
        4,
        14,
        8,
        8,
        4,
        4,
        8,
        1,
        10000,
    },


    {
        0x2182000,
        4,
        225,
        264,
        37,
        67,
        44,
        31,
        45,
        1000,
    },
};



static const struct {
    u32 titleUid;
    u32 value;
} g_startCpuTimeOverrides[] = {

    { 0x205, 10000 },
    { 0x215, 10000 },
    { 0x225, 10000 },
    { 0x304, 10000 },
    { 0x32E, 10000 },
    { 0x334, 30    },
    { 0x335, 30    },
    { 0x336, 30    },
    { 0x348, 10000 },
    { 0x349, 10000 },
    { 0x368, 10000 },
    { 0x370, 10000 },
    { 0x389, 10000 },
    { 0x490, 10000 },
    { 0x491, 10000 },
    { 0x562, 10000 },
};

static ReslimitValues *fixupReslimitValues(void)
{





    u32 appmemalloc = OS_KernelConfig->memregion_sz[0];
    u32 sysmemalloc = OS_KernelConfig->memregion_sz[1] + (hasKExt() ? getStolenSystemMemRegionSize() : 0);
    ReslimitValues *values = !IS_N3DS ? g_o3dsReslimitValues : g_n3dsReslimitValues;

    static const u32 minAppletMemAmount = 0x1200000;
    u32 defaultMemAmount = !IS_N3DS ? 0x2C00000 : 0x6400000;
    u32 otherMinOvercommitAmount = !IS_N3DS ? 0x280000 : 0x180000;
    u32 baseRegionSize = !IS_N3DS ? 0x1400000 : 0x2000000;

    if (sysmemalloc < minAppletMemAmount) {
        values[1][0] = sysmemalloc - minAppletMemAmount / 3;
        values[2][0] = 0;
        values[3][0] = baseRegionSize + otherMinOvercommitAmount;
    } else {
        u32 excess = sysmemalloc < defaultMemAmount ? 0 : sysmemalloc - defaultMemAmount;
        values[1][0] = 3 * excess / 4 + sysmemalloc - minAppletMemAmount / 3;
        values[2][0] = 1 * excess / 4 + minAppletMemAmount / 3;
        values[3][0] = baseRegionSize + (otherMinOvercommitAmount + excess / 4);
    }

    values[0][0] = appmemalloc;
    g_defaultAppMemLimit = appmemalloc;

    return values;
}

Result initializeReslimits(void)
{
    Result res = 0;
    ReslimitValues *values = fixupReslimitValues();
    for (u32 i = 0; i < 4; i++) {
        TRY(svcCreateResourceLimit(&g_manager.reslimits[i]));
        TRY(svcSetResourceLimitValues(g_manager.reslimits[i], g_reslimitInitOrder, values[i], 10));
    }

    return res;
}

Result setAppMemLimit(u32 limit)
{
    if (g_currentAppMemLimit == g_defaultAppMemLimit) {
        return 0;
    }

    ResourceLimitType category = RESLIMIT_COMMIT;
    s64 value = limit | ((u64)limit << 32);
    return svcSetResourceLimitValues(g_manager.reslimits[0], &category, &value, 1);
}

Result resetAppMemLimit(void)
{
    return setAppMemLimit(g_defaultAppMemLimit);
}

Result setAppCpuTimeLimit(s64 limit)
{


    if (IS_N3DS && CONFIG(REDIRECTAPPTHREADS))
    {
        s64 disableThreadRedir = 0;
        if (R_SUCCEEDED(svcGetSystemInfo(&disableThreadRedir, 0x10000, 0x181)) && !disableThreadRedir)
            return 0;
    }

    ResourceLimitType category = RESLIMIT_CPUTIME;
    return svcSetResourceLimitValues(g_manager.reslimits[0], &category, &limit, 1);
}

static Result getAppCpuTimeLimit(s64 *limit)
{
    ResourceLimitType category = RESLIMIT_CPUTIME;
    return svcGetResourceLimitLimitValues(limit, g_manager.reslimits[0], &category, 1);
}

void setAppCpuTimeLimitAndSchedModeFromDescriptor(u64 titleId, u16 descriptor)
{

    u8 cpuTime = (u8)descriptor;
    assertSuccess(setAppCpuTimeLimit(0));

    g_manager.cpuTimeBase = 0;
    u32 currentValueToSet = g_manager.cpuTimeBase;

    if (cpuTime == 0) {

        u32 titleUid = ((u32)titleId >> 8) & 0xFFFFF;


        cpuTime = CPUTIME_SINGLE_MASK | 80;

        static const u32 numOverrides = sizeof(g_startCpuTimeOverrides) / sizeof(g_startCpuTimeOverrides[0]);

        if (titleUid >= g_startCpuTimeOverrides[0].titleUid && titleUid <= g_startCpuTimeOverrides[numOverrides - 1].titleUid) {
            u32 i = 0;
            for (u32 i = 0; i < numOverrides && titleUid < g_startCpuTimeOverrides[i].titleUid; i++);
            if (i < numOverrides) {
                if (g_startCpuTimeOverrides[i].value > 100 && g_startCpuTimeOverrides[i].value < 200) {
                    cpuTime = CPUTIME_MULTI_MASK | 80;
                    currentValueToSet = g_startCpuTimeOverrides[i].value - 100;
                } else {
                    cpuTime = CPUTIME_SINGLE_MASK | 80;
                    currentValueToSet = g_startCpuTimeOverrides[i].value;
                }
            }
        }
    }


    assertSuccess(svcKernelSetState(6, 3, (cpuTime & CPUTIME_MULTI_MASK) ? 0LL : 1LL));


    g_manager.maxAppCpuTime = cpuTime & 0x7F;


    if (currentValueToSet != 0) {
        assertSuccess(setAppCpuTimeLimit(currentValueToSet));
    }
}

Result SetAppResourceLimit(u32 mbz, ResourceLimitType category, u32 value, u64 mbz2)
{
    if (mbz != 0 || mbz2 != 0 || category != RESLIMIT_CPUTIME || value > (u32)g_manager.maxAppCpuTime) {
        return 0xD8E05BF4;
    }

    value += value < 5 ? 0 : g_manager.cpuTimeBase;
    return setAppCpuTimeLimit(value);
}

Result GetAppResourceLimit(s64 *value, u32 mbz, ResourceLimitType category, u32 mbz2, u64 mbz3)
{
    if (mbz != 0 || mbz2 != 0 || mbz3 != 0 || category != RESLIMIT_CPUTIME) {
        return 0xD8E05BF4;
    }

    Result res = getAppCpuTimeLimit(value);
    if (R_SUCCEEDED(res) && *value >= 5) {
        *value = *value >= g_manager.cpuTimeBase ? *value - g_manager.cpuTimeBase : 0;
    }

    return res;
}
