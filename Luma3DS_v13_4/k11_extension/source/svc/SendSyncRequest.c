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
#include <string.h>

#include "svc/SendSyncRequest.h"
#include "ipc.h"
#include "mmu.h"
#include "plguds_ipc.h"


typedef struct {
    KProcess *process;
    KSession *session;
    s64 creationTime;
    bool active;
    bool pending;
    bool paused;
    bool signalState;
} PlgUdsContext;

static PlgUdsContext plgUdsContext;
static KRecursiveLock plgUdsLock;

static void PlgUdsRelease(KSession *session, KProcess *process)
{
    if (session) session->autoObject.vtable->DecrementReferenceCount(&session->autoObject);
    if (process)
    {
        KAutoObject *object = (KAutoObject *)process;
        object->vtable->DecrementReferenceCount(object);
    }
}

static bool PlgUdsOwns(const PlgUdsContext *context, KProcess *process)
{
    return context->process == process && context->creationTime == KPROCESS_GET_RVALUE(process, creationTime);
}

static void PlgUdsServiceName(KSession *session, char name[13])
{
    memset(name, 0, 13);
    KRecursiveLock__Lock(criticalSectionLock);
    SessionInfo *info = SessionInfo_Lookup(session);
    if (info) memcpy(name, info->name, 12);
    KRecursiveLock__Unlock(criticalSectionLock);
}

static bool PlgUdsOwnsServer(KProcess *process, KSession *session)
{
    KProcessHandleTable *table = handleTableOfProcess(process);
    bool owns = false;
    KClientPort *clientPort = session->clientSession.clientPort;
    KAutoObject *serverPort = clientPort && clientPort->parentKPort ? &clientPort->parentKPort->serverPort.syncObject.autoObject : NULL;
    KObjectMutex__Acquire(&table->mutex);
    HandleDescriptor *entries = table->handleTable ? table->handleTable : table->internalTable;
    for (s32 index = 0; index < table->maxHandleCount; index++)
    {
        if (entries[index].pointer == &session->serverSession.syncObject.autoObject || (serverPort && entries[index].pointer == serverPort))
        {
            owns = true;
            break;
        }
    }
    KObjectMutex__Release(&table->mutex);
    return owns;
}

void PlgUdsResetProcess(KProcess *process)
{
    KSession *session = NULL;
    KProcess *owner = NULL;
    KRecursiveLock__Lock(&plgUdsLock);
    if (plgUdsContext.process == process)
    {
        session = plgUdsContext.session;
        owner = plgUdsContext.process;
        memset(&plgUdsContext, 0, sizeof(plgUdsContext));
    }
    KRecursiveLock__Unlock(&plgUdsLock);
    PlgUdsRelease(session, owner);
}

static bool PlgUdsFinalizeSession(KProcess *process, KSession *controlSession)
{
    KSession *session = NULL;
    KProcess *owner = NULL;
    KRecursiveLock__Lock(&plgUdsLock);
    if (PlgUdsOwns(&plgUdsContext, process) && plgUdsContext.active && plgUdsContext.session == controlSession)
    {
        session = plgUdsContext.session;
        owner = plgUdsContext.process;
        memset(&plgUdsContext, 0, sizeof(plgUdsContext));
    }
    KRecursiveLock__Unlock(&plgUdsLock);
    PlgUdsRelease(session, owner);
    return owner != NULL;
}

static void PlgUdsPrune(void)
{
    KSession *session = NULL;
    KProcess *owner = NULL;
    KRecursiveLock__Lock(&plgUdsLock);
    if (plgUdsContext.process && KPROCESS_GET_RVALUE(plgUdsContext.process, status) != PROCESS_AVAILABLE)
    {
        session = plgUdsContext.session;
        owner = plgUdsContext.process;
        memset(&plgUdsContext, 0, sizeof(plgUdsContext));
    }
    KRecursiveLock__Unlock(&plgUdsLock);
    PlgUdsRelease(session, owner);
}

static PlgUdsContext PlgUdsSnapshot(KProcess *process)
{
    PlgUdsContext context = {0};
    KRecursiveLock__Lock(&plgUdsLock);
    if (PlgUdsOwns(&plgUdsContext, process) && plgUdsContext.active)
    {
        context = plgUdsContext;
        context.session = NULL;
    }
    KRecursiveLock__Unlock(&plgUdsLock);
    return context;
}

