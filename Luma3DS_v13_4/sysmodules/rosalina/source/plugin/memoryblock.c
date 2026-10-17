#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include "plugin.h"
#include "ifile.h"
#include "utils.h"
#include "sha256.h"
#include "plugin/payload_vm.h"
#include "plugin/online_bridge.h"

#define MEMPERM_RW (MEMPERM_READ | MEMPERM_WRITE)

u32  g_loadSaveSwapArgs[4];
const char g_swapFileName[] = "/luma/plugins/.swap";
u32  g_memBlockSize = 5 * 1024 * 1024;
static PluginPayload saveProgram, loadProgram, pendingSave;
static u32 pendingArguments[4];
static bool pendingSettings, swapDigestValid;
static u8 swapDigest[32];
static LightLock onlineBridgeLock = 1;
static volatile OnlineBridgeMailbox *onlineBridge;

static void onlineBridgeClear(void)
{
    LightLock_Lock(&onlineBridgeLock);
    onlineBridge = NULL;
    LightLock_Unlock(&onlineBridgeLock);
}

bool MemoryBlock__OnlineBridgeGet(OnlineBridgeStatus *status)
{
    if(!status) return false;
    LightLock_Lock(&onlineBridgeLock);
    bool valid = OnlineBridge_Valid(onlineBridge);
    if(valid)
    {
        status->requestSeq = onlineBridge->requestSeq;
        status->requestedEnabled = onlineBridge->requestedEnabled != 0;
        status->acknowledgedSeq = onlineBridge->acknowledgedSeq;
        status->state = onlineBridge->state;
        status->error = onlineBridge->error;
    }
    LightLock_Unlock(&onlineBridgeLock);
    return valid;
}

bool MemoryBlock__OnlineBridgeSet(bool enabled)
{
    LightLock_Lock(&onlineBridgeLock);
    bool valid = OnlineBridge_Valid(onlineBridge);
    if(valid)
    {
        onlineBridge->requestedEnabled = enabled;
        __sync_synchronize();
        ++onlineBridge->requestSeq;
        svcFlushDataCacheRange((const void *)onlineBridge, sizeof(*onlineBridge));
    }
    LightLock_Unlock(&onlineBridgeLock);
    return valid;
}

static void hashSwap(const void *buffer, u8 digest[32])
{
    Sha256 context;
    Sha256_Init(&context);
    Sha256_Update(&context, buffer, g_memBlockSize);
    Sha256_Final(&context, digest);
}

Result     MemoryBlock__SetSize(u32 size) {
    PluginLoaderContext *ctx = &PluginLoaderCtx;
    MemoryBlock *memblock = &ctx->memblock;

    if (memblock->isReady)
        return MAKERESULT(RL_PERMANENT, RS_INVALIDSTATE, RM_LDR, RD_ALREADY_INITIALIZED);

    g_memBlockSize = size;
    return 0;
}

Result      MemoryBlock__IsReady(void)
{
    PluginLoaderContext *ctx = &PluginLoaderCtx;
    MemoryBlock *memblock = &ctx->memblock;

    if (memblock->isReady)
        return 0;

    Result  res;

    if (isN3DS || (ctx->pluginMemoryStrategy == PLG_STRATEGY_MODE3))
    {
        s64     appRegionSize = 0;
        s64     appRegionUsed = 0;
        u32     appRegionFree;
        u32     gameReserveSize;
        vu32*   appMemAllocPtr = (vu32 *)PA_FROM_VA_PTR(0x1FF80040);
        u32     appMemAlloc = *appMemAllocPtr;
        u32     temp;

        memblock->isAppRegion = true;

        svcGetSystemInfo(&appRegionSize, 0x10000, 0x80);
        svcGetSystemInfo(&appRegionUsed, 0, 1);

        appRegionFree = appRegionSize - appRegionUsed;


        if ((u32)appRegionSize == appMemAlloc)
            *appMemAllocPtr -= g_memBlockSize;

        gameReserveSize = appRegionFree - g_memBlockSize;


        res = svcControlMemoryUnsafe((u32 *)&temp, 0x30000000,
                                    gameReserveSize, MEMOP_REGION_APP | MEMOP_ALLOC | MEMOP_LINEAR_FLAG, MEMPERM_RW);


        if (R_SUCCEEDED(res))
            res = svcControlMemoryUnsafe((u32 *)&memblock->memblock, 0x07000000,
                                        g_memBlockSize, MEMOP_REGION_APP | MEMOP_ALLOC | MEMOP_LINEAR_FLAG, MEMPERM_RW);


        if (R_SUCCEEDED(res))
            res = svcControlMemoryUnsafe((u32 *)&temp, temp, gameReserveSize, MEMOP_FREE, 0);
    }
    else
    {
        memblock->isAppRegion = false;
        res = svcControlMemoryUnsafe((u32 *)&memblock->memblock, 0x07000000,
                                    g_memBlockSize, MEMOP_REGION_SYSTEM | MEMOP_ALLOC | MEMOP_LINEAR_FLAG, MEMPERM_RW);
    }

    if (R_FAILED(res)) {
        if (isN3DS || (ctx->pluginMemoryStrategy == PLG_STRATEGY_MODE3))
            PluginLoader__Error("Cannot map plugin memory.", res);
        else
            PluginLoader__Error("A console reboot is needed to\nclose extended memory games.\n\nPress [B] to reboot.", res);
        svcKernelSetState(7);
    }
    else
    {

        memset(memblock->memblock, 0, g_memBlockSize);
        memblock->isReady = true;


    }

    return res;
}

