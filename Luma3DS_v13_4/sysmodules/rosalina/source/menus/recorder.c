#include <3ds.h>
#include <string.h>
#include "recorder.h"
#include "recorder_protocol.h"
#include "draw.h"
#include "fmt.h"
#include "menu.h"
#include "menus/lang.h"
#include "remote_play.h"

static Result recorderRequest(u32 command, u32 action, u32 status[12])
{
    Handle session;
    Result result = svcConnectToPort(&session, YU_REC_PORT);
    if(R_FAILED(result))
        return result;
    u32 *buffer = getThreadCommandBuffer();
    buffer[0] = command == 1 ? YU_REC_STATUS_REQUEST : YU_REC_COMMAND_REQUEST;
    buffer[1] = action;
    result = svcSendSyncRequest(session);
    if(R_SUCCEEDED(result))
    {
        if(buffer[0] != (command == 1 ? YU_REC_STATUS_REPLY : YU_REC_COMMAND_REPLY))
            result = (Result)YU_REC_ERROR_INVALID;
        else
        {
            result = (Result)buffer[1];
            if(R_SUCCEEDED(result) && command == 1)
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

bool Recorder_IsRunning(void)
{
    Handle session;
    if(R_FAILED(svcConnectToPort(&session, YU_REC_PORT)))
        return false;
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
    const FS_ProgramInfo program = {.programId = YU_REC_TITLE_ID, .mediaType = MEDIATYPE_NAND};
    result = PMAPP_LaunchTitle(&program, 0);
    if(R_FAILED(result))
        return result;
    for(u32 attempt = 0; attempt < 60 && !menuShouldExit; ++attempt)
    {
        u32 status[12];
        if(R_SUCCEEDED(recorderRequest(1, 0, status)))
        {
            if(status[3] == YU_REC_IDLE || status[3] == YU_REC_REPLAY)
                return recorderRequest(2, YU_REC_CMD_START, NULL);
            if(status[3] == YU_REC_RECORDING)
                return 0;
            if(status[3] == YU_REC_ERROR)
                return R_FAILED((Result)status[10]) ? (Result)status[10] : (Result)YU_REC_ERROR_STATE;
        }
        svcSleepThread(50000000LL);
    }
    return -3;
}

void RosalinaMenu_Recorder(void)
{
    Result result = 0;
    while(!menuShouldExit)
    {
        u32 status[12] = {0};
        Result statusResult = recorderRequest(1, 0, status);
        bool ready = R_SUCCEEDED(statusResult);
        u32 state = ready ? status[3] : YU_REC_EXITED;
        const char *stateText = "OFF";
        if(ready)
        {
            static const char *const states[] = {"Starting...", "Ready", "Recording", "Saving...", "Error", "OFF", "Replay"};
            stateText = state < 7 ? states[state] : "Error";
        }
        Draw_Lock();
        Draw_ClearFramebuffer();
        Draw_DrawString(10, 10, g_accentColor, Lang_Translate("SD recording"));
        Draw_DrawString(10, 38, g_textColor, Lang_Translate(stateText));
        if(ready)
            Draw_DrawFormattedString(10, 60, g_textColor, Lang_Translate("Time: %lu s  Files: %lu"),
                                     (unsigned long)status[4], (unsigned long)status[9]);
        Draw_DrawString(10, 92, g_textColor, "/3ds/recorder/recordings/");
        Draw_DrawString(10, 114, g_textColor, Lang_Translate("MP4 / settings: Yu_ Recorder"));
        Result error = result ? result : (ready ? (Result)status[10] : 0);
        if(error == -2)
            Draw_DrawString(10, 146, g_textColor, Lang_Translate("Enable external modules at boot."));
        else if(error == -4)
            Draw_DrawString(10, 146, g_textColor, Lang_Translate("Stop Yu_ Capture first."));
        else if(R_FAILED(error))
            Draw_DrawFormattedString(10, 146, g_textColor, Lang_Translate("Operation failed (0x%08lx)."), (unsigned long)(u32)error);
        Draw_DrawString(10, 188, g_cursorColor, Lang_Translate(state == YU_REC_RECORDING ? "A: stop" : "A: record"));
        Draw_DrawString(10, 210, g_cursorColor, Lang_Translate("Y: shut down  B: back"));
        Draw_FlushFramebuffer();
        Draw_Unlock();
        u32 keys = waitInputWithTimeout(500);
        if(keys & KEY_B)
            break;
        if((keys & KEY_Y) && ready)
            result = recorderRequest(2, YU_REC_CMD_EXIT, NULL);
        else if(keys & KEY_A)
        {
            if(state != YU_REC_RECORDING && RemotePlay_IsRunning())
                result = -4;
            else if(!ready)
                result = Recorder_IsRunning() ? statusResult : recorderLaunch();
            else
                result = recorderRequest(2, state == YU_REC_RECORDING ? YU_REC_CMD_STOP : YU_REC_CMD_START, NULL);
        }
    }
}