static bool PlgUdsKernelRange(void *context, uint32_t address, uint32_t size, bool write)
{
    KProcessHwInfo *hardware = hwInfoOfProcess((KProcess *)context);
    u32 required = write ? 3 : 1;
    u32 last = (address + size - 1) & ~0xFFFu;
    for (u32 page = address & ~0xFFFu;; page += 0x1000)
    {
        if ((KProcessHwInfo__GetAddressUserPerm(hardware, page) & required) != required) return false;
        if (page == last) return true;
    }
}

static bool PlgUdsKernelHandle(void *context, uint32_t handle, bool sharedMemory)
{
    if (!handle || handle >= 0xFFFF8000u) return false;
    KAutoObject *object = KProcessHandleTable__ToKAutoObject(handleTableOfProcess((KProcess *)context), handle);
    if (!object) return false;
    bool valid = strcmp(classNameOfAutoObject(object), sharedMemory ? "KSharedMemory" : "KEvent") == 0;
    object->vtable->DecrementReferenceCount(object);
    return valid;
}

static void PlgUdsReply(u32 *command, Result result)
{
    command[0] = PlgUdsHeader(command[0] >> 16, 1, 0);
    command[1] = (u32)result;
    command[2] = command[3] = 0;
}

static Result PlgUdsSignal(KProcess *process, u32 *command, u32 *staticBuffers, u32 event)
{
    KSession *session = NULL;
    KRecursiveLock__Lock(&plgUdsLock);
    if (PlgUdsOwns(&plgUdsContext, process) && plgUdsContext.active && plgUdsContext.session)
    {
        session = plgUdsContext.session;
        KAutoObject__AddReference(&session->autoObject);
    }
    KRecursiveLock__Unlock(&plgUdsLock);
    if (!session) return (Result)0xD8E007F7;
    Handle temporary = 0;
    Result result = createHandleForThisProcess(&temporary, &session->clientSession.syncObject.autoObject);
    session->autoObject.vtable->DecrementReferenceCount(&session->autoObject);
    if (result < 0) return result;
    u32 backup[96];
    memcpy(backup, command, 64 * sizeof(u32));
    memcpy(backup + 64, staticBuffers, 32 * sizeof(u32));
    command[0] = PlgUdsHeader(102, 1, 0);
    command[1] = event;
    result = SendSyncRequest(temporary);
    if (result >= 0)
        result = command[0] == PlgUdsHeader(102, 1, 0) ? (Result)command[1] : (Result)0xD9001830;
    memcpy(command, backup, 64 * sizeof(u32));
    memcpy(staticBuffers, backup + 64, 32 * sizeof(u32));
    CloseHandle(temporary);
    return result;
}

static bool PlgUdsBegin(KProcess *process, KSession *session)
{
    PlgUdsPrune();
    if (!PlgUdsOwnsServer(process, session)) return false;
    bool ready = false;
    KRecursiveLock__Lock(&plgUdsLock);
    if (!plgUdsContext.process)
    {
        plgUdsContext.process = process;
        plgUdsContext.creationTime = KPROCESS_GET_RVALUE(process, creationTime);
        plgUdsContext.session = session;
        plgUdsContext.pending = true;
        KAutoObject__AddReference((KAutoObject *)process);
        KAutoObject__AddReference(&session->autoObject);
        ready = true;
    }
    KRecursiveLock__Unlock(&plgUdsLock);
    return ready;
}

static void PlgUdsFinish(KProcess *process, bool success)
{
    KSession *session = NULL;
    KProcess *owner = NULL;
    KRecursiveLock__Lock(&plgUdsLock);
    if (PlgUdsOwns(&plgUdsContext, process) && plgUdsContext.pending)
    {
        if (success)
        {
            plgUdsContext.pending = false;
            plgUdsContext.active = true;
        }
        else
        {
            session = plgUdsContext.session;
            owner = plgUdsContext.process;
            memset(&plgUdsContext, 0, sizeof(plgUdsContext));
        }
    }
    KRecursiveLock__Unlock(&plgUdsLock);
    PlgUdsRelease(session, owner);
}

static inline bool isNdmuWorkaround(const SessionInfo *info, u32 pid)
{
    return info != NULL && strcmp(info->name, "ndm:u") == 0 && hasStartedRosalinaNetworkFuncsOnce && pid >= nbSection0Modules;
}

