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

@ Hooks mainly written by Stary (PoC originally by Shiny Quagsire)

.section .rodata.input_redirection_hook, "a", %progbits
.balign 4
.arm


.global hidCodePatchFunc
.type   hidCodePatchFunc, %function
hidCodePatchFunc:
push {r4-r6, lr}

push {r3}
mov r5, r1
mrc p15, 0, r4, c13, c0, 3
mov r1, #0x10000
str r1, [r4,#0x80]!
ldr r0, [r0]
svc 0x32
mov r12, r0
pop {r3}

cmp r12, #0
bne skip_touch_cp_cpy

ldrd r0, [r4,#8]
strd r0, [r3, #12]
ldrd r0, [r3, #4]
strd r0, [r5]

skip_touch_cp_cpy:





mov r0, r3
ldr r1, =0x1ec46000
mov r3, #0xf00
orr r3, #0xff


ldr r1, [r1]
ldr r2, [r0, #20]
cmp r2, r3
movne r1, r2
str r1, [r0]


cmp r12, #0

mov r3, #0x2000000
ldreq r1, [r0, #12]
movne r1, r3
ldr r2, [r0, #24]
cmp r2, r3
movne r1, r2
str r1, [r0, #4]


cmp r12, #0

ldr r3, =0x7ff7ff
ldreq r1, [r0, #16]
movne r1, r3
ldr r2, [r0, #28]
cmp r2, r3
movne r1, r2
str r1, [r0, #8]

ldr r0, [r4,#4]

pop {r4-r6, pc}

.pool


.global irCodePatchFunc
.type   irCodePatchFunc, %function
irCodePatchFunc:
add lr, lr, #4
b skip_vars

i2c_readdeviceraw_addr:
    .word 0
redirected_input_addr:
    .word 0

skip_vars:
stmfd sp!, {r4-r5, lr}
ir_check_i2c:



mov r4, r1
mov r2, #17
mov r3, #5

adr r12, i2c_readdeviceraw_addr
ldr r12, [r12]
blx r12

adr r5, redirected_input_addr
ldr r5, [r5]

ldr r1, =0x80800081
cmp r0, #0

ldreq r2, [r4]
tsteq r2, #0x100
movne r2, r1

cmp r1, r2
ldreq r0, [r5]
streq r0, [r4]


mov r0, #0
ldmfd sp!, {r4-r5, pc}
.pool
