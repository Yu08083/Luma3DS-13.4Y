/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2020, 2022 Aurora Wright, TuxSH, fincs
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
#include <string.h>
#include <assert.h>
#include "paslr.h"
#include "util.h"
#include "hbldr.h"
#include "3dsx.h"

extern bool isN3DS;



#if __GNUC__ >= 15

__attribute__((nonstring))
#endif
static const char serviceList[34][8] =
{
    "APT:U",
    "ac:u",
    "am:net",
    "boss:P",
    "cam:u",
    "cfg:nor",
    "cfg:u",
    "csnd:SND",
    "dsp::DSP",
    "fs:USER",
    "gsp::Lcd",
    "gsp::Gpu",
    "hid:USER",
    "http:C",
    "ir:USER",
    "ir:rst",
    "ir:u",
    "ldr:ro",
    "mic:u",
    "ndm:u",
    "news:s",
    "nim:s",
    "ns:s",
    "nwm::UDS",
    "nwm::EXT",
    "ptm:u",
    "ptm:sysm",
    "pxi:dev",
    "qtm:u",
    "soc:U",
    "ssl:C",
    "y2r:u",


    "nfc:u",
    "mvd:STD",
};






static const s16 dependencyListNativeFirmUids[] =
{
    0x1B,
    0x1E,
    0x21,
    0x23,

    0x1F,
    0x31,
    0x17,

    0x22,
    0x80,




    0x18,
    0x1D,
    0x27,
    0x16,
    -0x42,
    0x1C,

    0x1A,
    0x20,
    0x33,

    0x2D,
    0x2E,
    0x2F,
    0x29,
    0x24,

    0x32,

    0x15,

    0x26,
    0x34,
    0x2C,
    0x2B,
    0x35,

    -0x41,
    -0x40,

    0x38,
    0x37,
    0x28,
    0x2A,
};

static const s16 dependencyListSafeFirmUids[] =
{
    0x1B,
    0x1E,
    0x21,
    0x23,

    0x1F,
    0x31,
    0x17,

    0x22,
    0x80,




    0x18,
    0x1D,
    0x27,
    0x1C,

    0x1A,
    0x33,

    0x2D,
    0x2E,
    0x2F,
    0x29,
    0x24,

    0x32,

    0x15,
};

static const u32 kernelCaps[] =
{
    0xFC00022C,



    0xFF81FF00,
    0xFF81FF80,

    0xFF91F000,
    0xFF91F600,


    0xFF81EC00,
    0xFF81F000,

    0xFF002109,
    0xFE000200,


    0xFFE1EC40,
    0xF0FFFFFF,
    0xF1FFFFFF,
    0xF2FFFFFF,
    0xF3FFFFFF,
    0xF4FFFFFF,
    0xF5FFFFFF,
    0xF6FFFFFF,
    0xF7FFFFFF,
};

static u16 hbldrTarget[PATH_MAX+1];

static inline void error(u32* cmdbuf, Result rc)
{
    cmdbuf[0] = IPC_MakeHeader(0, 1, 0);
    cmdbuf[1] = rc;
}

static u16 *u16_strncpy(u16 *dest, const u16 *src, u32 size)
{
    u32 i;
    for (i = 0; i < size && src[i] != 0; i++)
        dest[i] = src[i];
    while (i < size)
        dest[i++] = 0;

    return dest;
}

void hbldrPatchExHeaderInfo(ExHeader_Info *exhi)
{
    u32 stacksize = 4096;
    memcpy(exhi->sci.codeset_info.name, "3dsx_app", 8);
    memcpy(&exhi->sci.codeset_info.stack_size, &stacksize, 4);
    memset(&exhi->sci.dependencies, 0, sizeof(exhi->sci.dependencies));

    u32 coreVer = OS_KernelConfig->kernel_syscore_ver;
    u32 deplistSize;
    const s16 *deplist;
    switch (coreVer) {
        case 2:
            deplist = dependencyListNativeFirmUids;
            deplistSize = sizeof(dependencyListNativeFirmUids)/sizeof(dependencyListNativeFirmUids[0]);
            break;
        case 3:
            deplist = dependencyListSafeFirmUids;
            deplistSize = sizeof(dependencyListSafeFirmUids)/sizeof(dependencyListSafeFirmUids[0]);
            break;
        default:
            panic(0);
            break;
    }

    for (u32 i = 0; i < deplistSize; i++)
    {
        static const u64 mask = 0x0004013000000000ull;
        bool n3dsOnly = deplist[i] < 0;
        u32 n3dsMask = n3dsOnly ? 0x20000000 : 0;
        u32 uid = n3dsOnly ? -deplist[i] : deplist[i];


        if (uid == 0x40 && GET_VERSION_MINOR(osGetKernelVersion()) >= 48)
            n3dsMask = 0;

        exhi->sci.dependencies[i] = mask | n3dsMask | (uid << 8) | coreVer;
    }

    ExHeader_Arm11SystemLocalCapabilities* localcaps0 = &exhi->aci.local_caps;

    localcaps0->core_info.core_version = coreVer;
    localcaps0->core_info.use_cpu_clockrate_804MHz = false;
    localcaps0->core_info.enable_l2c = false;
    localcaps0->core_info.ideal_processor = 0;
    localcaps0->core_info.affinity_mask = BIT(0);
    localcaps0->core_info.priority = 0x30;

    u32 appmemtype = OS_KernelConfig->app_memtype;
    localcaps0->core_info.o3ds_system_mode = appmemtype < 6 ? (SystemMode)appmemtype : SYSMODE_O3DS_PROD;
    localcaps0->core_info.n3ds_system_mode = appmemtype >= 6 ? (SystemMode)(appmemtype - 6 + 1) : SYSMODE_N3DS_PROD;

    memset(localcaps0->reslimits, 0, sizeof(localcaps0->reslimits));



    localcaps0->reslimits[0] = BIT(7) | 89;

    localcaps0->storage_info.fs_access_info = 0xFFFFFFFF;
    localcaps0->storage_info.no_romfs = true;
    localcaps0->storage_info.use_extended_savedata_access = true;



    u32 numServices = GET_VERSION_MINOR(osGetKernelVersion()) < 50 || coreVer != 2 ? 32 : 34;
    memset(localcaps0->service_access, 0, sizeof(localcaps0->service_access));
    memcpy(localcaps0->service_access, serviceList, 8 * numServices);

    localcaps0->reslimit_category = RESLIMIT_CATEGORY_APPLICATION;

    ExHeader_Arm11KernelCapabilities* kcaps0 = &exhi->aci.kernel_caps;
    memset(kcaps0->descriptors, 0xFF, sizeof(kcaps0->descriptors));
    memcpy(kcaps0->descriptors, kernelCaps, sizeof(kernelCaps));


    kcaps0->descriptors[0] = 0xFC000000 | (osGetKernelVersion() >> 16);
}

