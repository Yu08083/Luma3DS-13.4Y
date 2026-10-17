/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2021 Aurora Wright, TuxSH
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




#include "emunand.h"
#include "memory.h"
#include "utils.h"
#include "fatfs/sdmmc/sdmmc.h"
#include "large_patches.h"

u32 emuOffset,
    emuHeader;

void locateEmuNand(FirmwareSource *nandType, u32 *emunandIndex, bool configureCtrNandParams)
{
    static u8 __attribute__((aligned(4))) temp[0x200];
    static u32 nandSize = 0,
               fatStart;

    if(!nandSize)
    {
        nandSize = getMMCDevice(0)->total_size;
        sdmmc_sdcard_readsectors(0, 1, temp);
        fatStart = *(u32 *)(temp + 0x1C6);
    }



    for(u32 i = 0; i < 3; i++)
    {
        static const u32 roundedMinsizes[] = {0x1D8000, 0x26E000};

        u32 nandOffset;
        switch(i)
        {
            case 1:
                nandOffset = ROUND_TO_4MB(nandSize + 1);
                break;
            case 2:
                nandOffset = roundedMinsizes[ISN3DS ? 1 : 0];
                break;
            case 0:
                nandOffset = nandSize > 0x200000 ? 0x400000 : 0x200000;
                break;
        }

        nandOffset *= *emunandIndex;

        if(fatStart >= nandOffset + roundedMinsizes[ISN3DS ? 1 : 0])
        {

            if(!sdmmc_sdcard_readsectors(nandOffset + 1, 1, temp) && memcmp(temp + 0x100, "NCSD", 4) == 0)
            {
                if (configureCtrNandParams)
                {
                    emuOffset = nandOffset + 1;
                    emuHeader = 0;
                }
                return;
            }


            else if(i != 2 && !sdmmc_sdcard_readsectors(nandOffset + nandSize, 1, temp) && memcmp(temp + 0x100, "NCSD", 4) == 0)
            {
                if (configureCtrNandParams)
                {
                    emuOffset = nandOffset;
                    emuHeader = nandSize;
                }
                return;
            }
        }

        if(*emunandIndex == 0) break;
    }


    if(*emunandIndex != 0)
    {
        *emunandIndex = 0;
        locateEmuNand(nandType, emunandIndex, configureCtrNandParams);
    }
    else *nandType = FIRMWARE_SYSNAND;
}

static inline u32 getProtoSdmmc(u32 *sdmmc, u32 firmVersion)
{
    switch(firmVersion)
    {
        case 243:
            *sdmmc = (0x080AAA28 + 0x4e0);
            break;
        case 236:
                *sdmmc = (0x080CED30 + 0x690);
                break;
        case 238:
            *sdmmc = (0x080BEA70 + 0x690);
            break;
        case 1200:
            *sdmmc = (0x080C6A00 + 0x8DC);
             break;
        default:
            return 1;
    }

    return 0;
}

static inline u32 getOldSdmmc(u32 *sdmmc, u32 firmVersion)
{
    switch(firmVersion)
    {
        case 0x18:
            *sdmmc = 0x080D91D8;
            break;
        case 0x1D:
        case 0x1F:
            *sdmmc = 0x080D8CD0;
            break;
        default:
            return 1;
    }

    return 0;
}

static inline u32 getSdmmc(u8 *pos, u32 size, u32 *sdmmc)
{

    static const u8 pattern[] = {0x21, 0x20, 0x18, 0x20};

    const u8 *off = memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    *sdmmc = *(u32 *)(off + 9) + *(u32 *)(off + 0xD);

    return 0;
}

static inline u32 patchNandRw(u8 *pos, u32 size, u32 hookAddr)
{

    static const u8 pattern[] = {0x1E, 0x00, 0xC8, 0x05};

    u16 *readOffset = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(readOffset == NULL) return 1;

    readOffset -= 3;

    u16 *writeOffset = (u16 *)memsearch((u8 *)(readOffset + 5), pattern, 0x100, sizeof(pattern));

    if(writeOffset == NULL) return 1;

    writeOffset -= 3;
    *readOffset = *writeOffset = 0x4C00;
    readOffset[1] = writeOffset[1] = 0x47A0;
    ((u32 *)writeOffset)[1] = ((u32 *)readOffset)[1] = hookAddr;

    return 0;
}

