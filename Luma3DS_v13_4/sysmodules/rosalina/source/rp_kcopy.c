/*
*   This file is part of Luma3DS (Level256 fork)
*   Copyright (C) 2026 Level256
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
*/

#include <3ds.h>
#include <stdint.h>
#include "csvc.h"
#include "rp_kcopy.h"

#define ROWS_PER_CALL 40
#define HASH_PRIME 0x01000193u

typedef struct CopyArgs
{
    u32 *dst;
    const u32 *src;
    u32 rows, srcPitchWords, srcStepRows, rowWords, hash;
} CopyArgs;

static void copyRows(CopyArgs *args)
{
    u32 *dst = args->dst;
    const u32 *src = args->src;
    u32 h = args->hash;
    for (u32 y = 0; y < args->rows; y++)
    {
        for (u32 i = 0; i < args->rowWords; i++)
        {
            u32 value = src[i];
            dst[i] = value;
            h = (h ^ value) * HASH_PRIME;
        }
        dst += args->rowWords;
        if (y + 1 < args->rows)
            src += args->srcPitchWords * args->srcStepRows;
    }
    args->hash = h;
}

static bool dimensions(void *dst, u32 dstCap, uintptr_t src, u32 rows, u32 stride,
                       u32 rowStep, u32 rowBytes, u32 *hash, u64 *span)
{
    u64 output = (u64)rows * rowBytes;
    if (!dst || !src || !hash || !rows || rows > 400 || !rowStep || rowStep > 2
        || !rowBytes || rowBytes > 960 || stride < rowBytes || stride > 4096
        || (((uintptr_t)dst | src | stride | rowBytes) & 3) || output > dstCap || output > 384000)
        return false;
    *span = (u64)(rows - 1) * rowStep * stride + rowBytes;
    return (uintptr_t)dst <= UINTPTR_MAX - output && src <= UINTPTR_MAX - *span;
}

bool RpCopy_Rows(void *dst, u32 dstCap, const void *src, u32 srcCap, u32 rows,
                 u32 stride, u32 rowStep, u32 rowBytes, u32 *hash)
{
    u64 span;
    if (!dimensions(dst, dstCap, (uintptr_t)src, rows, stride, rowStep, rowBytes, hash, &span) || span > srcCap)
        return false;
    CopyArgs args = { (u32 *)dst, (const u32 *)src, rows, stride / 4, rowStep, rowBytes / 4, *hash };
    copyRows(&args);
    *hash = args.hash;
    return true;
}

bool RpKernelCopy_Rows(void *dst, u32 dstCap, u32 kernelVa, u32 rows,
                       u32 stride, u32 rowStep, u32 rowBytes, u32 *hash)
{
    u64 span;
    if (!dimensions(dst, dstCap, kernelVa, rows, stride, rowStep, rowBytes, hash, &span))
        return false;
    bool oldMapping = GET_VERSION_MINOR(osGetKernelVersion()) < 44;
    u64 low = oldMapping ? 0xF0000000ULL : 0xE0000000ULL;
    u64 high = oldMapping ? 0x100000000ULL : 0xF0000000ULL;
    if (kernelVa < low || (u64)kernelVa + span > high)
        return false;
    u32 h = *hash;
    for (u32 y = 0; y < rows; y += ROWS_PER_CALL)
    {
        CopyArgs args = {
            (u32 *)dst + y * (rowBytes / 4),
            (const u32 *)(kernelVa + y * rowStep * stride),
            rows - y < ROWS_PER_CALL ? rows - y : ROWS_PER_CALL,
            stride / 4, rowStep, rowBytes / 4, h
        };
        svcCustomBackdoor(copyRows, &args);
        h = args.hash;
    }
    *hash = h;
    return true;
}
