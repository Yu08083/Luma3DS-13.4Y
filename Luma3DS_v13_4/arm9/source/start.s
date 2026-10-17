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
_start:

    msr cpsr_cxsf, #0xD3



    cmp r0, #0
    cmpeq r1, #0
    cmpeq r2, #0
    ldreq r4, =0x20000035
    mrceq p15, 0, r5, c6, c0, 0
    cmpeq r4, r5
    mrceq p15, 0, r5, c6, c5, 0
    ldreq r4, =0x07FF801D
    cmpeq r4, r5
    mrceq p15, 0, r5, c6, c7, 0
    ldreq r4, =0x1FFFE019
    cmpeq r4, r5
    ldreq r2, =0xB002

    mov r9, r0
    mov r10, r1
    mov r11, r2


    mov sp, #0x08100000


    mrc p15, 0, r4, c1, c0, 0
    bic r4, #(1<<16)
    bic r4, #(1<<12)
    bic r4, #(1<<2)
    bic r4, #(1<<0)
    mcr p15, 0, r4, c1, c0, 0



    mov r4, #0
    mcr p15, 0, r4, c7, c5, 0
    mcr p15, 0, r4, c7, c6, 0
    mcr p15, 0, r4, c7, c10, 4


    ldr r0, =0x33333333
    mcr p15, 0, r0, c5, c0, 2
    mcr p15, 0, r0, c5, c0, 3


    ldr r0, =0xFFFF001D
    ldr r1, =0xFFF0001B
    ldr r2, =0x01FF801D
    ldr r3, =0x08000027
    ldr r4, =0x10000029
    ldr r5, =0x20000035
    ldr r6, =0x1FF00027
    ldr r7, =0x1800002D
    mov r8, #0x29
    mcr p15, 0, r0, c6, c0, 0
    mcr p15, 0, r1, c6, c1, 0
    mcr p15, 0, r2, c6, c2, 0
    mcr p15, 0, r3, c6, c3, 0
    mcr p15, 0, r4, c6, c4, 0
    mcr p15, 0, r5, c6, c5, 0
    mcr p15, 0, r6, c6, c6, 0
    mcr p15, 0, r7, c6, c7, 0
    mcr p15, 0, r8, c3, c0, 0
    mcr p15, 0, r8, c2, c0, 0
    mcr p15, 0, r8, c2, c0, 1


    ldr r1, =0xFFF0000A
    mcr p15, 0, r1, c9, c1, 0


    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #(1<<18)
    orr r0, r0, #(1<<16)
    orr r0, r0, #(1<<13)
    orr r0, r0, #(1<<12)
    orr r0, r0, #(1<<2)
    orr r0, r0, #(1<<0)
    mcr p15, 0, r0, c1, c0, 0


    ldr r0, =__bss_start__
    mov r1, #0
    ldr r2, =__bss_end__
    sub r2, r0
    bl memset

    bl __libc_init_array

    mov r0, r9
    mov r1, r10
    mov r2, r11
    b main
.pool

.section .chainloader.text.start, "ax", %progbits
.align 4
.global chainload
.type   chainload, %function
chainload:
    ldr sp, =__itcm_stack_top__
    b chainloader_main
.pool

.global disableMpuAndJumpToEntrypoints
.type   disableMpuAndJumpToEntrypoints, %function
disableMpuAndJumpToEntrypoints:
    mov r4, r0
    mov r5, r1
    mov r6, r2
    mov r7, r3


    ldr r12, =0xFFFF0830
    blx r12
    ldr r12, =0xFFFF0AB4
    blx r12


    mrc p15, 0, r0, c1, c0, 0
    bic r0, #(1<<12)
    bic r0, #(1<<2)
    bic r0, #(1<<0)
    mcr p15, 0, r0, c1, c0, 0


    mov r0, #0x20000000
    str r7, [r0, #-4]


    mov r0, r4
    mov r1, r5
    ldr r2, =0x4BEEF
    bx r6

.pool