static inline u32 patchProtoNandRw(u8 *pos, u32 size, u32 hookAddr, u32 hookCidAddr)
{

    static const u8 pattern[] = {
        0x03, 0x00, 0x51, 0xE3,
        0x02, 0xC0, 0xA0, 0xE1,
        0x04, 0x00, 0x80, 0xE2,
    };

    u32 *writeOffset = (u32 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(writeOffset == NULL) return 1;

    u32 *readOffset = (u32 *)memsearch((u8 *)(writeOffset + 3), pattern, 0x400, sizeof(pattern));

    if(readOffset == NULL) return 1;


    static const u8 mount_pattern[] = {
        0x20, 0x00, 0x84, 0xE2,
        0x01, 0x20, 0xA0, 0xE3,
        0x00, 0x10, 0xA0, 0xE3,
    };
    u32* mountOffset = (u32*) memsearch(pos, mount_pattern, size, sizeof(mount_pattern));
    if (mountOffset == NULL) return 1;


    static const u8 readcid_pattern[] = {
        0x31, 0xFF, 0x2F, 0xE1,
        0x20, 0x60, 0x9F, 0xE5,
        0x00, 0x00, 0x50, 0xE3,
    };
    u32* readCidOffset = (u32*) memsearch(pos, readcid_pattern, size, sizeof(readcid_pattern));
    if (readCidOffset == NULL) return 1;
    readCidOffset -= 5;

    mountOffset[1] = 0xe3a02000;

    readOffset[0] = writeOffset[0] = 0xe52de004;
    readOffset[1] = writeOffset[1] = 0xe59fc000;
    readOffset[2] = writeOffset[2] = 0xe12fff3c;
    readOffset[3] = writeOffset[3] = hookAddr;

    readCidOffset[0] = 0xe59fc000;
    readCidOffset[1] = 0xe12fff3c;
    readCidOffset[2] = hookCidAddr;


    sdmmc_get_cid(1, emunandPatchNandCid);

    return 0;
}

static inline u32 patchProtoNandRw236(u8 *pos, u32 size, u32 hookAddr, u32 hookCidAddr)
{

    static const u8 pattern[] = {
        0x03, 0x00, 0x50, 0xE3,
        0x00, 0x00, 0xA0, 0x13,
        0x01, 0x00, 0xA0, 0x03,
    };

    u32 *writeOffset = (u32 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(writeOffset == NULL) return 1;

    u32 *readOffset = (u32 *)memsearch((u8 *)(writeOffset + 3), pattern, 0x400, sizeof(pattern));

    if(readOffset == NULL) return 1;


    static const u8 mount_pattern[] = {
        0x30, 0xED, 0x0C, 0x08,
        0x01, 0x01, 0x00, 0x00,
    };
    u8* mountOffset = (u8*) memsearch(pos, mount_pattern, size, sizeof(mount_pattern));
    if (mountOffset == NULL) return 1;
    mountOffset += 4;


    static const u8 readcid_pattern[] = {
        0x31, 0xFF, 0x2F, 0xE1,
        0x20, 0x60, 0x9F, 0xE5,
        0x00, 0x00, 0x50, 0xE3,
    };
    u32* readCidOffset = (u32*) memsearch(pos, readcid_pattern, size, sizeof(readcid_pattern));
    if (readCidOffset == NULL) return 1;
    readCidOffset -= 5;

    *(u32*)mountOffset = 0x300;

    readOffset[0] = writeOffset[0] = 0xe59fc000;
    readOffset[1] = writeOffset[1] = 0xe12fff3c;
    readOffset[2] = writeOffset[2] = hookAddr;

    readCidOffset[0] = 0xe59fc000;
    readCidOffset[1] = 0xe12fff3c;
    readCidOffset[2] = hookCidAddr;


    sdmmc_get_cid(1, emunandPatchNandCid);

    return 0;
}

static inline u32 patchProtoNandRw238(u8 *pos, u32 size, u32 hookAddr, u32 hookCidAddr)
{

    static const u8 pattern[] = {
        0x03, 0x00, 0x50, 0xE3,
        0x00, 0x00, 0xA0, 0x13,
        0x01, 0x00, 0xA0, 0x03,
    };

    u32 *writeOffset = (u32 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(writeOffset == NULL) return 1;

    u32 *readOffset = (u32 *)memsearch((u8 *)(writeOffset + 3), pattern, 0x400, sizeof(pattern));

    if(readOffset == NULL) return 1;


    static const u8 mount_pattern[] = {
        0x08,
        0x01, 0x01, 0x00, 0x00,
    };
    u8* mountOffset = (u8*) memsearch(pos, mount_pattern, size, sizeof(mount_pattern));
    if (mountOffset == NULL) return 1;
    mountOffset++;


    static const u8 readcid_pattern[] = {
        0x31, 0xFF, 0x2F, 0xE1,
        0x20, 0x60, 0x9F, 0xE5,
        0x00, 0x00, 0x50, 0xE3,
    };
    u32* readCidOffset = (u32*) memsearch(pos, readcid_pattern, size, sizeof(readcid_pattern));
    if (readCidOffset == NULL) return 1;
    readCidOffset -= 5;

    *(u32*)mountOffset = 0x300;

    readOffset[0] = writeOffset[0] = 0xe59fc000;
    readOffset[1] = writeOffset[1] = 0xe12fff3c;
    readOffset[2] = writeOffset[2] = hookAddr;

    readCidOffset[0] = 0xe59fc000;
    readCidOffset[1] = 0xe12fff3c;
    readCidOffset[2] = hookCidAddr;


    sdmmc_get_cid(1, emunandPatchNandCid);

    return 0;
}

static inline u32 patchProtoNandRw1200(u8 *pos, u32 size, u32 hookAddr, u32 hookCidAddr)
{

    static const u8 pattern[] = {
        0x03, 0x00, 0x50, 0xE3,
        0x14, 0xD0, 0x4D, 0xE2,
        0x00, 0x70, 0xA0, 0xE3,
    };

    u32 *writeOffset = (u32 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(writeOffset == NULL) return 1;

    u32 *readOffset = (u32 *)memsearch((u8 *)(writeOffset + 3), pattern, 0x400, sizeof(pattern));

    if(readOffset == NULL) return 1;


    static const u8 mount_pattern[] = {
        0xE8,
        0x01, 0x01, 0x00, 0x00,
        0x5C
    };
    u8 *mountOffset = (u8 *)memsearch(pos, mount_pattern, size, sizeof(mount_pattern));
    if (mountOffset == NULL) return 1;
    ++mountOffset;

    static const u8 readcid_pattern[] = {
        0x31, 0xFF, 0x2F, 0xE1,
        0x00, 0x00, 0x50, 0xE3,
        0x34, 0x00, 0x9F, 0x05,
    };
    u32 *readCidOffset = (u32 *)memsearch(pos, readcid_pattern, size, sizeof(readcid_pattern));
    if (readCidOffset == NULL) return 1;
    readCidOffset -= 5;

    *(u32 *)mountOffset = 0x100;

    readOffset[0] = writeOffset[0] = 0xe59fc000;
    readOffset[1] = writeOffset[1] = 0xe12fff3c;
    readOffset[2] = writeOffset[2] = hookAddr;

    readCidOffset[0] = 0xe59fc000;
    readCidOffset[1] = 0xe12fff3c;
    readCidOffset[2] = hookCidAddr;


    sdmmc_get_cid(1, emunandPatchNandCid);

    return 0;
}

u32 patchEmuNand(u8 *process9Offset, u32 process9Size, u32 firmVersion)
{
    u32 ret = 0;


    emunandPatchNandOffset = emuOffset;
    emunandPatchNcsdHeaderOffset = emuHeader;


    u32 sdmmc;
    ret += !ISN3DS && firmVersion < 0x25 ? getOldSdmmc(&sdmmc, firmVersion) : getSdmmc(process9Offset, process9Size, &sdmmc);
    if(!ret) emunandPatchSdmmcStructPtr = sdmmc;


    ret += patchNandRw(process9Offset, process9Size, (u32)emunandPatch);

    return ret;
}

u32 patchProtoEmuNand(u8 *process9Offset, u32 process9Size)
{
    extern u32 firmProtoVersion;
    u32 ret = 0;


    emunandPatchNandOffset = emuOffset;
    emunandPatchNcsdHeaderOffset = emuHeader;


    u32 sdmmc;
    ret += getProtoSdmmc(&sdmmc, firmProtoVersion);
    if(!ret) emunandPatchSdmmcStructPtr = sdmmc;


    switch (firmProtoVersion) {
        case 243:
            ret += patchProtoNandRw(process9Offset, process9Size, (u32)emunandProtoPatch, (u32)emunandProtoCidPatch);
            break;
        case 238:
            ret += patchProtoNandRw238(process9Offset, process9Size, (u32)emunandProtoPatch238, (u32)emunandProtoCidPatch);
            break;
        case 236:
            ret += patchProtoNandRw236(process9Offset, process9Size, (u32)emunandProtoPatch238, (u32)emunandProtoCidPatch);
            break;
        case 1200:
            ret += patchProtoNandRw1200(process9Offset, process9Size, (u32)emunandProtoPatch1200, (u32)emunandProtoCidPatch1200);
            break;
        default:
            ret++;
            break;
    }











    memset((void *)0x01FFB900, 0x0, 0x200);

    return ret;
}
