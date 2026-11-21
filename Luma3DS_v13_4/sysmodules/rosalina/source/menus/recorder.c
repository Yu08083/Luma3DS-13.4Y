#include <3ds.h>
#include <string.h>
#include "recorder.h"
#include "recorder_protocol.h"
#include "draw.h"
#include "fmt.h"
#include "menu.h"
#include "menus/lang.h"
#include "remote_play.h"
#include "MyThread.h"

#define REC_PORT_ABSENT ((Result)0xD88007FA)
#define REC_TIMEOUT ((Result)-3)
#define REC_OFFLINE ((Result)-5)
#define REC_CHECKING ((Result)1)

enum { REC_CHECK = 0, REC_LAUNCH = YU_REC_CMD_SAVE_CLIP + 1 };

static MyThread recorderThread;
static u32 recorderStack[0x800 / sizeof(u32)] __attribute__((aligned(8)));
static u32 workAction, workStatus[12], workCancelled;
static Result workResult;
static u64 workStart;

static bool recorderCancelled(void)
{
    return __atomic_load_n(&workCancelled, __ATOMIC_ACQUIRE) != 0;
}

static Result recorderRequest(u32 command, u32 action, u32 status[12])
{
    Handle session;
    Result result = svcConnectToPort(&session, YU_REC_PORT);
    if(result != 0)
        return result == REC_PORT_ABSENT ? REC_OFFLINE : result;
    u32 *buffer = getThreadCommandBuffer();
    buffer[0] = command == 1 ? YU_REC_STATUS_REQUEST : YU_REC_COMMAND_REQUEST;
    buffer[1] = action;
    result = recorderCancelled() ? REC_TIMEOUT : svcSendSyncRequest(session);
    if(result == 0)
    {
        if(buffer[0] != (command == 1 ? YU_REC_STATUS_REPLY : YU_REC_COMMAND_REPLY))
            result = (Result)YU_REC_ERROR_INVALID;
        else
        {
            result = (Result)buffer[1];
            if(result == 0 && command == 1)
            {
                if(buffer[2] != YU_REC_MAGIC || buffer[3] != YU_REC_PROTOCOL_VERSION || buffer[4] > YU_REC_REPLAY)
                    result = (Result)YU_REC_ERROR_INVALID;
                else if(status)
                    memcpy(status, buffer + 1, 12 * sizeof(u32));
            }
        }
    }
    svcCloseHandle(session);
    return result;
}

static bool recorderFinished(void)
{
    if(svcWaitSynchronization(recorderThread.handle, 0) != 0)
        return false;
    svcCloseHandle(recorderThread.handle);
    recorderThread.handle = 0;
    return true;
}

bool Recorder_IsRunning(void)
{
    if(recorderThread.handle && !recorderFinished())
        return true;
    Handle session;
    Result result = svcConnectToPort(&session, YU_REC_PORT);
    if(result != 0)
        return result != REC_PORT_ABSENT;
    svcCloseHandle(session);
    return true;
}

static Result recorderLaunch(void)
{
    s64 options = 0;
    Result result = svcGetSystemInfo(&options, 0x10000, 3);
    if(R_FAILED(result))
        return result;
    if(!(options & 2))
        return -2;
    if(recorderCancelled())
        return REC_TIMEOUT;
    const FS_ProgramInfo program = {.programId = YU_REC_TITLE_ID, .mediaType = MEDIATYPE_NAND};
    return PMAPP_LaunchTitle(&program, 0);
}

static void recorderWork(void)
{
    if(recorderCancelled())
        workResult = REC_TIMEOUT;
    else if(workAction == REC_LAUNCH)
        workResult = recorderLaunch();
    else
        workResult = recorderRequest(workAction == REC_CHECK ? 1 : 2, workAction, workStatus);
}

static Result startRecorderWork(u32 action, u64 now)
{
    workAction = action;
    workStart = now;
    __atomic_store_n(&workCancelled, 0, __ATOMIC_RELEASE);
    Result result = MyThread_Create(&recorderThread, recorderWork, recorderStack, sizeof(recorderStack), 54, CORE_SYSTEM);
    if(result != 0)
        recorderThread.handle = 0;
    return result;
}

static u32 recordAction(Result statusResult, u32 state)
{
    if(statusResult == REC_OFFLINE) return REC_LAUNCH;
    if(statusResult != 0) return REC_CHECK;
    if(state == YU_REC_RECORDING) return YU_REC_CMD_STOP;
    return state == YU_REC_IDLE || state == YU_REC_REPLAY ? YU_REC_CMD_START : REC_CHECK;
}

