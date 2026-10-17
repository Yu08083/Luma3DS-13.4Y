.section .emunand_patch, "aw", %progbits
.arm
.align 4

@ Code originally by Normmatt

.global emunandPatch
emunandPatch:

    mov r4, r0
    mov r5, r1
    mov r7, r2
    mov r6, r3



    ldr r2, [r0, #4]
    ldr r1, emunandPatchSdmmcStructPtr
    cmp r2, r1
    beq out

    str r1, [r0, #4]
    ldr r2, [r0, #8]
    cmp r2, #0

    ldr r3, emunandPatchNandOffset
    add r2, r3

    ldreq r3, emunandPatchNcsdHeaderOffset
    addeq r2, r3

    str r2, [r0, #8]

    out:

        mov r1, r5
        mov r2, r7
        mov r3, r6



        mov r0, lr
        add r0, #4
        bx r0

.pool

_emunandPatchEnd:

.global emunandProtoPatch
emunandProtoPatch:

    push {r0-r3}


    ldr r2, [r0, #4]
    ldr r1, emunandPatchSdmmcStructPtr
    cmp r2, r1
    beq _out

    ldrb r2, [r1, #0xc]
    cmp r2, #0
    beq _pastSdmc
    str r1, [r0, #4]
    _pastSdmc:
    ldr r2, [r0, #8]
    cmp r2, #0

    ldr r3, emunandPatchNandOffset
    add r2, r3

    ldreq r3, emunandPatchNcsdHeaderOffset
    addeq r2, r3

    str r2, [r0, #8]

    _out:

        pop {r0-r3}

        cmp r1, #3
        mov r12, r2
        add r0, r0, #4
        movne r1, #0
        moveq r1, #1


        mov r2, lr
        pop {lr}



        add r2, #8
        bx r2

.global emunandProtoCidPatch
emunandProtoCidPatch:

    ldr r4, emunandPatchSdmmcStructPtr
    cmp r0, r4
    beq _cid_return


    adr r4, emunandPatchNandCid
    ldr r2, [r4, #0]
    ldr r3, [r4, #4]
    ldr r5, [r4, #8]
    ldr r6, [r4, #0xc]
    str r2, [r1, #0]
    str r3, [r1, #4]
    str r5, [r1, #8]
    str r6, [r1, #0xc]

    mov r0, #0
    pop {r4-r6, pc}

    _cid_return:

        mov r4, r0
        ldr r0, [r0]
        mov r5, r1


        add lr, #4
        bx lr

.global emunandProtoCidPatch1200
emunandProtoCidPatch1200:

    ldr r4, emunandPatchSdmmcStructPtr
    cmp r0, r4
    beq _cid_return1200


    adr r4, emunandPatchNandCid
    ldr r2, [r4, #0]
    ldr r3, [r4, #4]
    ldr r5, [r4, #8]
    ldr r6, [r4, #0xc]
    str r2, [r1, #0]
    str r3, [r1, #4]
    str r5, [r1, #8]
    str r6, [r1, #0xc]

    mov r0, #0
    pop {r4-r6, pc}

    _cid_return1200:

        mov r4, r0
        ldr r0, [r0]
        mov r6, r1


        add lr, #4
        bx lr

.global emunandProtoPatch238
emunandProtoPatch238:

    push {r0-r3}


    ldr r2, [r4, #4]
    ldr r1, emunandPatchSdmmcStructPtr
    cmp r2, r1
    beq _out238

    ldr r2, [r1, #0x24]
    cmp r2, #0
    beq _pastSdmc238
    str r1, [r4, #4]
    _pastSdmc238:

    ldr r2, [r4, #8]
    cmp r2, #0

    ldr r3, emunandPatchNandOffset
    add r2, r3

    ldreq r3, emunandPatchNcsdHeaderOffset
    addeq r2, r3

    str r2, [r4, #8]

    _out238:

        pop {r0-r3}

        cmp r0, #3
        movne r0, #0
        moveq r0, #1


        mov r1, lr


        add r1, #4
        bx r1

.global emunandProtoPatch1200
emunandProtoPatch1200:
    push {r0-r3}


    ldr r2, [r4]
    ldr r1, emunandPatchSdmmcStructPtr
    cmp r1, r2
    beq _out1200

    ldr r2, [r1, #0x24]
    cmp r2, #0
    beq _pastSdmc1200
    str r1, [r4]
    _pastSdmc1200:

    ldr r2, [r4, #4]
    cmp r2, #0

    ldr r3, emunandPatchNandOffset
    add r2, r3

    ldreq r3, emunandPatchNcsdHeaderOffset
    addeq r2, r3

    str r2, [r4, #4]

    _out1200:

    pop {r0-r3}
    cmp r0, #3
    sub sp, sp, #0x14
    mov r7, #0



    mov r1, lr


    add r1, #4
    bx r1

.pool

.global emunandPatchSdmmcStructPtr
.global emunandPatchNandOffset
.global emunandPatchNcsdHeaderOffset
.global emunandPatchNandCid

_emunandPatchBssStart:
emunandPatchSdmmcStructPtr:     .word   0
emunandPatchNandOffset:         .word   0
emunandPatchNcsdHeaderOffset:   .word   0
emunandPatchNandCid:
    .word 0,0,0,0
_emunandPatchBssEnd:

.pool
.balign 4

.global emunandPatchSize
emunandPatchSize:
    .word _emunandPatchEnd - emunandPatch

.global emunandPatchBssSize
emunandPatchBssSize:
    .word _emunandPatchBssEnd - _emunandPatchBssStart