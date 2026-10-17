#include <3ds.h>
#include "paslr.h"
#include "util.h"
#include "hbldr.h"

static const u32 titleUidListForPaslr[] = {

    0x0209, 0x0219, 0x0229, 0x0269, 0x0289,
    0x0334, 0x0335, 0x0336,
    0x0343, 0x0465, 0x04B3,
    0x0913, 0x09FA, 0x0933,
            0x07FD, 0x0961,
    0x0A5D, 0x0A5E, 0x0A6F, 0x0C61, 0x0C81,
    0x0D7C, 0x0D7D, 0x0D7E,
    0x11C4,
    0x11C5,
    0x149B, 0x1746, 0x1744,
    0x1773, 0x12C1, 0x136E,
    0x17C1,
};

static u32 getRandomInt(u32 maxNum)
{

    static u64 state = 0;
    s64 tick = (s64)svcGetSystemTick();

    u64 tmp = (state >> 7) - (tick >> 2);
    state = (tmp ^ state) & 0x49CC9BA7FC61CA67ull;

    tmp = 0x5D588B656C078965ull * state + 0x29EC3;
    return (u32)(((tmp >> 32) * maxNum) >> 32);
}

static bool needsPaslr(u32 *outRegion, const ExHeader_Info *exhi)
{
    u32 region = 0;
    u16 minKernelVer = 0;
    for (u32 i = 0; i < 28; i++)
    {
        u32 desc = exhi->aci.kernel_caps.descriptors[i];
        if (desc >> 23 == 0x1FE)
            region = desc & 0xF00;
        else if (desc >> 25 == 0x7E)
            minKernelVer = (u16)desc;
    }

    *outRegion = region;

#if defined(LOADER_ENABLE_PASLR) || defined(BUILD_FOR_EXPLOIT_DEV)

    if (region != MEMOP_REGION_APP && region != MEMOP_REGION_SYSTEM)
        return false;
    else if (region == MEMOP_REGION_SYSTEM && exhi->aci.local_caps.reslimit_category == RESLIMIT_CATEGORY_LIB_APPLET)
        return false;


    if (minKernelVer >= (SYSTEM_VERSION(2, 57, 0) >> 16))
        return true;


    u64 titleId = exhi->aci.local_caps.title_id;


    if (titleId >> 46 != 0x10)
        return false;

    for (u32 i = 0; i < sizeof(titleUidListForPaslr)/sizeof(titleUidListForPaslr[0]); i++)
    {
        if (((titleId >> 8) & 0xFFFFF) == titleUidListForPaslr[i])
            return true;
    }
#else
    (void)minKernelVer;
    (void)titleUidListForPaslr;
#endif
    return false;
}

Result allocateProgramMemory(const ExHeader_Info *exhi, u32 vaddr, u32 size)
{
    Result res = 0;
    u32 region = 0;
    bool doPaslr = needsPaslr(&region, exhi);
    u32 tmp = 0;

    if (region == 0)
        return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, 1, 2);

    u32 allocFlags = MEMOP_ALLOC | region;
    if (!doPaslr)
        return svcControlMemory(&tmp, vaddr, 0, size, allocFlags, MEMPERM_READWRITE);




    u32 totalNumPages = size >> 12;
    u32 curPage = 0;
    u32 numChunks;
    u32 chunkStartAddrs[8];
    u32 chunkSizes[8];
    for (numChunks = 0; numChunks < 7; numChunks++)
    {
        u32 maxUnits = (totalNumPages - curPage) / 16;
        if (maxUnits == 0)
            break;
        else if (maxUnits > 15)
            maxUnits = 15;

        u32 numPages = 16 * (1 + getRandomInt(maxUnits));
        chunkStartAddrs[numChunks] = vaddr + (curPage << 12);
        chunkSizes[numChunks] = numPages << 12;
        curPage += numPages;
    }
    if (curPage < totalNumPages)
    {
        u32 numPages = totalNumPages - curPage;
        chunkStartAddrs[numChunks] = vaddr + (curPage << 12);
        chunkSizes[numChunks] = numPages << 12;
        curPage += numPages;
        ++numChunks;
    }


    u32 chunkOrder[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    u32 numChunksToRandomize = numChunks;
    if (totalNumPages % 16 != 0)
    {





        --numChunksToRandomize;
    }

    for (s32 i = numChunksToRandomize - 1; i >= 0; i--)
    {
        u32 j = getRandomInt(i + 1);
        tmp = chunkOrder[i];
        chunkOrder[i] = chunkOrder[j];
        chunkOrder[j] = tmp;
    }


    u32 i;
    for (i = 0; i < numChunks; i++)
    {
        u32 idx = chunkOrder[i];
        res = svcControlMemory(&tmp, chunkStartAddrs[idx], 0, chunkSizes[idx], allocFlags, MEMPERM_READWRITE);
        if (R_FAILED(res))
            break;
    }


    if (i == numChunks)
        return res;


    for (u32 j = 0; j < i; j++)
    {
        u32 idx = chunkOrder[i];
        svcControlMemory(&tmp, chunkStartAddrs[idx], 0, chunkSizes[idx], MEMOP_FREE, MEMPERM_DONTCARE);
    }

    return res;
}
