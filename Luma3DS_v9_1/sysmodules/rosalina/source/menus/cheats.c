/*
 *   This file is part of Luma3DS
 *   Copyright (C) 2016-2018 Aurora Wright, TuxSH
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
#include "menus/cheats.h"
#include "memory.h"
#include "draw.h"
#include "menu.h"
#include "utils.h"
#include "fmt.h"
#include "ifile.h"

#define MAKE_QWORD(hi,low) \
    ((u64) ((((u64)(hi)) << 32) | (low)))

typedef struct CheatProcessInfo
{
    u32 pid;
    u64 titleId;
} CheatProcessInfo;

typedef struct CheatDescription
{
    u32 active;
    u32 valid;
    u32 keyActivated;
    u32 keyCombo;
    char name[40];
    u32 codesCount;
    u64 codes[0];
} CheatDescription;

CheatDescription* cheats[1024] = { 0 };
u8 cheatFileBuffer[16384] = { 0 };
u32 cheatFilePos = 0;
u8 cheatBuffer[16384] = { 0 };

static CheatProcessInfo cheatinfo[0x40] = { 0 };

static s32 Cheats_FetchProcessInfo(void)
{
    u32 pidList[0x40];
    s32 processAmount;

    s64 sa, textTotalRoundedSize, rodataTotalRoundedSize, dataTotalRoundedSize;

    svcGetProcessList(&processAmount, pidList, 0x40);

    for (s32 i = 0; i < processAmount; i++)
    {
        Handle processHandle;
        Result res = svcOpenProcess(&processHandle, pidList[i]);
        if (R_FAILED(res)) continue;

        cheatinfo[i].pid = pidList[i];
        svcGetProcessInfo((s64 *) &cheatinfo[i].titleId, processHandle, 0x10001);
        svcGetProcessInfo(&textTotalRoundedSize, processHandle, 0x10002);
        svcGetProcessInfo(&rodataTotalRoundedSize, processHandle, 0x10003);
        svcGetProcessInfo(&dataTotalRoundedSize, processHandle, 0x10004);
        svcGetProcessInfo(&sa, processHandle, 0x10005);

        svcCloseHandle(processHandle);
    }

    return processAmount;
}

typedef struct CheatState
{
    u32 index;
    u32 offset;
    u32 data;
    u8 typeELine;
    u8 typeEIdx;

    s8 loopLine;
    u32 loopCount;

    u32 ifStack;
    u32 storedStack;
    u8 ifCount;
    u8 storedIfCount;

} CheatState;

CheatState cheat_state = { 0 };
u8 cheatCount = 0;
u8 hasKeyActivated = 0;
u64 cheatTitleInfo = -1ULL;

char failureReason[64];

static bool Cheat_IsValidAddress(const Handle processHandle, u32 address, u32 size)
{
    MemInfo info;
    PageInfo out;

    Result res = svcQueryDebugProcessMemory(&info, &out, processHandle, address);
    if (R_SUCCEEDED(res) && info.state != MEMSTATE_FREE && info.base_addr > 0 && info.base_addr <= address && address <= info.base_addr + info.size - size) {
        return true;
    }
    return false;
}

static u32 ReadWriteBuffer32 = 0;
static u16 ReadWriteBuffer16 = 0;
static u8 ReadWriteBuffer8 = 0;

static bool Cheat_Write8(const Handle processHandle, u32 offset, u8 value)
{
    if (Cheat_IsValidAddress(processHandle, cheat_state.offset + offset, 1))
    {
        *((u8*) (&ReadWriteBuffer8)) = value;
        return R_SUCCEEDED(svcWriteProcessMemory(processHandle, &ReadWriteBuffer8, cheat_state.offset + offset, 1));
    }
    return false;
}

static bool Cheat_Write16(const Handle processHandle, u32 offset, u16 value)
{
    if (Cheat_IsValidAddress(processHandle, cheat_state.offset + offset, 2))
    {
        *((u16*) (&ReadWriteBuffer16)) = value;
        return R_SUCCEEDED(svcWriteProcessMemory(processHandle, &ReadWriteBuffer16, cheat_state.offset + offset, 2));
    }
    return false;
}

static bool Cheat_Write32(const Handle processHandle, u32 offset, u32 value)
{
    if (Cheat_IsValidAddress(processHandle, cheat_state.offset + offset, 4))
    {
        *((u32*) (&ReadWriteBuffer32)) = value;
        return R_SUCCEEDED(svcWriteProcessMemory(processHandle, &ReadWriteBuffer32, cheat_state.offset + offset, 4));
    }
    return false;
}

static bool Cheat_Read8(const Handle processHandle, u32 offset, u8* retValue)
{
    if (Cheat_IsValidAddress(processHandle, cheat_state.offset + offset, 1))
    {
        Result res = svcReadProcessMemory(&ReadWriteBuffer8, processHandle, cheat_state.offset + offset, 1);
        *retValue = *((u8*) (&ReadWriteBuffer8));
        return R_SUCCEEDED(res);
    }
    return false;
}

static bool Cheat_Read16(const Handle processHandle, u32 offset, u16* retValue)
{
    if (Cheat_IsValidAddress(processHandle, cheat_state.offset + offset, 2))
    {
        Result res = svcReadProcessMemory(&ReadWriteBuffer16, processHandle, cheat_state.offset + offset, 2);
        *retValue = *((u16*) (&ReadWriteBuffer16));
        return R_SUCCEEDED(res);
    }
    return false;
}

static bool Cheat_Read32(const Handle processHandle, u32 offset, u32* retValue)
{
    if (Cheat_IsValidAddress(processHandle, cheat_state.offset + offset, 4))
    {
        Result res = svcReadProcessMemory(&ReadWriteBuffer32, processHandle, cheat_state.offset + offset, 4);
        *retValue = *((u32*) (&ReadWriteBuffer32));
        return R_SUCCEEDED(res);
    }
    return false;
}

static u8 typeEMapping[] = { 4 << 3, 5 << 3, 6 << 3, 7 << 3, 0 << 3, 1 << 3, 2 << 3, 3 << 3 };

static u8 Cheat_GetNextTypeE(const CheatDescription* cheat)
{

    if (cheat_state.typeEIdx == 7)
    {
        cheat_state.typeEIdx = 0;
        cheat_state.typeELine++;
    }
    else
    {
        cheat_state.typeEIdx++;
    }
    return (u8) ((cheat->codes[cheat_state.typeELine] >> (typeEMapping[cheat_state.typeEIdx])) & 0xFF);
}

static u32 Cheat_ApplyCheat(const Handle processHandle, const CheatDescription* const cheat)
{
    cheat_state.index = 0;
    cheat_state.offset = 0;
    cheat_state.data = 0;
    cheat_state.index = 0;
    cheat_state.loopCount = 0;
    cheat_state.loopLine = -1;
    cheat_state.ifStack = 0;
    cheat_state.storedStack = 0;
    cheat_state.ifCount = 0;
    cheat_state.storedIfCount = 0;

    while (cheat_state.index < cheat->codesCount)
    {
        u32 skipExecution = cheat_state.ifStack & 0x00000001;
        u32 arg0 = (u32) ((cheat->codes[cheat_state.index] >> 32) & 0x00000000FFFFFFFFULL);
        u32 arg1 = (u32) ((cheat->codes[cheat_state.index]) & 0x00000000FFFFFFFFULL);
        if (arg0 == 0 && arg1 == 0)
        {
            return 0;
        }
        u32 code = ((arg0 >> 28) & 0x0F);
        u32 subcode = ((arg0 >> 24) & 0x0F);

        switch (code)
        {
            case 0x0:
                
                
                
                if (!skipExecution)
                {
                    if (!Cheat_Write32(processHandle, (arg0 & 0x0FFFFFFF), arg1)) return 0;
                }
                break;
            case 0x1:
                
                
                
                if (!skipExecution)
                {
                    if (!Cheat_Write16(processHandle, (arg0 & 0x0FFFFFFF), (u16) (arg1 & 0xFFFF))) return 0;
                }
                break;
            case 0x2:
                
                
                
                if (!skipExecution)
                {
                    if (!Cheat_Write8(processHandle, (arg0 & 0x0FFFFFFF), (u8) (arg1 & 0xFF))) return 0;
                }
                break;
            case 0x3:
                
                
                
                
                
            {
                u32 newSkip;
                u32 value = 0;
                if (!Cheat_Read32(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if (value < arg1)
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;
            case 0x4:
                
                
                
                
                
            {
                u32 newSkip;
                u32 value = 0;
                if (!Cheat_Read32(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if (value > arg1)
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;
            case 0x5:
                
                
                
                
                
            {
                u32 newSkip;
                u32 value = 0;
                if (!Cheat_Read32(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if (value == arg1)
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;
            case 0x6:
                
                
                
                
                
            {
                u32 newSkip;
                u32 value = 0;
                if (!Cheat_Read32(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if (value != arg1)
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;
            case 0x7:
                
                
                
                
                
            {
                u32 newSkip;
                u16 mask = (u16) ((arg1 >> 16) & 0xFFFF);
                u16 value = 0;
                if (!Cheat_Read16(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if ((value & (~mask)) < (arg1 & 0xFFFF))
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;
            case 0x8:
                
                
                
                
                
            {
                u32 newSkip;
                u16 mask = (u16) ((arg1 >> 16) & 0xFFFF);
                u16 value = 0;
                if (!Cheat_Read16(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if ((value & (~mask)) > (arg1 & 0xFFFF))
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;
            case 0x9:
                
                
                
                
                
            {
                u32 newSkip;
                u16 mask = (u16) ((arg1 >> 16) & 0xFFFF);
                u16 value = 0;
                if (!Cheat_Read16(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if ((value & (~mask)) == (arg1 & 0xFFFF))
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;
            case 0xA:
                
                
                
                
                
            {
                u32 newSkip;
                u16 mask = (u16) ((arg1 >> 16) & 0xFFFF);
                u16 value = 0;
                if (!Cheat_Read16(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                if ((value & (~mask)) != (arg1 & 0xFFFF))
                {
                    newSkip = 0;
                }
                else
                {
                    newSkip = 1;
                }

                cheat_state.ifStack <<= 1;
                cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                cheat_state.ifCount++;
            }
                break;

            case 0xB:
                
                
                
                if (!skipExecution)
                {
                    u32 value;
                    if (!Cheat_Read32(processHandle, arg0 & 0x0FFFFFFF, &value)) return 0;
                    cheat_state.offset = value;
                }
                break;
            case 0xC:
                
                
                
                
                
                

                
                
                
                

                cheat_state.loopLine = cheat_state.index;
                cheat_state.loopCount = arg1;
                cheat_state.storedStack = cheat_state.ifStack;
                cheat_state.storedIfCount = cheat_state.ifCount;
                break;
            case 0xD:
                switch (subcode)
                {
                    case 0x00:
                        
                        
                        
                        
                        

                        
                        
                        
                        

                        
                        if (cheat_state.loopLine != -1)
                        {
                            if (cheat_state.ifCount > 0 && cheat_state.ifCount > cheat_state.storedIfCount)
                            {
                                cheat_state.ifStack >>= 1;
                                cheat_state.ifCount--;
                            }
                            else
                            {

                                if (cheat_state.loopCount > 0)
                                {
                                    cheat_state.loopCount--;
                                    if (cheat_state.loopCount == 0)
                                    {
                                        cheat_state.loopLine = -1;
                                    }
                                    else
                                    {
                                        if (cheat_state.loopLine != -1)
                                        {
                                            cheat_state.index = cheat_state.loopLine;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (cheat_state.ifCount > 0)
                            {
                                cheat_state.ifStack >>= 1;
                                cheat_state.ifCount--;
                            }
                        }
                        break;
                    case 0x01:
                        
                        
                        
                        
                        

                        
                        
                        
                        
                        
                        

                        
                        if (cheat_state.loopCount > 0)
                        {
                            cheat_state.ifStack = cheat_state.storedStack;
                            cheat_state.ifCount = cheat_state.storedIfCount;
                            cheat_state.loopCount--;
                            if (cheat_state.loopCount == 0)
                            {
                                cheat_state.loopLine = -1;
                            }
                            else
                            {
                                if (cheat_state.loopLine != -1)
                                {
                                    cheat_state.index = cheat_state.loopLine;
                                }
                            }
                        }
                        break;
                    case 0x02:
                        
                        
                        
                        
                        

                        
                        
                        
                        
                        
                        

                        
                        if (cheat_state.loopCount > 0)
                        {
                            cheat_state.loopCount--;
                            if (cheat_state.loopCount == 0)
                            {
                                cheat_state.data = 0;
                                cheat_state.offset = 0;
                                cheat_state.loopLine = -1;

                                cheat_state.ifStack = 0;
                                cheat_state.ifCount = 0;
                            }
                            else
                            {
                                if (cheat_state.loopLine != -1)
                                {
                                    cheat_state.index = cheat_state.loopLine;
                                }
                            }
                        }
                        else
                        {
                            cheat_state.data = 0;
                            cheat_state.offset = 0;
                            cheat_state.ifStack = 0;
                            cheat_state.ifCount = 0;
                        }
                        break;
                    case 0x03:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            cheat_state.offset = arg1;
                        }
                        break;
                    case 0x04:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            cheat_state.data += arg1;
                        }
                        break;
                    case 0x05:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            cheat_state.data = arg1;
                        }
                        break;
                    case 0x06:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            if (!Cheat_Write32(processHandle, arg1, cheat_state.data)) return 0;
                            cheat_state.offset += 4;
                        }
                        break;
                    case 0x07:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            if (!Cheat_Write16(processHandle, arg1, (u16) (cheat_state.data & 0xFFFF))) return 0;
                            cheat_state.offset += 2;
                        }
                        break;
                    case 0x08:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            if (!Cheat_Write8(processHandle, arg1, (u8) (cheat_state.data & 0xFF))) return 0;
                            cheat_state.offset += 1;
                        }
                        break;
                    case 0x09:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            u32 value = 0;
                            if (!Cheat_Read32(processHandle, arg1, &value)) return 0;
                            cheat_state.data = value;
                        }
                        break;
                    case 0x0A:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            u16 value = 0;
                            if (!Cheat_Read16(processHandle, arg1, &value)) return 0;
                            cheat_state.data = value;
                        }
                        break;
                    case 0x0B:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            u8 value = 0;
                            if (!Cheat_Read8(processHandle, arg1, &value)) return 0;
                            cheat_state.data = value;
                        }
                        break;
                    case 0x0C:
                        
                        
                        
                        
                        
                        
                        if (!skipExecution)
                        {
                            cheat_state.offset += arg1;
                        }
                        break;
                    case 0x0D:
                        
                    {
                        u32 newSkip;
                        if (arg1 == 0 || (HID_PAD & arg1) == arg1)
                        {
                            newSkip = 0;
                        }
                        else
                        {
                            newSkip = 1;
                        }

                        cheat_state.ifStack <<= 1;
                        cheat_state.ifStack |= ((newSkip | skipExecution) & 0x1);
                        cheat_state.ifCount++;
                    }
                        break;
                    default:
                        return 0;
                }
                break;
            case 0xE:
                
                
                
                

                

            {
                u32 beginOffset = (arg0 & 0x0FFFFFFF);
                u32 count = arg1;
                cheat_state.typeELine = cheat_state.index;
                cheat_state.typeEIdx = 7;
                for (u32 i = 0; i < count; i++)
                {
                    u8 byte = Cheat_GetNextTypeE(cheat);
                    if (!skipExecution)
                    {
                        if (!Cheat_Write8(processHandle, beginOffset + i, byte)) return 0;
                    }
                }
                cheat_state.index = cheat_state.typeELine;
            }
                break;
            default:
                return 0;
        }
        cheat_state.index++;
    }
    return 1;
}

static void Cheat_EatEvents(Handle debug)
{
    DebugEventInfo info;
    Result r;

    while(true)
    {
        if((r = svcGetProcessDebugEvent(&info, debug)) != 0)
        {
            if(r == (s32)(0xd8402009))
            {
                break;
            }
        }
        svcContinueDebugEvent(debug, 3);
    }
}

static Result Cheat_MapMemoryAndApplyCheat(u32 pid, CheatDescription* const cheat)
{
    Handle processHandle;
    Handle debugHandle;
    Result res;
    res = svcOpenProcess(&processHandle, pid);
    if (R_SUCCEEDED(res))
    {
        res = svcDebugActiveProcess(&debugHandle, pid);
        if (R_SUCCEEDED(res))
        {
            Cheat_EatEvents(debugHandle);
            cheat->valid = Cheat_ApplyCheat(debugHandle, cheat);

            svcCloseHandle(debugHandle);
            svcCloseHandle(processHandle);
            cheat->active = 1;
        }
        else
        {
            svcCloseHandle(processHandle);
        }
    }
    else
    {
        sprintf(failureReason, "Open process failed");
    }
    return res;
}

static CheatDescription* Cheat_AllocCheat()
{
    CheatDescription* cheat;
    if (cheatCount == 0)
    {
        cheat = (CheatDescription*) cheatBuffer;
    }
    else
    {
        CheatDescription* prev = cheats[cheatCount - 1];
        cheat = (CheatDescription *) ((u8*) (prev) + sizeof(CheatDescription) + sizeof(u64) * (prev->codesCount));
    }
    cheat->active = 0;
    cheat->valid = 1;
    cheat->codesCount = 0;
    cheat->keyActivated = 0;
    cheat->keyCombo = 0;
    cheat->name[0] = '\0';

    cheats[cheatCount] = cheat;
    cheatCount++;
    return cheat;
}

static void Cheat_AddCode(CheatDescription* cheat, u64 code)
{
    if (cheat)
    {
        cheat->codes[cheat->codesCount] = code;
        (cheat->codesCount)++;
    }
}

static Result Cheat_ReadLine(char* line)
{
    Result res = 0;

    char c = '\0';
    u32 idx = 0;
    while (R_SUCCEEDED(res))
    {
        c = cheatFileBuffer[cheatFilePos++];
        res = c ? 0 : -1;
        if (R_SUCCEEDED(res) && c != '\0')
        {
            if (c == '\n' || c == '\r' || idx >= 1023)
            {
                line[idx++] = '\0';
                return idx;
            }
            else
            {
                line[idx++] = c;
            }
        }
        else
        {
            if (idx > 0)
            {
                line[idx++] = '\0';
                return idx;
            }
        }
    }
    return res;
}

static bool Cheat_IsCodeLine(const char *line)
{
    s32 len = strnlen(line, 1023);
    if (len != 17)
    {
        return false;
    }
    if (line[8] != ' ')
    {
        return false;
    }

    int i;
    for (i = 0; i < 8; i++)
    {
        char c = line[i];
        if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f')))
        {
            return false;
        }
    }

    for (i = 9; i < 17; i++)
    {
        char c = line[i];
        if (!(('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f')))
        {
            return false;
        }
    }

    return true;
}

static u64 Cheat_GetCode(const char *line)
{
    u64 tmp = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        char c = line[i];
        u8 code = 0;
        if ('0' <= c && c <= '9')
        {
            code = c - '0';
        }
        if ('A' <= c && c <= 'F')
        {
            code = c - 'A' + 10;
        }
        if ('a' <= c && c <= 'f')
        {
            code = c - 'a' + 10;
        }
        tmp <<= 4;
        tmp |= (code & 0xF);
    }

    for (i = 9; i < 17; i++)
    {
        char c = line[i];
        u8 code = 0;
        if ('0' <= c && c <= '9')
        {
            code = c - '0';
        }
        if ('A' <= c && c <= 'F')
        {
            code = c - 'A' + 10;
        }
        if ('a' <= c && c <= 'f')
        {
            code = c - 'a' + 10;
        }
        tmp <<= 4;
        tmp |= (code & 0xF);
    }

    return tmp;
}

static void Cheat_LoadCheatsIntoMemory(u64 titleId)
{
    cheatCount = 0;
    cheatTitleInfo = titleId;
    hasKeyActivated = 0;

    char path[64] = { 0 };
    sprintf(path, "/luma/titles/%016llX/cheats.txt", titleId);

    IFile file;

    if (R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, path), FS_OPEN_READ)))
    {
        
        sprintf(path, "/cheats/%016llX.txt", titleId);
        if (R_FAILED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, path), FS_OPEN_READ))) return;
    };

    u64 fileLen = 0;
    IFile_GetSize(&file, &fileLen);
    if (fileLen > 16383)
    {
        fileLen = 16383;
    }

    u64 total;
    IFile_Read(&file, &total, cheatFileBuffer, fileLen);
    IFile_Close(&file);
    for (int i = fileLen; i < 16384; i++)
    {
        cheatFileBuffer[i] = 0;
    }

    char line[1024] = { 0 };
    Result res = 0;
    CheatDescription* cheat = 0;
    cheatFilePos = 0;
    do
    {
        res = Cheat_ReadLine(line);
        if (R_SUCCEEDED(res))
        {
            s32 lineLen = strnlen(line, 1023);
            if (!lineLen)
            {
                continue;
            }
            if (line[0] == '#')
            {
                continue;
            }
            if (Cheat_IsCodeLine(line))
            {
                if (cheat)
                {
                    u64 tmp = Cheat_GetCode(line);
                    Cheat_AddCode(cheat, tmp);
                    if (((tmp >> 32) & 0xFFFFFFFF) == 0xDD000000)
                    {
                        if (tmp & 0xFFFFFFFF)
                        {
                            
                            cheat->keyCombo |= (tmp & 0xFFF);
                            cheat->keyActivated = 1;
                        }
                    }
                }
            }
            else
            {
                if (!cheat)
                {
                    cheat = Cheat_AllocCheat();
                }
                else
                {
                    if (cheat->codesCount > 0)
                    {
                        
                        cheat = Cheat_AllocCheat();
                    }
                }
                strncpy(cheat->name, line, 39);
            }
        }
    } while (R_SUCCEEDED(res));

    if ((cheatCount > 0) && (cheats[cheatCount - 1]->codesCount == 0))
    {
        cheatCount--; 
    }
}

static u32 Cheat_GetCurrentPID(u64* titleId)
{
    s32 processAmount = Cheats_FetchProcessInfo();

    s32 index = -1;

    for (s32 i = 0; i < processAmount; i++)
    {

        if (((u32) (cheatinfo[i].titleId >> 32)) == 0x00040010 || ((u32) (cheatinfo[i].titleId >> 32) == 0x00040000))
        {
            index = i;

            break;
        }
    }

    if (index != -1)
    {
        *titleId = cheatinfo[index].titleId;
        return cheatinfo[index].pid;
    }
    else
    {
        *titleId = 0;
        return 0xFFFFFFFF;
    }
}

void Cheat_ApplyCheats(void)
{
    if (!cheatCount)
    {
        return;
    }

    u64 titleId = 0;
    u32 pid = Cheat_GetCurrentPID(&titleId);

    if (!titleId)
    {
        cheatCount = 0;
        hasKeyActivated = 0;
        return;
    }

    if (titleId != cheatTitleInfo)
    {
        cheatCount = 0;
        hasKeyActivated = 0;
        return;
    }

    u32 keys = HID_PAD & 0xFFF;
    for (int i = 0; i < cheatCount; i++)
    {
        if (cheats[i]->active && !(cheats[i]->keyActivated))
        {
            Cheat_MapMemoryAndApplyCheat(pid, cheats[i]);
        } 
        else if (cheats[i]->active && cheats[i]->keyActivated && (cheats[i]->keyCombo & keys) == keys)
        {
            Cheat_MapMemoryAndApplyCheat(pid, cheats[i]);    
        }
    }
}

void RosalinaMenu_Cheats(void)
{
    u64 titleId = 0;
    u32 pid = Cheat_GetCurrentPID(&titleId);

    if (titleId != 0)
    {
        if (cheatTitleInfo != titleId || cheatCount == 0)
        {
            Cheat_LoadCheatsIntoMemory(titleId);
        }
    }

    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_FlushFramebuffer();
    Draw_Unlock();

    if (titleId == 0 || cheatCount == 0)
    {
        do
        {
            Draw_Lock();
            Draw_DrawString(10, 10, COLOR_TITLE, "Cheats");
            if (titleId == 0)
            {
                Draw_DrawString(10, 30, COLOR_WHITE, "No suitable title found");
            }
            else
            {
                Draw_DrawFormattedString(10, 30, COLOR_WHITE, "No cheats found for title %016llX", titleId);
            }

            Draw_FlushFramebuffer();
            Draw_Unlock();
        } while (!(waitInput() & BUTTON_B) && !terminationRequest);
    }
    else
    {
        s32 selected = 0, page = 0, pagePrev = 0;

        Result r = 0;
        do
        {
            Draw_Lock();
            if (page != pagePrev || R_FAILED(r))
            {
                Draw_ClearFramebuffer();
            }
            if (R_SUCCEEDED(r))
            {
                Draw_DrawFormattedString(10, 10, COLOR_TITLE, "Cheat list");

                for (s32 i = 0; i < CHEATS_PER_MENU_PAGE && page * CHEATS_PER_MENU_PAGE + i < cheatCount; i++)
                {
                    char buf[65] = { 0 };
                    s32 j = page * CHEATS_PER_MENU_PAGE + i;
                    const char * checkbox = (cheats[j]->active ? "(x) " : "( ) ");
                    const char * keyAct = (cheats[j]->keyActivated ? "*" : " ");
                    sprintf(buf, "%s%s%s", checkbox, keyAct, cheats[j]->name);

                    Draw_DrawString(30, 30 + i * SPACING_Y, cheats[j]->valid ? COLOR_WHITE : COLOR_RED, buf);
                    Draw_DrawCharacter(10, 30 + i * SPACING_Y, COLOR_TITLE, j == selected ? '>' : ' ');
                }
            }
            else
            {
                Draw_DrawFormattedString(10, 10, COLOR_TITLE, "ERROR: %08x", r);
                Draw_DrawFormattedString(10, 30, COLOR_RED, failureReason);
            }
            Draw_FlushFramebuffer();
            Draw_Unlock();

            if (terminationRequest) break;

            u32 pressed;
            do
            {
                pressed = waitInputWithTimeout(50);
                if (pressed != 0) break;
            } while (pressed == 0 && !terminationRequest);

            if (pressed & BUTTON_B)
                break;
            else if ((pressed & BUTTON_A) && R_SUCCEEDED(r))
            {
                if (cheats[selected]->active)
                {
                    cheats[selected]->active = 0;
                }
                else
                {
                    r = Cheat_MapMemoryAndApplyCheat(pid, cheats[selected]);
                }
                hasKeyActivated = 0;
                for (int i = 0; i < cheatCount; i++)
                {
                    if (cheats[i]->active && cheats[i]->keyActivated)
                    {
                        hasKeyActivated = 1;
                        break;
                    }
                }
            }
            else if (pressed & BUTTON_DOWN)
                selected++;
            else if (pressed & BUTTON_UP)
                selected--;
            else if (pressed & BUTTON_LEFT)
                selected -= CHEATS_PER_MENU_PAGE;
            else if (pressed & BUTTON_RIGHT)
            {
                if (selected + CHEATS_PER_MENU_PAGE < cheatCount)
                    selected += CHEATS_PER_MENU_PAGE;
                else if ((cheatCount - 1) / CHEATS_PER_MENU_PAGE == page)
                    selected %= CHEATS_PER_MENU_PAGE;
                else selected = cheatCount - 1;
            }

            if (selected < 0)
                selected = cheatCount - 1;
            else if (selected >= cheatCount) selected = 0;

            pagePrev = page;
            page = selected / CHEATS_PER_MENU_PAGE;
        } while (!terminationRequest);
    }

}
