/*
*   This file is part of Luma3DS
*   Copyright (C) 2023 Aurora Wright, TuxSH
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
#include "shell.h"
#include "utils.h"
#include "screen_filters.h"
#include "luma_config.h"

static void forceAudioOutput(u32 forceOp)
{













    svcSleepThread(20 * 1000 * 1000LL);

    Handle *cdcChkHandlePtr = cdcChkGetSessionHandle();
    *cdcChkHandlePtr = 0;

    Result res = srvGetServiceHandle(cdcChkHandlePtr, "cdc:CHK");

    if (R_FAILED(res))
        res = svcControlService(SERVICEOP_STEAL_CLIENT_SESSION, cdcChkHandlePtr, "cdc:CHK");

    if (R_FAILED(res))
        return;

    u8 reg;
    switch (forceOp) {
        case 0:
        default:
            __builtin_trap();
            break;
        case 1:
            reg = 0x30;
            break;
        case 2:
            reg = 0x20;
            break;
    }
    res = CDCCHK_WriteRegisters2(100, 69, &reg, 1);

    svcCloseHandle(*cdcChkHandlePtr);
}

void handleShellOpened(void)
{



    s64 out = 0;
    svcGetSystemInfo(&out, 0x10000, 4);
    u32 multiConfig = (u32)out;
    u32 forceOp = (multiConfig >> (2 * (u32)FORCEAUDIOOUTPUT)) & 3;





    if (isServiceUsable("gsp::Gpu"))
        ScreenFiltersMenu_RestoreSettings();

    if (forceOp != 0 && isServiceUsable("cdc:CHK"))
        forceAudioOutput(forceOp);
}
