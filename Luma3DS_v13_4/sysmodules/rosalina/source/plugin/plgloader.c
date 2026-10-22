#include <3ds.h>
#include "ifile.h"
#include "utils.h"
#include "luma_config.h"
#include "plugin.h"
#include "fmt.h"
#include "menu.h"
#include "menus.h"
#include "memory.h"
#include "sleep.h"
#include "task_runner.h"

#define PLGLDR_VERSION (SYSTEM_VERSION(1, 0, 2))

#define THREADVARS_MAGIC  0x21545624

#define PERS_USER_FILE_MAGIC 0x53524550

static const char *g_title = "Plugin loader";
PluginLoaderContext PluginLoaderCtx;
extern u32 g_blockMenuOpen;

void        PluginLoader__Init(void)
{
    PluginLoaderContext *ctx = &PluginLoaderCtx;

    memset(ctx, 0, sizeof(PluginLoaderContext));

    s64 pluginLoaderFlags = 0;

    svcGetSystemInfo(&pluginLoaderFlags, 0x10000, 0x180);
    ctx->isEnabled = pluginLoaderFlags & 1;

    ctx->plgEventPA = (s32 *)PA_FROM_VA_PTR(&ctx->plgEvent);
    ctx->plgReplyPA = (s32 *)PA_FROM_VA_PTR(&ctx->plgReply);

    ctx->pluginMemoryStrategy = PLG_STRATEGY_SWAP;

    MemoryBlock__ResetSwapSettings();

    assertSuccess(svcCreateAddressArbiter(&ctx->arbiter));
    assertSuccess(svcCreateEvent(&ctx->kernelEvent, RESET_ONESHOT));

    svcKernelSetState(0x10007, ctx->kernelEvent, 0, 0);

    IFile file;
    if (R_SUCCEEDED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/plugins/user_param.bin"), FS_OPEN_READ)))
    {
        PluginLoadParameters *params = &ctx->userLoadParameters;
        u64 temp_read;
        u32 magic = 0;
        if (R_SUCCEEDED(IFile_Read(&file, &temp_read, &magic, sizeof(magic))) && temp_read == sizeof(magic) && magic == PERS_USER_FILE_MAGIC
            && R_SUCCEEDED(IFile_Read(&file, &temp_read, params, sizeof(PluginLoadParameters))) && temp_read == sizeof(PluginLoadParameters))
        {
            ctx->useUserLoadParameters = true;
        }
        else
        {
            memset(params, 0, sizeof(PluginLoadParameters));
        }
        IFile_Close(&file);
    }
}

void    PluginLoader__Error(const char *message, Result res)
{
    DispErrMessage(g_title, message, res);
}

bool        PluginLoader__IsEnabled(void)
{
    return PluginLoaderCtx.isEnabled;
}

void        PluginLoader__MenuCallback(void)
{
    PluginLoaderCtx.isEnabled = !PluginLoaderCtx.isEnabled;
    LumaConfig_RequestSaveSettings();
    PluginLoader__UpdateMenu();
}

void        PluginLoader__UpdateMenu(void)
{
    static const char *status[2] =
    {
        "Plugin Loader: [Disabled]",
        "Plugin Loader: [Enabled]"
    };

    for(u32 i=0;i<sizeof(rosalinaMenu.items)/sizeof(rosalinaMenu.items[0]);++i)
    {
        MenuItem *item=&rosalinaMenu.items[i];
        if(item->action_type==MENU_END)break;
        if(item->action_type==METHOD&&item->method==PluginLoader__MenuCallback)
        {
            item->title=status[PluginLoaderCtx.isEnabled];
            break;
        }
    }
}

static ControlApplicationMemoryModeOverrideConfig g_memorymodeoverridebackup = { 0 };
Result  PluginLoader__SetMode3AppMode(bool enable)
{
    Handle loaderHandle;
    Result res = srvGetServiceHandle(&loaderHandle, "Loader");

    if (R_FAILED(res)) return res;

    u32 *cmdbuf = getThreadCommandBuffer();

    if (enable) {
        ControlApplicationMemoryModeOverrideConfig* mode = (ControlApplicationMemoryModeOverrideConfig*)&cmdbuf[1];

        memset(mode, 0, sizeof(ControlApplicationMemoryModeOverrideConfig));
        mode->query = true;
        cmdbuf[0] = IPC_MakeHeader(0x101, 1, 0);

        if (R_SUCCEEDED((res = svcSendSyncRequest(loaderHandle))) && R_SUCCEEDED(res = cmdbuf[1]))
        {
            memcpy(&g_memorymodeoverridebackup, &cmdbuf[2], sizeof(ControlApplicationMemoryModeOverrideConfig));

            memset(mode, 0, sizeof(ControlApplicationMemoryModeOverrideConfig));
            mode->enable_o3ds = true;
            mode->o3ds_mode = SYSMODE_DEV2;
            cmdbuf[0] = IPC_MakeHeader(0x101, 1, 0);
            if (R_SUCCEEDED((res = svcSendSyncRequest(loaderHandle)))) {
                res = cmdbuf[1];
            }
        }
    } else {
        ControlApplicationMemoryModeOverrideConfig* mode = (ControlApplicationMemoryModeOverrideConfig*)&cmdbuf[1];
        *mode = g_memorymodeoverridebackup;
        cmdbuf[0] = IPC_MakeHeader(0x101, 1, 0);
        if (R_SUCCEEDED((res = svcSendSyncRequest(loaderHandle)))) {
            res = cmdbuf[1];
        }
    }

    svcCloseHandle(loaderHandle);
    return res;
}
static void j_PluginLoader__SetMode3AppMode(void* arg) {(void)arg; PluginLoader__SetMode3AppMode(false);}

void     PluginLoader__HandleCommands(void *_ctx)
{
    (void)_ctx;

    u32    *cmdbuf = getThreadCommandBuffer();
    PluginLoaderContext *ctx = &PluginLoaderCtx;

    switch (cmdbuf[0] >> 16)
    {
        case 1:
        {
            if (cmdbuf[0] != IPC_MakeHeader(1, 2, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            ctx->plgEvent = PLG_OK;
            svcOpenProcess(&ctx->target, cmdbuf[1]);

            if (ctx->useUserLoadParameters && ctx->userLoadParameters.pluginMemoryStrategy == PLG_STRATEGY_MODE3)
                TaskRunner_RunTask(j_PluginLoader__SetMode3AppMode, NULL, 0);

            bool flash = !(ctx->useUserLoadParameters && ctx->userLoadParameters.noFlash);
            if (!g_safeMode && ctx->isEnabled && TryToLoadPlugin(ctx->target, cmdbuf[2]))
            {
                if (flash)
                {

                    for (u32 i = 0; i < 64; i++)
                    {
                        REG32(0x10202204) = 0x01FF9933;
                        svcSleepThread(5000000);
                    }
                    REG32(0x10202204) = 0;
                }
                PLG__SetConfigMemoryStatus(PLG_CFG_RUNNING);
            }
            else
            {
                svcCloseHandle(ctx->target);
                ctx->target = 0;
            }

            cmdbuf[0] = IPC_MakeHeader(1, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 2:
        {
            if (cmdbuf[0] != IPC_MakeHeader(2, 0, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            cmdbuf[0] = IPC_MakeHeader(2, 2, 0);
            cmdbuf[1] = 0;
            cmdbuf[2] = (u32)ctx->isEnabled;
            break;
        }

        case 3:
        {
            if (cmdbuf[0] != IPC_MakeHeader(3, 1, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            if (cmdbuf[1] != ctx->isEnabled)
            {
                ctx->isEnabled = cmdbuf[1];
                LumaConfig_RequestSaveSettings();
                PluginLoader__UpdateMenu();
            }

            cmdbuf[0] = IPC_MakeHeader(3, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 4:
        {
            if (cmdbuf[0] != IPC_MakeHeader(4, 2, 4))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            PluginLoadParameters *params = &ctx->userLoadParameters;

            ctx->useUserLoadParameters = true;
            params->noFlash = cmdbuf[1] & 0xFF;
            params->pluginMemoryStrategy = (cmdbuf[1] >> 8) & 0xFF;
            params->persistent = (cmdbuf[1] >> 16) & 0x1;
            params->lowTitleId = cmdbuf[2];

            strncpy(params->path, (const char *)cmdbuf[4], 255);
            memcpy(params->config, (void *)cmdbuf[6], 32 * sizeof(u32));

            if (params->persistent)
            {
                IFile file;
                if (R_SUCCEEDED(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/plugins/user_param.bin"),
                                           FS_OPEN_CREATE | FS_OPEN_READ | FS_OPEN_WRITE))) {
                    u64 tempWritten;
                    u32 magic = PERS_USER_FILE_MAGIC;
                    IFile_Write(&file, &tempWritten, &magic, sizeof(magic), 0);
                    IFile_Write(&file, &tempWritten, params, sizeof(PluginLoadParameters), 0);
                    IFile_Close(&file);
                }
            }

            if (params->pluginMemoryStrategy == PLG_STRATEGY_MODE3)
                cmdbuf[1] = PluginLoader__SetMode3AppMode(true);
            else
                cmdbuf[1] = 0;

            cmdbuf[0] = IPC_MakeHeader(4, 1, 0);
            break;
        }

        case 5:
        {
            if (cmdbuf[0] != IPC_MakeHeader(5, 1, 8))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            u32 nbItems = cmdbuf[1];
            u32 states = cmdbuf[3];
            DisplayPluginMenu(cmdbuf);

            cmdbuf[0] = IPC_MakeHeader(5, 1, 2);
            cmdbuf[1] = 0;
            cmdbuf[2] = IPC_Desc_Buffer(nbItems, IPC_BUFFER_RW);
            cmdbuf[3] = states;
            break;
        }

        case 6:
        {
            if (cmdbuf[0] != IPC_MakeHeader(6, 0, 4))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            const char *title = (const char *)cmdbuf[2];
            const char *body = (const char *)cmdbuf[4];

            DispMessage(title, body);

            cmdbuf[0] = IPC_MakeHeader(6, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 7:
        {
            if (cmdbuf[0] != IPC_MakeHeader(7, 1, 4))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            const char *title = (const char *)cmdbuf[3];
            const char *body = (const char *)cmdbuf[5];

            DispErrMessage(title, body, cmdbuf[1]);

            cmdbuf[0] = IPC_MakeHeader(7, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 8:
        {
            if (cmdbuf[0] != IPC_MakeHeader(8, 0, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            cmdbuf[0] = IPC_MakeHeader(8, 2, 0);
            cmdbuf[1] = 0;
            cmdbuf[2] = PLGLDR_VERSION;
            break;
        }

        case 9:
        {
            if (cmdbuf[0] != IPC_MakeHeader(9, 0, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            cmdbuf[0] = IPC_MakeHeader(9, 1, 2);
            cmdbuf[1] = 0;
            cmdbuf[2] = IPC_Desc_SharedHandles(1);
            cmdbuf[3] = ctx->arbiter;
            break;
        }

        case 10:
        {
            if (cmdbuf[0] != IPC_MakeHeader(10, 0, 2))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            u32 descriptor = cmdbuf[1];
            u32 pathSize = descriptor >> 4;
            u32 mode = descriptor & 15;
            if (!cmdbuf[2] || !ctx->pluginPath || pathSize < 2 || pathSize > 256 ||
                (mode != (IPC_Desc_Buffer(1, IPC_BUFFER_W) & 15) &&
                 mode != (IPC_Desc_Buffer(1, IPC_BUFFER_RW) & 15)))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }
            u32 length = strnlen(ctx->pluginPath, 256);
            if (length >= pathSize)
            {
                error(cmdbuf, MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, RD_TOO_LARGE));
                break;
            }
            char *path = (char *)cmdbuf[2];
            memcpy(path, ctx->pluginPath, length + 1);

            cmdbuf[0] = IPC_MakeHeader(10, 1, 2);
            cmdbuf[1] = 0;
            cmdbuf[2] = descriptor;
            cmdbuf[3] = (u32)path;

            break;
        }

        case 11:
        {
            if (cmdbuf[0] != IPC_MakeHeader(11, 1, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            g_blockMenuOpen = cmdbuf[1];

            cmdbuf[0] = IPC_MakeHeader(11, 1, 0);
            cmdbuf[1] = 0;
            break;
        }

        case 12:
        {
            if (cmdbuf[0] != IPC_MakeHeader(12, 2, 4))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }
            cmdbuf[0] = IPC_MakeHeader(12, 1, 0);
            u32 pathSize = cmdbuf[5] >> 4;
            u32 physicalSave = cmdbuf[1] & 0x7FFFFFFF, physicalLoad = cmdbuf[2] & 0x7FFFFFFF;
            u32 physicalEnd = isN3DS ? 0x30000000 : 0x28000000;
            if(cmdbuf[3] != IPC_Desc_Buffer(16, IPC_BUFFER_R) || !cmdbuf[4] || !cmdbuf[6] ||
               (cmdbuf[5] & 15) != (IPC_Desc_Buffer(1, IPC_BUFFER_R) & 15) || !pathSize || pathSize > 256 ||
               (physicalSave & 3) || (physicalLoad & 3) || physicalSave < 0x20000000 || physicalLoad < 0x20000000 ||
               physicalSave > physicalEnd - 128 || physicalLoad > physicalEnd - 128)
            {
                cmdbuf[1] = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, RD_INVALID_ADDRESS);
                break;
            }

            u32* remoteSavePhysAddr = (u32*)(cmdbuf[1] | (1 << 31));
            u32* remoteLoadPhysAddr = (u32*)(cmdbuf[2] | (1 << 31));

            Result ret = MemoryBlock__SetSwapSettings(remoteSavePhysAddr, false, (u32*)cmdbuf[4]);
            if (!ret) ret = MemoryBlock__SetSwapSettings(remoteLoadPhysAddr, true, (u32*)cmdbuf[4]);

            if (ret) {
                cmdbuf[1] = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, RD_TOO_LARGE);
                MemoryBlock__ResetSwapSettings();
                break;
            }

            ctx->isSwapFunctionset = true;

            svcInvalidateEntireInstructionCache();

            cmdbuf[1] = 0;
            break;
        }

        case 13:
        {
            if (cmdbuf[0] != IPC_MakeHeader(13, 1, 2))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }
            cmdbuf[0] = IPC_MakeHeader(13, 1, 0);
            u32 physical = cmdbuf[1] & 0x7FFFFFFF;
            if(cmdbuf[2] != IPC_Desc_Buffer(16, IPC_BUFFER_R) || !cmdbuf[3] || (physical & 3) ||
               physical < 0x20000000 || physical > (isN3DS ? 0x30000000u : 0x28000000u) - 128)
            {
                cmdbuf[1] = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, RD_INVALID_ADDRESS);
                break;
            }

            u32* remoteLoadExeFuncAddr = (u32*)(cmdbuf[1] | (1 << 31));
            Result ret = Set_3gx_LoadParams(remoteLoadExeFuncAddr, (u32*)cmdbuf[3]);
            if (ret)
            {
                cmdbuf[1] = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, RD_TOO_LARGE);
                Reset_3gx_LoadParams();
                break;
            }

            ctx->isExeLoadFunctionset = true;

            svcInvalidateEntireInstructionCache();

            cmdbuf[1] = 0;
            break;
        }

        case 14:
        {
            if (cmdbuf[0] != IPC_MakeHeader(14, 0, 0))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            ctx->useUserLoadParameters = false;
            memset(&ctx->userLoadParameters, 0, sizeof(PluginLoadParameters));

            FS_Archive sd;
            if(R_SUCCEEDED(FSUSER_OpenArchive(&sd, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
            {
                FSUSER_DeleteFile(sd, fsMakePath(PATH_ASCII, "/luma/plugins/user_param.bin"));
                FSUSER_CloseArchive(sd);
            }

            cmdbuf[1] = 0;
            break;
        }

        default:
        {
            error(cmdbuf, 0xD900182F);
            break;
        }
    }

    if (ctx->error.message)
    {
        PluginLoader__Error(ctx->error.message, ctx->error.code);
        ctx->error.message = NULL;
        ctx->error.code = 0;
    }
}

static bool     ThreadPredicate(u32 *kthread)
{

    u32 *tls = (u32 *)kthread[0x26];

    return *tls == THREADVARS_MAGIC;
}

static void     __strex__(s32 *addr, s32 val)
{
    do
        __ldrex(addr);
    while (__strex(addr, val));
}

void    PLG__NotifyEvent(PLG_Event event, bool signal)
{
    if (PluginLoaderCtx.eventsSelfManaged || !PluginLoaderCtx.plgEventPA) return;

    __strex__(PluginLoaderCtx.plgEventPA, event);
    if (signal)
        svcArbitrateAddress(PluginLoaderCtx.arbiter, (u32)PluginLoaderCtx.plgEventPA, ARBITRATION_SIGNAL, 1, 0);
}

void    PLG__WaitForReply(void)
{
    if (PluginLoaderCtx.eventsSelfManaged) return;
    __strex__(PluginLoaderCtx.plgReplyPA, PLG_WAIT);
    svcArbitrateAddress(PluginLoaderCtx.arbiter, (u32)PluginLoaderCtx.plgReplyPA, ARBITRATION_WAIT_IF_LESS_THAN_TIMEOUT, PLG_OK, MemoryBlock__GetMappedPluginHeader()->waitForReplyTimeout);
}

void     PLG__SetConfigMemoryStatus(u32 status)
{
    *(vu32 *)PA_FROM_VA_PTR(0x1FF800F0) = status;
}

u32      PLG__GetConfigMemoryStatus(void)
{
    return (*(vu32 *)PA_FROM_VA_PTR((u32 *)0x1FF800F0)) & 0xFFFF;
}

u32      PLG__GetConfigMemoryEvent(void)
{
    return (*(vu32 *)PA_FROM_VA_PTR(0x1FF800F0)) & ~0xFFFF;
}

static void WaitForProcessTerminated(void *arg)
{
    (void)arg;
    PluginLoaderContext *ctx = &PluginLoaderCtx;


    for (u32 i = 0; svcWaitSynchronization(ctx->target, 0) != 0 && i < 50; i++) svcSleepThread(50000000);


    if (!ctx->pluginIsSwapped) {
        MemoryBlock__UnmountFromProcess();
        MemoryBlock__Free();
    }

    svcCloseHandle(ctx->target);

    PLG__SetConfigMemoryStatus(PLG_CFG_NONE);
    ctx->pluginIsSwapped = false;
    ctx->pluginIsHome = false;
    ctx->target = 0;
    ctx->isExeLoadFunctionset = false;
    ctx->isSwapFunctionset = false;
    ctx->pluginMemoryStrategy = PLG_STRATEGY_SWAP;
    ctx->eventsSelfManaged = false;
    ctx->isMemPrivate = false;
    g_blockMenuOpen = 0;
    MemoryBlock__ResetSwapSettings();
}

void    PluginLoader__HandleKernelEvent(u32 notifId)
{
    (void)notifId;
    if (PLG__GetConfigMemoryStatus() == PLG_CFG_EXITING)
    {
        srvPublishToSubscriber(0x1002, 0);
        return;
    }

    PluginLoaderContext *ctx = &PluginLoaderCtx;
    u32 event = PLG__GetConfigMemoryEvent();

    if (event == PLG_CFG_EXIT_EVENT)
    {
        PLG__SetConfigMemoryStatus(PLG_CFG_EXITING);
        if (!ctx->pluginIsSwapped)
        {

            PLG__NotifyEvent(PLG_ABOUT_TO_EXIT, false);

            PLG__WaitForReply();
        }

        TaskRunner_RunTask(WaitForProcessTerminated, NULL, 0);
    }
    else if (event == PLG_CFG_HOME_EVENT)
    {
        if ((ctx->pluginMemoryStrategy == PLG_STRATEGY_SWAP) && !isN3DS) {
            if (ctx->pluginIsSwapped)
            {

                MemoryBlock__IsReady();
                MemoryBlock__FromSwapFile();
                MemoryBlock__MountInProcess();

                svcControlProcess(ctx->target, PROCESSOP_SCHEDULE_THREADS, 0, (u32)ThreadPredicate);

                PLG__NotifyEvent(PLG_OK, true);
                PLG__SetConfigMemoryStatus(PLG_CFG_RUNNING);
            }
            else
            {

                PLG__NotifyEvent(PLG_ABOUT_TO_SWAP, false);

                PLG__WaitForReply();

                svcControlProcess(ctx->target, PROCESSOP_SCHEDULE_THREADS, 1, (u32)ThreadPredicate);

                MemoryBlock__UnmountFromProcess();
                MemoryBlock__ToSwapFile();
                MemoryBlock__Free();
                PLG__SetConfigMemoryStatus(PLG_CFG_INHOME);
            }
            ctx->pluginIsSwapped = !ctx->pluginIsSwapped;
        } else {


            volatile PluginHeader* mappedHeader = MemoryBlock__GetMappedPluginHeader();
            mappedHeader = mappedHeader ? PA_FROM_VA_PTR(mappedHeader) : NULL;
            bool doNotification = mappedHeader ? mappedHeader->notifyHomeEvent : false;
            if (ctx->pluginIsHome)
            {
                if (doNotification) {

                    PLG__NotifyEvent(PLG_HOME_EXIT, false);

                    PLG__WaitForReply();
                }
                PLG__SetConfigMemoryStatus(PLG_CFG_RUNNING);
            }
            else
            {
                if (doNotification) {

                    PLG__NotifyEvent(PLG_HOME_ENTER, false);

                    PLG__WaitForReply();
                }
                PLG__SetConfigMemoryStatus(PLG_CFG_INHOME);
            }
            ctx->pluginIsHome = !ctx->pluginIsHome;
        }
    }
    srvPublishToSubscriber(0x1002, 0);
}
