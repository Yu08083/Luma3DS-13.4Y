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

#include <3ds.h>
#include <arpa/inet.h>
#include "utils.h"
#include "minisoc.h"
#include "input_redirection.h"
#include "process_patches.h"
#include "menus.h"
#include "memory.h"
#include "sleep.h"
#include "sock_util.h"

bool inputRedirectionEnabled = false;
Handle inputRedirectionThreadStartedEvent;

static MyThread inputRedirectionThread;
static u8 CTR_ALIGN(8) inputRedirectionThreadStack[0x4000];

MyThread *inputRedirectionCreateThread(void)
{
    if(R_FAILED(MyThread_Create(&inputRedirectionThread, inputRedirectionThreadMain, inputRedirectionThreadStack, 0x4000, 0x20, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);
    return &inputRedirectionThread;
}


static u32 hidData[] = { 0x00000FFF, 0x02000000, 0x007FF7FF, 0x00000000, 0x00000000, 0x00000FFF, 0x02000000, 0x007FF7FF };

static u32 irData[] = { 0x80800081 };

int inputRedirectionStartResult;

void inputRedirectionThreadMain(void)
{
    Result res = 0;
    inputRedirectionStartResult = 0;

    res = miniSocInit();
    if(R_FAILED(res))
    {

        inputRedirectionStartResult = res;

        miniSocExit();

        svcSignalEvent(inputRedirectionThreadStartedEvent);
        return;
    }

    int sock = socSocket(AF_INET, SOCK_DGRAM, 0);
    u32 tries = 15;
    while(sock == -1 && --tries > 0)
    {
        svcSleepThread(100 * 1000 * 1000LL);
        sock = socSocket(AF_INET, SOCK_DGRAM, 0);
    }

    if (sock < -10000 || tries == 0) {

        inputRedirectionStartResult = -1;

        miniSocExit();

        svcSignalEvent(inputRedirectionThreadStartedEvent);
        return;
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(4950);
    saddr.sin_addr.s_addr = socGethostid();
    res = socBind(sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in));
    if(res != 0)
    {
        socClose(sock);
        miniSocExit();
        inputRedirectionStartResult = res;


        svcSignalEvent(inputRedirectionThreadStartedEvent);
        return;
    }

    inputRedirectionEnabled = true;
    svcSignalEvent(inputRedirectionThreadStartedEvent);

    u32 *hidDataPhys = PA_FROM_VA_PTR(hidData);
    hidDataPhys += 5;

    u32 *irDataPhys = PA_FROM_VA_PTR(irData);

    char buf[20];
    u32 oldSpecialButtons = 0, specialButtons = 0;
    while(inputRedirectionEnabled && !preTerminationRequested)
    {
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLIN;
        pfd.revents = 0;

        if (Sleep__Status())
        {
            while (!Wifi__IsConnected()
                    && inputRedirectionEnabled && !preTerminationRequested)
                svcSleepThread(1000000000ULL);
        }

        int pollres = socPoll(&pfd, 1, 10);
        if(pollres > 0 && (pfd.revents & POLLIN))
        {
            int n = socRecvfrom(sock, buf, 20, 0, NULL, 0);
            if(n < 0)
                break;
            else if(n < 12)
                continue;

            memcpy(hidDataPhys, buf, 12);
            if(n >= 20)
            {
                memcpy(irDataPhys, buf + 12, 4);

                oldSpecialButtons = specialButtons;
                memcpy(&specialButtons, buf + 16, 4);

                if(!(oldSpecialButtons & 1) && (specialButtons & 1))
                    srvPublishToSubscriber(0x204, 0);
                else if((oldSpecialButtons & 1) && !(specialButtons & 1))
                    srvPublishToSubscriber(0x205, 0);

                if(!(oldSpecialButtons & 2) && (specialButtons & 2))
                    srvPublishToSubscriber(0x202, 0);

                if(!(oldSpecialButtons & 4) && (specialButtons & 4))
                    srvPublishToSubscriber(0x203, 0);
            }
        }
        else if(pollres < -10000)
            break;
    }

    inputRedirectionEnabled = false;
    struct linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;

    socSetsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, sizeof(struct linger));
    socClose(sock);

    miniSocExit();
}

void hidCodePatchFunc(void);
void irCodePatchFunc(void);

static Result InputRedirection_DoUndoIrPatches(Handle processHandle, bool doPatch)
{
    static u32* hookLoc = NULL;
    static u32* syncLoc = NULL;
    static u32* cppFlagLoc = NULL;
    static u32  origIrSync = 0;
    static u32  origCppFlag = 0;

    static bool patchPrepared = false;

    static u32  irOrigReadingCode[5] = {
        0xE5940000,
        0xE1A01005,
        0xE3A03005,
        0xE3A02011,
        0x00000000
    };

    static u32  irHook[] = {
        0xE5940000,
        0xE1A01005,
        0xE59FC000,
        0xE12FFF3C,
        0x00000000
    };

    static u32  syncHookCode[] = {
        0xE5900000,
        0xEF000024,
        0xE3A00000,
        0xE51FF004,
        0x00000000,
    };


    s64     startAddress, textTotalRoundedSize, rodataTotalRoundedSize, dataTotalRoundedSize;
    u32     totalSize;
    Result  res;

    svcGetProcessInfo(&textTotalRoundedSize, processHandle, 0x10002);
    svcGetProcessInfo(&rodataTotalRoundedSize, processHandle, 0x10003);
    svcGetProcessInfo(&dataTotalRoundedSize, processHandle, 0x10004);

    totalSize = (u32)(textTotalRoundedSize + rodataTotalRoundedSize + dataTotalRoundedSize);

    svcGetProcessInfo(&startAddress, processHandle, 0x10005);
    res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, processHandle, (u32) startAddress, totalSize, 0);

    if(R_SUCCEEDED(res) && !patchPrepared)
    {
        static const u32 irOrigWaitSyncCode[] = {
            0xEF000024,
            0xE1B01FA0,
            0xE1A0A000,
        }, irOrigWaitSyncCodeOld[] = {
            0xE0AC6000,
            0xE5D70000,
        };

        static const u32 irOrigCppFlagCode[] = {
            0xE3550000,
            0xE3A0B080,
        };

        u32 irDataPhys = (u32)PA_FROM_VA_PTR(irData);
        u32 irCodePhys = (u32)PA_FROM_VA_PTR(&irCodePatchFunc);

        u32 *off = (u32 *)memsearch((u8 *)0x00100000, &irOrigReadingCode, totalSize, sizeof(irOrigReadingCode) - 4);
        if(off == NULL)
        {
            svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);
            return -1;
        }

        u32 *off2 = (u32 *)memsearch((u8 *)0x00100000, &irOrigWaitSyncCode, totalSize, sizeof(irOrigWaitSyncCode));
        if(off2 == NULL)
        {
            off2 = (u32 *)memsearch((u8 *)0x00100000, &irOrigWaitSyncCodeOld, totalSize, sizeof(irOrigWaitSyncCodeOld));
            if(off2 == NULL)
            {
                svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);
                return -2;
            }
        }

        u32 *off3 = (u32 *)memsearch((u8 *)0x00100000, &irOrigCppFlagCode, totalSize, sizeof(irOrigCppFlagCode));
        if(off3 == NULL)
        {
            svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);
            return -3;
        }

        origIrSync = *off2;
        origCppFlag = *off3;

        *(void **)(irCodePhys + 8) = decodeArmBranch(off + 4);
        *(void **)(irCodePhys + 12) = (void*)irDataPhys;

        irHook[4] = irCodePhys;
        irOrigReadingCode[4] = off[4];
        syncHookCode[4] = (u32)off2 + 4;

        hookLoc = PA_FROM_VA_PTR(off);
        syncLoc = PA_FROM_VA_PTR(off2);
        cppFlagLoc = PA_FROM_VA_PTR(off3);

        patchPrepared = true;
    }

    if (R_SUCCEEDED(res))
    {
        if (doPatch)
        {
            memcpy(hookLoc, &irHook, sizeof(irHook));


            if (*syncLoc == 0xEF000024)
            {
                syncLoc[-1] = 0xE51FF004;
                syncLoc[0] = (u32)PA_FROM_VA_PTR(&syncHookCode);
            }
            else
            {

                *syncLoc = 0xE3A00000;
            }


            *cppFlagLoc = 0xE3150000;
        }
        else
        {
            memcpy(hookLoc, irOrigReadingCode, sizeof(irOrigReadingCode));

            if (*syncLoc == 0xE3A00000)
                *syncLoc = origIrSync;
            else
            {
                syncLoc[-1] = 0xE5900000;
                syncLoc[0] = 0xEF000024;
            }

            *cppFlagLoc = origCppFlag;
        }
    }

    svcInvalidateEntireInstructionCache();
    svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);

    return res;
}

