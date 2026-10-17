@   This file is part of Luma3DS
@   Copyright (C) 2016-2020 Aurora Wright, TuxSH
@
@   This program is free software: you can redistribute it and/or modify
@   it under the terms of the GNU General Public License as published by
@   the Free Software Foundation, either version 3 of the License, or
@   (at your option) any later version.
@
@   This program is distributed in the hope that it will be useful,
@   but WITHOUT ANY WARRANTY; without even the implied warranty of
@   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@   GNU General Public License for more details.
@
@   You should have received a copy of the GNU General Public License
@   along with this program.  If not, see <http://www.gnu.org/licenses/>.
@
@   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
@       * Requiring preservation of specified reasonable legal notices or
@         author attributions in that material or in the Appropriate Legal
@         Notices displayed by works containing it.
@       * Prohibiting misrepresentation of the origin of that material,
@         or requiring that modified versions of such material be marked in
@         reasonable ways as different from the original version.

.section .text.cache, "ax", %progbits
.arm
.align 4

.global flushEntireDCache
.type flushEntireDCache, %function
flushEntireDCache:
    @ Adapted from http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0155a/ch03s03s05.html,
    @ and https://github.com/gemarcano/libctr9_io/blob/master/src/ctr_system_ARM.c#L39 as well



    mov r1, #0
    outer_loop:
        mov r0, #0

        inner_loop:
            orr r2, r1, r0
            mcr p15, 0, r2, c7, c14, 2
            add r0, #0x20
            cmp r0, #0x400
            bne inner_loop

        add r1, #0x40000000
        cmp r1, #0
        bne outer_loop

    mcr p15, 0, r1, c7, c10, 4
    bx lr

.global flushDCacheRange
.type flushDCacheRange, %function
flushDCacheRange:

    add r1, r0, r1
    bic r0, #0x1f

    flush_dcache_range_loop:
        mcr p15, 0, r0, c7, c14, 1
        add r0, #0x20
        cmp r0, r1
        blo flush_dcache_range_loop

    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4
    bx lr

.global flushEntireICache
.type flushEntireICache, %function
flushEntireICache:

    mov r0, #0
    mcr p15, 0, r0, c7, c5, 0
    bx lr

.global flushICacheRange
.type flushICacheRange, %function
flushICacheRange:

    add r1, r0, r1
    bic r0, #0x1f

    flush_icache_range_loop:
        mcr p15, 0, r0, c7, c5, 1
        add r0, #0x20
        cmp r0, r1
        blo flush_icache_range_loop

    bx lr