Result      MemoryBlock__Free(void)
{
    onlineBridgeClear();
    MemoryBlock *memblock = &PluginLoaderCtx.memblock;

    if (!memblock->isReady)
        return 0;

    MemOp   memRegion = memblock->isAppRegion ? MEMOP_REGION_APP : MEMOP_REGION_SYSTEM;
    Result  res = svcControlMemoryUnsafe((u32 *)&memblock->memblock, (u32)memblock->memblock,
                                    g_memBlockSize, memRegion | MEMOP_FREE, 0);

    memblock->isReady = false;
    memblock->memblock = NULL;

    if (R_FAILED(res))
        PluginLoader__Error("Couldn't free memblock", res);

    return res;
}

#define FS_OPEN_RWC (FS_OPEN_READ | FS_OPEN_WRITE | FS_OPEN_CREATE)

static Result swapFailure(IFile *file, bool opened, const char *message, Result result)
{
    if(opened) IFile_Close(file);
    if(!R_FAILED(result)) result = -1;
    PluginLoader__Error(message, result);
    svcKernelSetState(7);
    return result;
}

Result MemoryBlock__ToSwapFile(void)
{
    onlineBridgeClear();
    PluginLoaderContext *context = &PluginLoaderCtx;
    void *buffer = context->memblock.memblock;
    IFile file;
    swapDigestValid = false;
    uint32_t checksum;
    if(!context->isSwapFunctionset || !PluginPayload_Run(&saveProgram, buffer, g_memBlockSize, (const uint32_t *)g_loadSaveSwapArgs, &checksum))
        return swapFailure(&file, false, "Unsupported swap save payload.", -1);
    context->swapLoadChecksum = checksum;
    hashSwap(buffer, swapDigest);
    Result result = IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, g_swapFileName), FS_OPEN_RWC);
    if(R_FAILED(result)) return swapFailure(&file, false, "Failed to open swap file.", result);
    u64 written = 0;
    result = IFile_Write(&file, &written, buffer, g_memBlockSize, FS_WRITE_FLUSH);
    if(R_FAILED(result) || written != g_memBlockSize) return swapFailure(&file, true, "Could not write the whole swap file.", result);
    result = IFile_SetSize(&file, g_memBlockSize);
    if(R_FAILED(result)) return swapFailure(&file, true, "Could not resize swap file.", result);
    swapDigestValid = true;
    IFile_Close(&file);
    return 0;
}

Result MemoryBlock__FromSwapFile(void)
{
    PluginLoaderContext *context = &PluginLoaderCtx;
    void *buffer = context->memblock.memblock;
    IFile file;
    if(!swapDigestValid) return swapFailure(&file, false, "No verified swap file is available.", -1);
    Result result = IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, g_swapFileName), FS_OPEN_READ);
    if(R_FAILED(result)) return swapFailure(&file, false, "Failed to open swap file.", result);
    u64 read = 0, size = 0;
    result = IFile_GetSize(&file, &size);
    if(R_FAILED(result) || size != g_memBlockSize) return swapFailure(&file, true, "Swap file size changed.", result);
    result = IFile_Read(&file, &read, buffer, g_memBlockSize);
    if(R_FAILED(result) || read != g_memBlockSize) return swapFailure(&file, true, "Could not read the whole swap file.", result);
    u8 digest[32];
    hashSwap(buffer, digest);
    if(memcmp(digest, swapDigest, sizeof(digest))) return swapFailure(&file, true, "Swap file was changed or corrupted.", -1);
    uint32_t checksum;
    if(!PluginPayload_Run(&loadProgram, buffer, g_memBlockSize, (const uint32_t *)g_loadSaveSwapArgs, &checksum) || checksum != context->swapLoadChecksum)
        return swapFailure(&file, true, "Unsupported swap load payload or corrupted data.", -1);
    svcFlushDataCacheRange(buffer, g_memBlockSize);
    IFile_Close(&file);
    return 0;
}

