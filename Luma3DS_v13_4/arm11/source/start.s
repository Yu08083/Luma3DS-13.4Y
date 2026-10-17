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

.section .text.start, "ax", %progbits
.align 4
.global _start
.type   _start, %function
_start:
b start

.global operation
operation:
    .word 0

start:

    cpsid aif, #0x13


    ldr r0, =0x54078
    mcr p15, 0, r0, c1, c0, 0




    mov r0, #0xF
    mcr p15, 0, r0, c1, c0, 1


    mov r0, #0
    mcr p15, 0, r0, c7, c5, 4
    mcr p15, 0, r0, c7, c7, 0
    mcr p15, 0, r0, c7, c10, 4

    ldr sp, =__stack_top__
    mov fp, #0


    ldr r0, =__bss_start__
    mov r1, #0
    ldr r2, =__bss_end__
    sub r2, r0
    bl memset


    bl __libc_init_array

    b main

.global prepareForFirmlaunch
.type   prepareForFirmlaunch, %function
prepareForFirmlaunch:
    str r0, [r1]
    mov r0, #0x20000000

    _wait_for_core0_entrypoint_loop:
        ldr r1, [r0, #-4]
        cmp r1, #0
        beq _wait_for_core0_entrypoint_loop

    bx r1
prepareForFirmlaunchEnd:

.global prepareForFirmlaunchSize
prepareForFirmlaunchSize: .word prepareForFirmlaunchEnd - prepareForFirmlaunch
