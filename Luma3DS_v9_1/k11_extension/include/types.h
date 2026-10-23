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



#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define U64_MAX	UINT64_MAX


#ifndef SSIZE_MAX
#ifdef SIZE_MAX
#define SSIZE_MAX ((SIZE_MAX) >> 1)
#endif
#endif

typedef uint8_t u8;   
typedef uint16_t u16; 
typedef uint32_t u32; 
typedef uint64_t u64; 

typedef int8_t s8;   
typedef int16_t s16; 
typedef int32_t s32; 
typedef int64_t s64; 

typedef volatile u8 vu8;   
typedef volatile u16 vu16; 
typedef volatile u32 vu32; 
typedef volatile u64 vu64; 

typedef volatile s8 vs8;   
typedef volatile s16 vs16; 
typedef volatile s32 vs32; 
typedef volatile s64 vs64; 

typedef u32 Handle;                 
typedef s32 Result;                 


#define BIT(n) (1U<<(n))


#define ALIGN(m)   __attribute__((aligned(m)))

#define PACKED      __attribute__((packed))
#define USED        __attribute__((used))
#define UNUSED      __attribute__((unused))

#define SYSTEM_VERSION(major, minor, revision) \
    (((major)<<24)|((minor)<<16)|((revision)<<8))

#define CUR_THREAD_HANDLE       0xFFFF8000
#define CUR_PROCESS_HANDLE      0xFFFF8001

#define MPCORE_REGS_BASE        (0x17E00000 | (1u << 31))
#define MPCORE_SCU_CFG          (*(vu32 *)(MPCORE_REGS_BASE + 4))
#define MPCORE_INT_ACK          (*(vu32 *)(MPCORE_REGS_BASE + 0x10C))

#define MPCORE_GID_REGS_BASE    (MPCORE_REGS_BASE + 0x1000)
#define MPCORE_GID_SGI          (*(vu32 *)(MPCORE_GID_REGS_BASE + 0xF00))

#define CFG11_REGS_BASE           (0x10140000 | (1u << 31))
#define CFG11_WIFICNT             (*(vu8  *)(CFG11_REGS_BASE + 0x180))
#define CFG11_MPCORE_CFG          (*(vu16 *)(CFG11_REGS_BASE + 0xFFC))
#define CFG11_MPCORE_CLKCNT       (*(vu16 *)(CFG11_REGS_BASE + 0x1300))

#define L2C_REGS_BASE           (0x17E10000 | (1u << 31))
#define L2C_CTRL                (*(vu32 *)(L2C_REGS_BASE + 0x100))
