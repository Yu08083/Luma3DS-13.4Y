/*
*   This file is part of Luma3DS
*   Copyright (C) 2022 Aurora Wright, TuxSH
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

#include "deliver_arg.h"
#include "utils.h"
#include "memory.h"
#include "config.h"
#include "fs.h"
#include "i2c.h"
#include "screen.h"

u8 *loadDeliverArg(void)
{
    static __attribute__((aligned(8))) u8 deliverArg[0x1000] = {0};
    static bool deliverArgLoaded = false;

    if (!deliverArgLoaded)
    {
        u32 bootenv = CFG_BOOTENV;
        if ((bootenv & 1) == 0)
        {
            memset(deliverArg, 0, 0x1000);
        }
        else
        {
            u32 mode = bootenv >> 1;
            if (mode == 0)
            {
                memcpy(deliverArg, (const void *)0x20000000, 0x1000);


                u32 testPattern = *(u32 *)(deliverArg + 0x438);
                u32 *crcPtr = (u32 *)(deliverArg + 0x43C);
                u32 crc = *crcPtr;
                *crcPtr = 0;
                u32 expectedCrc = crc32(deliverArg + 0x400, 0x140, 0xFFFFFFFF);
                *crcPtr = crc;
                if (testPattern != 0xFFFF || crc != expectedCrc)
                    memset(deliverArg, 0, 0x1000);
            }
            else
            {

                copyFromLegacyModeFcram(deliverArg, (const void *)0x20000000, 0x400);



                u8 *tlnc = deliverArg + 0x300;
                bool hasMagic = memcmp(tlnc, "TLNC", 4) == 0;
                u8 crcLen = tlnc[5];
                u16 crc = *(u16 *)(tlnc + 6);
                if (!hasMagic || (8 + crcLen) > 0x100 || crc != crc16(tlnc + 8, crcLen, 0xFFFF))
                    memset(tlnc, 0, 0x100);

                memset(deliverArg + 0x400, 0, 0xC00);
            }
        }
        deliverArgLoaded = true;
    }

    return deliverArg;
}

void commitDeliverArg(void)
{
    u8 *deliverArg = loadDeliverArg();
    u32 bootenv = CFG_BOOTENV;

    if ((bootenv & 1) == 0)
    {
        bootenv = 1;
        CFG_BOOTENV = 1;
    }

    u32 mode = bootenv >> 1;
    if (mode == 0)
    {
        *(u32 *)(deliverArg + 0x438) = 0xFFFF;
        *(u32 *)(deliverArg + 0x43C) = 0;
        *(u32 *)(deliverArg + 0x43C) = crc32(deliverArg + 0x400, 0x140, 0xFFFFFFFF);
        memcpy((void *)0x20000000, deliverArg, 0x1000);
    }
    else
    {
        copyToLegacyModeFcram((void *)0x20000000, deliverArg, 0x400);
    }
}

bool hasValidTlncAutobootParams(void)
{
    u8 *tlnc = loadDeliverArg() + 0x300;
    return memcmp(tlnc, "TLNC", 4) == 0 && (*(u16 *)(tlnc + 0x18) & 1) != 0;
}

bool isTwlToCtrLaunch(void)
{

    u8 *tlnc = loadDeliverArg() + 0x300;
    u64 twlTid = *(u64 *)(tlnc + 0x10);

    switch (twlTid & ~0xFFull)
    {
        case 0x0000000000000000ull:
        case 0x00030015484E4200ull:
            return true;
        default:
            return false;
    }
}

static bool configureHomebrewAutobootCtr(u8 *deliverArg)
{
    static const u8 appmemtypesO3ds[] = { 0, 2, 3, 4, 5 };
    static const u8 appmemtypesN3ds[] = { 6, 7, 7, 7, 7 };

    u64 hbldrTid = configData.hbldr3dsxTitleId;
    hbldrTid = hbldrTid == 0 ? HBLDR_DEFAULT_3DSX_TID : hbldrTid;
    if ((hbldrTid >> 46) != 0x10)
        return false;

    u8 memtype = configData.autobootCtrAppmemtype;

    memtype = memtype >= 5 ? 0 : memtype;
    deliverArg[0x400] = ISN3DS ? appmemtypesN3ds[memtype] : appmemtypesO3ds[memtype];


    u32 category = (hbldrTid >> 32) & 0xFFFF;
    bool isSdApp = (category & 0x10) == 0 && category != 1;
    *(u64 *)(deliverArg + 0x440) = hbldrTid;
    *(u64 *)(deliverArg + 0x448) = isSdApp ? 1 : 0;


    *(u32 *)(deliverArg + 0x460) = (0 << 1) | (1 << 0);















    I2C_writeReg(I2C_DEV_MCU, 0x60, 0);
    u8 flags = I2C_readReg(I2C_DEV_MCU, 0x61);
    flags &= ~4;
    I2C_writeReg(I2C_DEV_MCU, 0x60, 0);
    I2C_writeReg(I2C_DEV_MCU, 0x61, flags);

    CFG_BOOTENV = 1;

    return true;
}

static bool configureHomebrewAutobootTwl(u8 *deliverArg)
{



    memset(deliverArg + 0x000, 0, 0x300);


    u8 *tlnc = deliverArg + 0x300;
    memset(tlnc, 0, 0x100);
    memcpy(tlnc, "TLNC", 4);
    tlnc[4] = 1;
    tlnc[5] = 0x18;

    *(u64 *)(tlnc + 8) = 0;
    *(u64 *)(tlnc + 0x10) = configData.autobootTwlTitleId;

    *(u16 *)(tlnc + 0x18) = (1 << 4) | (3 << 1) | (1 << 0);

    *(u16 *)(tlnc + 6) = crc16(tlnc + 8, 0x18, 0xFFFF);





















    if (ISN3DS)
        zerofillN3dsAblRegisters();

    CFG_BOOTENV = 3;

    return true;
}

bool configureHomebrewAutoboot(void)
{
    bool ret;
    u8 *deliverArg = loadDeliverArg();

    u32 bootenv = CFG_BOOTENV;
    u32 mode = bootenv >> 1;







    if (mode != 0)
        return false;
    else if (bootenv == 1)
    {
        for (u32 i = 0; i < 0x410; i++)
        {
            if (deliverArg[i] != 0)
                return false;
        }
        for (u32 i = 0x440; i < 0x1000; i++)
        {
            if (deliverArg[i] != 0)
                return false;
        }
    }

    switch (MULTICONFIG(AUTOBOOTMODE))
    {
        case 1:
            ret = configureHomebrewAutobootCtr(deliverArg);
            break;
        case 2:
            ret = configureHomebrewAutobootTwl(deliverArg);
            break;
        case 0:
        default:
            ret = false;
            break;
    }

    if (ret)
        commitDeliverArg();
    return ret;
}