Result     MemoryBlock__MountInProcess(void)
{
    onlineBridgeClear();
    Handle          target = PluginLoaderCtx.target;
    Error           *error = &PluginLoaderCtx.error;
    PluginHeader    *header = &PluginLoaderCtx.header;
    MemoryBlock     *memblock = &PluginLoaderCtx.memblock;
    bool            isPrivate = PluginLoaderCtx.isMemPrivate;

    Result       res = 0;


    if (R_FAILED((res = svcMapProcessMemoryEx(target, 0x07000000, CUR_PROCESS_HANDLE, (u32) memblock->memblock, header->exeSize, isPrivate ? MAPEXFLAGS_PRIVATE : 0))))
    {
        error->message = "Couldn't map exe memory block";
        error->code = res;
        return res;
    }


    if (R_FAILED((res = svcMapProcessMemoryEx(target, header->heapVA, CUR_PROCESS_HANDLE, (u32) memblock->memblock + header->exeSize, header->heapSize, isPrivate ? MAPEXFLAGS_PRIVATE : 0))))
    {
        error->message = "Couldn't map heap memory block";
        error->code = res;
    }

    LightLock_Lock(&onlineBridgeLock);
    onlineBridge = R_SUCCEEDED(res) && header->exeSize <= g_memBlockSize ? OnlineBridge_Find(memblock->memblock, header->exeSize) : NULL;
    LightLock_Unlock(&onlineBridgeLock);

    return res;
}

Result     MemoryBlock__UnmountFromProcess(void)
{
    onlineBridgeClear();
    Handle          target = PluginLoaderCtx.target;
    PluginHeader    *header = &PluginLoaderCtx.header;

    Result  res = 0;

    res |= svcUnmapProcessMemoryEx(target, 0x07000000, header->exeSize);
    res |= svcUnmapProcessMemoryEx(target, header->heapVA, header->heapSize);

    return res;
}

Result MemoryBlock__SetSwapSettings(u32 *code, bool isLoad, u32 *arguments)
{
    PluginPayload candidate;
    if(!code || !arguments || !PluginPayload_Set(&candidate, (const uint32_t *)code))
    {
        pendingSettings = false;
        return -1;
    }
    if(!isLoad)
    {
        pendingSave = candidate;
        memcpy(pendingArguments, arguments, sizeof(pendingArguments));
        pendingSettings = true;
        return 0;
    }
    if(!pendingSettings || memcmp(pendingArguments, arguments, sizeof(pendingArguments))) return -1;
    saveProgram = pendingSave;
    loadProgram = candidate;
    memcpy(g_loadSaveSwapArgs, arguments, sizeof(g_loadSaveSwapArgs));
    pendingSettings = false;
    PluginLoaderCtx.isSwapFunctionset = true;
    return 0;
}

void MemoryBlock__ResetSwapSettings(void)
{
    memset(&saveProgram, 0, sizeof(saveProgram));
    memset(&loadProgram, 0, sizeof(loadProgram));
    memset(&pendingSave, 0, sizeof(pendingSave));
    memset(pendingArguments, 0, sizeof(pendingArguments));
    memset(g_loadSaveSwapArgs, 0, sizeof(g_loadSaveSwapArgs));
    memset(swapDigest, 0, sizeof(swapDigest));
    swapDigestValid = pendingSettings = false;
    PluginLoaderCtx.isSwapFunctionset = false;
}

PluginHeader* MemoryBlock__GetMappedPluginHeader() {
    MemoryBlock     *memblock = &PluginLoaderCtx.memblock;
    return memblock->isReady ? (PluginHeader*)memblock->memblock : NULL;
}