Result SendSyncRequestHook(Handle handle)
{
    KProcess *currentProcess = currentCoreContext->objectContext.currentProcess;
    KProcessHandleTable *handleTable = handleTableOfProcess(currentProcess);
    u32 pid = idOfProcess(currentProcess);
    KClientSession *clientSession = (KClientSession *)KProcessHandleTable__ToKAutoObject(handleTable, handle);

    u32 *cmdbuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x80);
    u32 *staticBuffers = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x180);
    bool skip = false;
    bool initializePlgUds = false;
    bool detachPlgUds = false;
    Result res = 0;


    bool isValidClientSession = clientSession != NULL && strcmp(classNameOfAutoObject(&clientSession->syncObject.autoObject), "KClientSession") == 0;

    if(isValidClientSession)
    {
        char serviceName[13];
        PlgUdsServiceName(clientSession->parentSession, serviceName);
        SessionInfo infoSnapshot = {clientSession->parentSession, {0}};
        memcpy(infoSnapshot.name, serviceName, 12);
        infoSnapshot.name[11] = 0;
        SessionInfo *info = serviceName[0] ? &infoSnapshot : NULL;
        PlgUdsContext uds = PlgUdsSnapshot(currentProcess);
        if (strcmp(serviceName, "plg:UDS") == 0)
        {
            PlgUdsValidation validation = {currentProcess, PlgUdsKernelRange, PlgUdsKernelHandle, pid};
            PlgUdsAction action = PlgUdsPrepare(cmdbuf, staticBuffers, &validation);
            bool ownsServer = PlgUdsOwnsServer(currentProcess, clientSession->parentSession);
            if (action == PlgUdsReject || pid < nbSection0Modules)
            {
                PlgUdsReply(cmdbuf, (Result)0xD9001830);
                skip = true;
            }
            else if (action == PlgUdsFinalize)
            {
                if (!PlgUdsFinalizeSession(currentProcess, clientSession->parentSession))
                {
                    PlgUdsReply(cmdbuf, (Result)0xD9001830);
                    skip = true;
                }
                else if (!ownsServer)
                {
                    PlgUdsReply(cmdbuf, 0);
                    skip = true;
                }
            }
            else if (!ownsServer || (action != PlgUdsInitialize && !uds.active))
            {
                PlgUdsReply(cmdbuf, (Result)0xD9001830);
                skip = true;
            }
            else if (action == PlgUdsInitialize)
            {
                initializePlgUds = PlgUdsBegin(currentProcess, clientSession->parentSession);
                if (!initializePlgUds)
                {
                    PlgUdsReply(cmdbuf, (Result)0xD8E007F7);
                    skip = true;
                }
            }
            else if (action == PlgUdsSignalState || action == PlgUdsPause)
            {
                KRecursiveLock__Lock(&plgUdsLock);
                if (PlgUdsOwns(&plgUdsContext, currentProcess) && plgUdsContext.active)
                {
                    if (action == PlgUdsSignalState) plgUdsContext.signalState = cmdbuf[1] != 0;
                    else plgUdsContext.paused = cmdbuf[1] != 0;
                    PlgUdsReply(cmdbuf, 0);
                }
                else PlgUdsReply(cmdbuf, (Result)0xD8E007F7);
                KRecursiveLock__Unlock(&plgUdsLock);
                skip = true;
            }
        }
        else if (uds.active)
        {
            if (!uds.paused && strcmp(serviceName, "ndm:u") == 0 && cmdbuf[0] == 0x10042 && cmdbuf[1] == 2)
                cmdbuf[1] = 1;
            if (uds.signalState && strcmp(serviceName, "soc:U") == 0)
            {
                if (cmdbuf[0] == 0x10044)
                {
                    res = PlgUdsSignal(currentProcess, cmdbuf, staticBuffers, 0);
                    if (res < 0)
                    {
                        PlgUdsResetProcess(currentProcess);
                        skip = true;
                    }
                }
                else if (cmdbuf[0] == 0x190000) detachPlgUds = true;
            }
        }
        if (!skip) switch (cmdbuf[0])
        {
            case 0x10042:
            {

                if(isNdmuWorkaround(info, pid))
                {
                    cmdbuf[0] = 0x10040;
                    cmdbuf[1] = 0;
                    skip = true;
                }

                break;
            }

            case 0x10082:
            {

                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x10800:
            {

                if(info != NULL && strcmp(info->name, "err:f") == 0)
                    skip = doErrfThrowHook(cmdbuf);

                break;
            }

            case 0x20000:
            {

                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x20002:
            {

                if(isNdmuWorkaround(info, pid))
                {
                    cmdbuf[0] = 0x20040;
                    cmdbuf[1] = 0;
                    skip = true;
                }

                break;
            }

            case 0x50100:
            {

                if(info != NULL && (strcmp(info->name, "srv:") == 0 || (GET_VERSION_MINOR(kernelVersion) < 39 && strcmp(info->name, "srv:pm") == 0)))
                {
                    char name[9] = { 0 };
                    memcpy(name, cmdbuf + 1, 8);
                    PlgUdsContext currentUds = PlgUdsSnapshot(currentProcess);
                    if (currentUds.active && !currentUds.paused && cmdbuf[3] == 8 && cmdbuf[4] <= 1 && strcmp(name, "nwm::UDS") == 0)
                    {
                        memcpy(name, "plg:UDS", 8);
                        memcpy(cmdbuf + 1, name, 8);
                        cmdbuf[3] = 7;
                    }

                    skip = true;
                    res = SendSyncRequest(handle);
                    if(res == 0 && cmdbuf[0] == PlgUdsHeader(5, 1, 2) && (s32)cmdbuf[1] >= 0 && (cmdbuf[2] & ~0x10u) == 0)
                    {
                        KClientSession *outClientSession;

                        outClientSession = (KClientSession *)KProcessHandleTable__ToKAutoObject(handleTable, (Handle)cmdbuf[3]);
                        if(outClientSession != NULL)
                        {
                            if(strcmp(classNameOfAutoObject(&outClientSession->syncObject.autoObject), "KClientSession") == 0)
                                SessionInfo_Add(outClientSession->parentSession, name);
                            outClientSession->syncObject.autoObject.vtable->DecrementReferenceCount(&outClientSession->syncObject.autoObject);
                        }
                    }
                    else if (res < 0)
                    {


                        cmdbuf[3] = 0;
                    }
                }

                break;
            }

            case 0x80040:
            {
                if(!hasStartedRosalinaNetworkFuncsOnce)
                    break;

                skip = isNdmuWorkaround(info, pid);
                if(skip)
                    cmdbuf[1] = 0;
                break;
            }

            case 0x90000:
            {
                if(!hasStartedRosalinaNetworkFuncsOnce)
                    break;

                if(isNdmuWorkaround(info, pid))
                {
                    cmdbuf[0] = 0x90040;
                    cmdbuf[1] = 0;
                    skip = true;
                }
                break;
            }

            case 0x00C0080:
            {


                if (info != NULL && strcmp(info->name, "srv:") == 0 && cmdbuf[1] == 0x1002)
                {

                    PLG__WakeAppThread();
                    cmdbuf[0] = 0xC0040;
                    cmdbuf[1] = 0;
                    skip = true;
                }
                break;
            }

            case 0x00D0080:
            {


                if (info != NULL && strncmp(info->name, "APT:", 4) == 0 && cmdbuf[1] == 0x300)
                {
                    res = SendSyncRequest(handle);
                    skip = true;

                    if (res >= 0)
                    {
                        u32 plgStatus = PLG_GetStatus();
                        u32 command = cmdbuf[3];

                        if ((plgStatus == PLG_CFG_RUNNING && command == 3)
                        || (plgStatus == PLG_CFG_INHOME && (command >= 10 && command <= 12)))
                            PLG_SignalEvent(PLG_CFG_HOME_EVENT);
                    }
                }
                break;
            }

            case 0x4010082:
            {

                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x4020082:
            {

                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x8010082:
            {

                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x8020082:
            {

                if(info != NULL && strcmp(info->name, "cfg:i") == 0)
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x4060000:
            {

                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x8160000:
            {

                if(info != NULL && strcmp(info->name, "cfg:i") == 0)
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }
        }
    }

    if(clientSession != NULL)
        clientSession->syncObject.autoObject.vtable->DecrementReferenceCount(&clientSession->syncObject.autoObject);

    res = skip ? res : SendSyncRequest(handle);
    if (initializePlgUds)
        PlgUdsFinish(currentProcess, res >= 0 && cmdbuf[0] == PlgUdsHeader(100, 1, 0) && (s32)cmdbuf[1] >= 0);
    if (detachPlgUds && res >= 0)
    {
        Result signalResult = PlgUdsSignal(currentProcess, cmdbuf, staticBuffers, 1);
        if (signalResult < 0) PlgUdsResetProcess(currentProcess);
    }

    return res;
}