static Result InputRedirection_DoUndoHidPatches(Handle processHandle, bool doPatches)
{
    static const u32 hidOrigRegisterAndValue[] = { 0x1EC46000, 0x4001 };
    static const u32 hidOrigCode[] = {
        0xE92D4070,
        0xE1A05001,
        0xEE1D4F70,
        0xE3A01801,
        0xE5A41080,
    };

    static bool patchPrepared = false;
    static u32 *hidRegPatchOffsets[2];
    static u32 *hidPatchJumpLoc;


    s64     startAddress, textTotalRoundedSize, rodataTotalRoundedSize, dataTotalRoundedSize;
    u32     totalSize;
    Result  res;

    svcGetProcessInfo(&textTotalRoundedSize, processHandle, 0x10002);
    svcGetProcessInfo(&rodataTotalRoundedSize, processHandle, 0x10003);
    svcGetProcessInfo(&dataTotalRoundedSize, processHandle, 0x10004);

    totalSize = (u32)(textTotalRoundedSize + rodataTotalRoundedSize + dataTotalRoundedSize);

    svcGetProcessInfo(&startAddress, processHandle, 0x10005);
    res = svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, processHandle, (u32) startAddress, totalSize, 0);

    if (R_SUCCEEDED(res) && !patchPrepared)
    {
        u32 *off = (u32 *)memsearch((u8 *)0x00100000, &hidOrigRegisterAndValue, totalSize, sizeof(hidOrigRegisterAndValue));
        if(off == NULL)
        {
            svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);
            return -1;
        }

        u32 *off2 = (u32 *)memsearch((u8 *)off + sizeof(hidOrigRegisterAndValue), &hidOrigRegisterAndValue, totalSize - ((u32)off - 0x00100000), sizeof(hidOrigRegisterAndValue));
        if(off2 == NULL)
        {
            svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);
            return -2;
        }

        u32 *off3 = (u32 *)memsearch((u8 *)0x00100000, &hidOrigCode, totalSize, sizeof(hidOrigCode));
        if(off3 == NULL)
        {
            svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);
            return -3;
        }

        hidRegPatchOffsets[0] = off;
        hidRegPatchOffsets[1] = off2;
        hidPatchJumpLoc = off3;

        patchPrepared = true;
    }

    if(R_SUCCEEDED(res))
    {
        if (doPatches)
        {
            u32 hidDataPhys = (u32)PA_FROM_VA_PTR(hidData);
            u32 hidCodePhys = (u32)PA_FROM_VA_PTR(&hidCodePatchFunc);
            u32 hidHook[] = {
                0xE59F3004,
                0xE59FC004,
                0xE12FFF1C,
                hidDataPhys,
                hidCodePhys,
            };

            *hidRegPatchOffsets[0] = *hidRegPatchOffsets[1] = hidDataPhys;
            memcpy(hidPatchJumpLoc, &hidHook, sizeof(hidHook));
        }
        else
        {
            memcpy(hidRegPatchOffsets[0], &hidOrigRegisterAndValue, sizeof(hidOrigRegisterAndValue));
            memcpy(hidRegPatchOffsets[1], &hidOrigRegisterAndValue, sizeof(hidOrigRegisterAndValue));
            memcpy(hidPatchJumpLoc, &hidOrigCode, sizeof(hidOrigCode));
        }
    }

    svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x00100000, totalSize);
    return res;
}