static void drawRecorder(const u32 status[12], Result statusResult, Result result, u32 pending)
{
    static const char *const states[] = {"Starting...", "Ready", "Recording", "Saving...", "Error", "OFF", "Replay"};
    bool ready = statusResult == 0;
    u32 state = ready ? status[3] : YU_REC_EXITED;
    const char *stateText = pending == REC_LAUNCH && statusResult == REC_OFFLINE ? states[YU_REC_STARTING] :
                            ready ? states[state] : statusResult == REC_CHECKING ? "Waiting for connection" :
                            statusResult == REC_OFFLINE ? "OFF" : "Error";
    Draw_Lock();
    Draw_ClearFramebuffer();
    Draw_DrawString(10, 10, g_accentColor, Lang_Translate("SD recording"));
    Draw_DrawString(10, 38, g_textColor, Lang_Translate(stateText));
    if(ready)
        Draw_DrawFormattedString(10, 60, g_textColor, Lang_Translate("Time: %lu s  Files: %lu"),
                                 (unsigned long)status[4], (unsigned long)status[9]);
    Draw_DrawString(10, 92, g_textColor, "/3ds/recorder/recordings/");
    Draw_DrawString(10, 114, g_textColor, Lang_Translate("MP4 / settings: Yu_ Recorder"));
    Result error = statusResult != REC_OFFLINE && R_FAILED(statusResult) ? statusResult : result;
    if(!error && ready)
        error = (Result)status[10];
    if(error == -2)
        Draw_DrawString(10, 146, g_textColor, Lang_Translate("Enable external modules at boot."));
    else if(error == -4)
        Draw_DrawString(10, 146, g_textColor, Lang_Translate("Stop Yu_ Capture first."));
    else if(error == REC_TIMEOUT)
        Draw_DrawString(10, 146, g_textColor, Lang_Translate("Recorder is not responding."));
    else if(R_FAILED(error))
        Draw_DrawFormattedString(10, 146, g_textColor, Lang_Translate("Operation failed (0x%08lx)."), (unsigned long)(u32)error);
    if(!pending && recordAction(statusResult, state))
        Draw_DrawString(10, 188, g_cursorColor, Lang_Translate(state == YU_REC_RECORDING ? "A: stop" : "A: record"));
    Draw_DrawString(10, 210, g_cursorColor, Lang_Translate(ready && state != YU_REC_EXITED && !pending ? "Y: shut down  B: back" : "B: back"));
    Draw_FlushFramebuffer();
    Draw_Unlock();
}

void RosalinaMenu_Recorder(void)
{
    u32 status[12] = {0}, requested = 0, pending = 0;
    Result statusResult = REC_CHECKING, result = 0;
    u64 nextPoll = 0, deadline = 0;
    bool redraw = true;
    while(!menuShouldExit)
    {
        u64 now = svcGetSystemTick();
        if(recorderThread.handle)
        {
            if(recorderFinished())
            {
                if(!recorderCancelled())
                {
                    if(workAction == REC_CHECK)
                    {
                        statusResult = workResult;
                        if(workResult == 0)
                            memcpy(status, workStatus, sizeof(status));
                        if(pending == REC_LAUNCH)
                        {
                            if(workResult != REC_OFFLINE && (workResult || status[3] != YU_REC_STARTING))
                            {
                                if(recordAction(workResult, status[3]) == YU_REC_CMD_START) requested = YU_REC_CMD_START;
                                else if(!workResult && status[3] == YU_REC_ERROR && !R_FAILED((Result)status[10]))
                                    result = (Result)YU_REC_ERROR_STATE;
                                pending = 0;
                            }
                        }
                        else if(workResult || status[3] == YU_REC_EXITED || (pending != YU_REC_CMD_EXIT &&
                            (recordAction(workResult, status[3]) != pending || R_FAILED((Result)status[10]))))
                            pending = 0;
                        if(pending && now >= deadline)
                        {
                            result = REC_TIMEOUT;
                            pending = 0;
                        }
                        nextPoll = now + SYSCLOCK_ARM11 / 2;
                    }
                    else
                    {
                        result = workResult;
                        if(!result)
                        {
                            pending = workAction;
                            deadline = now + (pending == REC_LAUNCH ? 120ULL : 30ULL) * SYSCLOCK_ARM11;
                        }
                        statusResult = REC_CHECKING;
                    }
                }
                redraw = true;
            }
            else if(now - workStart >= 4ULL * SYSCLOCK_ARM11 && (!recorderCancelled() || statusResult != REC_TIMEOUT))
            {
                __atomic_store_n(&workCancelled, 1, __ATOMIC_RELEASE);
                statusResult = REC_TIMEOUT;
                result = 0;
                requested = 0;
                pending = 0;
                redraw = true;
            }
        }
        if(redraw)
        {
            drawRecorder(status, statusResult, result, pending);
            redraw = false;
        }
        if(!recorderThread.handle)
        {
            u32 action = REC_CHECK;
            if(requested == YU_REC_CMD_EXIT ? statusResult == 0 && status[3] != YU_REC_EXITED :
               requested == recordAction(statusResult, status[3]))
                action = requested;
            requested = 0;
            if((action == REC_LAUNCH || action == YU_REC_CMD_START) && RemotePlay_IsRunning())
            {
                result = -4;
                redraw = true;
            }
            else if(action != REC_CHECK || now >= nextPoll)
            {
                Result started = startRecorderWork(action, now);
                nextPoll = action == REC_CHECK ? now + SYSCLOCK_ARM11 / 2 : 0;
                if(started != 0)
                {
                    statusResult = started;
                    result = 0;
                    nextPoll = now + SYSCLOCK_ARM11 / 2;
                    redraw = true;
                }
            }
        }
        u32 keys = waitInputWithTimeout(100);
        if(keys & KEY_B)
            break;
        if(!pending && (!recorderThread.handle || (workAction == REC_CHECK && !recorderCancelled())))
        {
            if((keys & KEY_Y) && statusResult == 0 && status[3] != YU_REC_EXITED)
                requested = YU_REC_CMD_EXIT;
            else if(keys & KEY_A)
                requested = recordAction(statusResult, status[3]);
        }
    }
    if(recorderThread.handle)
        __atomic_store_n(&workCancelled, 1, __ATOMIC_RELEASE);
}
