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
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PlgUdsReject,
    PlgUdsForward,
    PlgUdsInitialize,
    PlgUdsFinalize,
    PlgUdsSignalState,
    PlgUdsPause
} PlgUdsAction;

typedef struct {
    void *context;
    bool (*range)(void *context, uint32_t address, uint32_t size, bool write);
    bool (*handle)(void *context, uint32_t handle, bool sharedMemory);
    uint32_t processId;
} PlgUdsValidation;

static inline uint32_t PlgUdsHeader(uint32_t command, uint32_t normal, uint32_t translate)
{
    return (command << 16) | (normal << 6) | translate;
}

static inline bool PlgUdsRange(const PlgUdsValidation *validation, uint32_t address, uint32_t size, bool write)
{
    if (size > 0x100000 || address >= 0x40000000 || size > 0x40000000 - address) return false;
    if (!size) return true;
    return address >= 0x1000 && validation->range(validation->context, address, size, write);
}

static inline bool PlgUdsStatic(const PlgUdsValidation *validation, uint32_t descriptor, uint32_t address, uint32_t id, uint32_t minimum, bool write)
{
    if ((descriptor & 0x3FFF) != ((id << 10) | 2) || (descriptor >> 14) < minimum) return false;
    return PlgUdsRange(validation, address, descriptor >> 14, write);
}

static inline PlgUdsAction PlgUdsPrepare(uint32_t command[64], const uint32_t staticBuffers[32], const PlgUdsValidation *validation)
{
    if (!command || !staticBuffers || !validation || !validation->range || !validation->handle) return PlgUdsReject;
    uint32_t header = command[0];
    uint32_t id = header >> 16;
    if ((header & 0xF000) || ((header >> 6) & 63) + (header & 63) > 63) return PlgUdsReject;
    uint32_t normal = 0;
    switch (id)
    {
        case 100:
            return header == PlgUdsHeader(100, 2, 0) && command[1] == validation->processId && command[2] == 0 ? PlgUdsInitialize : PlgUdsReject;
        case 101:
            return header == PlgUdsHeader(101, 0, 0) ? PlgUdsFinalize : PlgUdsReject;
        case 104:
            return header == PlgUdsHeader(104, 1, 0) && command[1] <= 1 ? PlgUdsSignalState : PlgUdsReject;
        case 105:
            return header == PlgUdsHeader(105, 1, 0) && command[1] <= 1 ? PlgUdsPause : PlgUdsReject;
        case 3: case 6: case 8: case 10: case 11: case 26:
            return header == PlgUdsHeader(id, 0, 0) ? PlgUdsForward : PlgUdsReject;
        case 5: case 13: case 19:
            return header == PlgUdsHeader(id, 1, 0) ? PlgUdsForward : PlgUdsReject;
        case 7: case 21: case 33:
            return header == PlgUdsHeader(id, 2, 0) ? PlgUdsForward : PlgUdsReject;
        case 18:
            return header == PlgUdsHeader(id, 4, 0) ? PlgUdsForward : PlgUdsReject;
        case 14: case 31:
            if (header != PlgUdsHeader(id, 0, 6)
                || !PlgUdsStatic(validation, command[1], command[2], 1, 0x108, false)
                || !PlgUdsStatic(validation, command[3], command[4], 2, 0xFE, false)
                || !PlgUdsStatic(validation, command[5], command[6], 3, 0xFE, false)
                || !PlgUdsStatic(validation, staticBuffers[0], staticBuffers[1], 0, 0x280, true)) return PlgUdsReject;
            command[7] = staticBuffers[1];
            command[0] = PlgUdsHeader(31, 7, 0);
            return PlgUdsForward;
        case 15:
            if (header != PlgUdsHeader(id, 16, 4) || command[1] < 12 || command[17] != 0
                || !validation->handle(validation->context, command[18], false)
                || (command[19] & 15) != 12 || (command[19] >> 4) != command[1]
                || !PlgUdsRange(validation, command[20], command[1], true)) return PlgUdsReject;
            normal = 20;
            break;
        case 16:
            if (header != PlgUdsHeader(id, 1, 2) || command[1] > 0xC8
                || !PlgUdsStatic(validation, command[2], command[3], 4, command[1], false)) return PlgUdsReject;
            normal = 3;
            break;
        case 17:
            if (header != PlgUdsHeader(id, 1, 0) || command[1] > 0xC8
                || !PlgUdsStatic(validation, staticBuffers[0], staticBuffers[1], 0, command[1], true)) return PlgUdsReject;
            command[2] = staticBuffers[1];
            normal = 2;
            break;
        case 20:
            if (header != PlgUdsHeader(id, 3, 0) || command[2] > 0x40000 || command[3] > command[2] * 4
                || !PlgUdsStatic(validation, staticBuffers[0], staticBuffers[1], 0, command[2] * 4, true)) return PlgUdsReject;
            command[4] = staticBuffers[1];
            normal = 4;
            break;
        case 23:
            if (header != PlgUdsHeader(id, 6, 2) || command[4] > 0x40000 || command[5] > command[4] * 4
                || (command[7] >> 14) != command[4] * 4
                || !PlgUdsStatic(validation, command[7], command[8], 5, command[5], false)) return PlgUdsReject;
            normal = 8;
            break;
        case 27:
            if (header != PlgUdsHeader(id, 12, 2) || !command[1] || command[1] > 0x100000 || command[1] & 0xFFF
                || command[13] != 0 || !validation->handle(validation->context, command[14], true)) return PlgUdsReject;
            normal = 14;
            break;
        case 29: case 30:
        {
            uint32_t parameters = id == 29 ? 1 : 2;
            uint32_t offset = parameters + 1;
            if (header != PlgUdsHeader(id, parameters, 4) || command[parameters] > 0x100
                || !PlgUdsStatic(validation, command[offset], command[offset + 1], 1, 0x108, false)
                || !PlgUdsStatic(validation, command[offset + 2], command[offset + 3], 0, command[parameters], false)) return PlgUdsReject;
            normal = parameters + 4;
            break;
        }
        default:
            return PlgUdsReject;
    }
    command[0] = PlgUdsHeader(id, normal, 0);
    return PlgUdsForward;
}
