

.section .data.romfsRedirPatch, "aw", %progbits
.align 4

.global romfsRedirPatch
romfsRedirPatch:

    _mountArchive:
        b       mountArchive
    .global romfsRedirPatchSubstituted1
    romfsRedirPatchSubstituted1:
        .word   0xdead0000
    .global romfsRedirPatchHook1
    romfsRedirPatchHook1:
        .word   0xdead0001


    _fsRedir:
        b       fsRedir
    .global romfsRedirPatchSubstituted2
    romfsRedirPatchSubstituted2:
        .word   0xdead0002
    .global romfsRedirPatchHook2
    romfsRedirPatchHook2:
        .word   0xdead0003


    mountArchive:
        cmp     r3, #3
        bne     romfsRedirPatchSubstituted1
        stmfd   sp!, {r0-r4, lr}
        adr     r0, romfsRedirPatchArchiveName
        .global romfsRedirPatchFsUnMountArchive
        romfsRedirPatchFsUnMountArchive:
            .word 0xdead0004
        sub     sp, sp, #4
        ldr     r1, romfsRedirPatchArchiveId
        mov     r0, sp
        .global romfsRedirPatchFsMountArchive
        romfsRedirPatchFsMountArchive:
            .word 0xdead0005
        mov     r3, #0
        mov     r2, #0
        ldr     r1, [sp]
        adr     r0, romfsRedirPatchArchiveName
        .global romfsRedirPatchFsRegisterArchive
        romfsRedirPatchFsRegisterArchive:
            .word 0xdead0006
        add     sp, sp, #4
        ldmfd   sp!, {r0-r4, lr}
        b       romfsRedirPatchSubstituted1






    fsRedir:
        stmfd   sp!, {r0-r12, lr}
        adr     r3, romfsRedirPatchRomFsMount
        bl      compare
        adrne   r3, romfsRedirPatchUpdateRomFsMount
        blne    compare
        bne     endRedir
        sub     sp, sp, #0x400
        pathRedir:
            stmfd   sp!, {r0-r3}
            add     r0, sp, #0x10
            ldr     r3, romfsRedirPatchCustomPath
            pathRedir_1:
                ldrb    r2, [r3], #1
                cmp     r2, #0
                strneh  r2, [r0], #2
                bne     pathRedir_1
            pathRedir_2:
                ldrh    r2, [r1], #2
                cmp     r2, #0x3A
                bne     pathRedir_2


            ldrh    r3, [r1, #2]
            cmp     r3, #0x2F
            pathRedir_3:
                ldrh    r2, [r1], #2
                strneh  r2, [r0], #2
                cmp     r2, #0
                bne     pathRedir_3
            ldmfd   sp!, {r0-r3}
        mov     r1, sp
        bl      romfsRedirPatchSubstituted2
        add     sp, sp, #0x400
        cmp     r0, #0

    endRedir:
        ldmfd   sp!, {r0-r12, lr}
        moveq   r0, #0
        bxeq    lr
        b       romfsRedirPatchSubstituted2

    compare:
        mov     r9, r1
        add     r10, r3, #4
        loop:
            ldrb    r12, [r3], #1
            ldrb    r11, [r9], #2
            cmp     r11, r12
            bxne    lr
            cmp     r10, r3
            bne     loop
        bx lr

.pool
.balign 4

    .global romfsRedirPatchArchiveName
    .global romfsRedirPatchArchiveId
    .global romfsRedirPatchRomFsMount
    .global romfsRedirPatchUpdateRomFsMount
    .global romfsRedirPatchCustomPath

    romfsRedirPatchArchiveName       : .ascii "lf:\0"
    romfsRedirPatchArchiveId         : .word 0xdead0007
    romfsRedirPatchRomFsMount        : .ascii "rom:"
    romfsRedirPatchUpdateRomFsMount  : .word 0xdead0008
    romfsRedirPatchCustomPath        : .word 0xdead0009

_romfsRedirPatchEnd:

.global romfsRedirPatchSize
romfsRedirPatchSize:
    .word _romfsRedirPatchEnd - romfsRedirPatch