Result InputRedirection_Disable(s64 timeout)
{
    if(!inputRedirectionEnabled)
        return 0;

    Result res = InputRedirection_DoOrUndoPatches();
    if(R_FAILED(res))
        return res;

    inputRedirectionEnabled = false;
    res = MyThread_Join(&inputRedirectionThread, timeout);
    svcCloseHandle(inputRedirectionThreadStartedEvent);

    return res;
}

Result InputRedirection_DoOrUndoPatches(void)
{
    static bool hidPatched = false;
    static bool irPatched = false;

    Handle hidProcHandle = 0, irProcHandle = 0;



    svcKernelSetState(0x10000, 4);

    Result res = OpenProcessByName("hid", &hidProcHandle);
    if (R_FAILED(res))
        goto cleanup;

    res = OpenProcessByName("ir", &irProcHandle);
    if (R_FAILED(res))
        goto cleanup;

    if(R_SUCCEEDED(res))
    {
        res = InputRedirection_DoUndoHidPatches(hidProcHandle, !hidPatched);
        if (R_SUCCEEDED(res))
            hidPatched = !hidPatched;
    }

    if(R_SUCCEEDED(res) && GET_VERSION_MINOR(osGetKernelVersion()) >= 44)
    {
        res = InputRedirection_DoUndoIrPatches(irProcHandle, !irPatched);
        if (R_SUCCEEDED(res))
            irPatched = !irPatched;
        else if (!irPatched)
        {
            InputRedirection_DoUndoHidPatches(hidProcHandle, false);
            hidPatched = false;
        }
    }

cleanup:
    svcKernelSetState(0x10000, 4);

    svcCloseHandle(hidProcHandle);
    svcCloseHandle(irProcHandle);
    return res;
}