Result hbldrLoadProcess(Handle *outProcessHandle, const ExHeader_Info *exhi)
{
    IFile file;
    Result res;

    const ExHeader_CodeSetInfo *csi = &exhi->sci.codeset_info;

    u32 region = 0;
    u32 count;
    for (count = 0; count < 28; count++)
    {
        u32 desc = exhi->aci.kernel_caps.descriptors[count];
        if (0x1FE == desc >> 23)
            region = desc & 0xF00;
    }
    if (region == 0)
            return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, 1, 2);


    if (hbldrTarget[0] == 0)
    {
        u16_strncpy(hbldrTarget, u"/boot.3dsx", PATH_MAX);
        ldrArgvBuf[0] = 1;
        strncpy((char*)&ldrArgvBuf[1], "sdmc:/boot.3dsx", sizeof(ldrArgvBuf)-4);
    }

    res = IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_UTF16, hbldrTarget), FS_OPEN_READ);
    hbldrTarget[0] = 0;
    if (R_FAILED(res))
        return res;

    u32 totalSize = 0;
    if (!Ldr_Get3dsxSize(&totalSize, &file))
    {
        IFile_Close(&file);
        return (Result)-1;
    }

    u32 tmp = 0;
    u32 addr = 0x10000000;
    res = allocateProgramMemory(exhi, addr, totalSize);
    if (R_FAILED(res))
    {
        IFile_Close(&file);
        return res;
    }

    Handle hCodeset = Ldr_CodesetFrom3dsx(csi->name, (u32 *)addr, csi->text.address, &file, exhi->aci.local_caps.title_id);
    IFile_Close(&file);

    if (hCodeset != 0)
    {

        res = svcCreateProcess(outProcessHandle, hCodeset, exhi->aci.kernel_caps.descriptors, count);
        svcCloseHandle(hCodeset);
    }
    else
        res = MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_LDR, RD_NOT_FOUND);

    svcControlMemory(&tmp, addr, 0, totalSize, MEMOP_FREE, 0);
    return res;
}

void hbldrHandleCommands(void *ctx)
{
    (void)ctx;
    u32 *cmdbuf = getThreadCommandBuffer();
    switch (cmdbuf[0] >> 16)
    {
        case 2:
        {
            if (cmdbuf[0] != IPC_MakeHeader(2, 0, 2) || (cmdbuf[1] & 0x3FFF) != 0x0002)
            {
                error(cmdbuf, 0xD9001830);
                break;
            }
            size_t inSize = cmdbuf[1] >> 14;
            inSize = inSize > PATH_MAX ? inSize : PATH_MAX;
            ssize_t units = utf8_to_utf16(hbldrTarget, (const uint8_t*)cmdbuf[2], inSize);
            if (units < 0 || units > PATH_MAX)
            {
                hbldrTarget[0] = 0;
                error(cmdbuf, 0xD9001830);
                break;
            }

            hbldrTarget[units] = 0;
            cmdbuf[0] = IPC_MakeHeader(2, 1, 0);
            cmdbuf[1] = 0;
            break;
        }
        case 3:
        {
            if (cmdbuf[0] != IPC_MakeHeader(3, 0, 2) || (cmdbuf[1] & 0x3FFF) != (0x2 | (1<<10)))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }
            size_t inSize = cmdbuf[1] >> 14;
            inSize = inSize > ARGVBUF_SIZE ? inSize : ARGVBUF_SIZE;
            memcpy(ldrArgvBuf, (const u8 *)cmdbuf[2], inSize);
            cmdbuf[0] = IPC_MakeHeader(3, 1, 0);
            cmdbuf[1] = 0;
            break;
        }
        case 1:
        case 4:
        case 5:
        default:
        {
            error(cmdbuf, 0xD900182F);
            break;
        }
    }
}
