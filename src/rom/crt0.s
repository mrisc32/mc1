; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the common startup code. It defines _start, which does
; some initialization and then calls main.
; ----------------------------------------------------------------------------

.include "mc1/memory.inc"
.include "mc1/mmio.inc"


.macro BOOTSTAGE num:req, sevseg:req
    ldi     r1, #MMIO_START
    ldi     r2, #1<<(\num - 1)
    stw     r2, [r1, #LEDS]
    ldi     r2, #\sevseg
    stw     r2, [r1, #SEGDISP0]
.endm


    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Clear all CPU registers.
    ; ------------------------------------------------------------------------

    BOOTSTAGE   1, 0b0000110

    ; Set all the scalar registers (except Z, SP and VL) to a known state.
    ldi     r1, #0
    ldi     r2, #0
    ldi     r3, #0
    ldi     r4, #0
    ldi     r5, #0
    ldi     r6, #0
    ldi     r7, #0
    ldi     r8, #0
    ldi     r9, #0
    ldi     r10, #0
    ldi     r11, #0
    ldi     r12, #0
    ldi     r13, #0
    ldi     r14, #0
    ldi     r15, #0
    ldi     r16, #0
    ldi     r17, #0
    ldi     r18, #0
    ldi     r19, #0
    ldi     r20, #0
    ldi     r21, #0
    ldi     r22, #0
    ldi     r23, #0
    ldi     r24, #0
    ldi     r25, #0
    ldi     r26, #0
    ldi     tp, #0
    ldi     fp, #0
    ldi     lr, #0

    ; Set all the vector registers to a known state: clear all elements.
    ; Also: The default vector length is the max vector register length.
    getsr   vl, #0x10
    or      v1, vz, #0
    or      v2, vz, #0
    or      v3, vz, #0
    or      v4, vz, #0
    or      v5, vz, #0
    or      v6, vz, #0
    or      v7, vz, #0
    or      v8, vz, #0
    or      v9, vz, #0
    or      v10, vz, #0
    or      v11, vz, #0
    or      v12, vz, #0
    or      v13, vz, #0
    or      v14, vz, #0
    or      v15, vz, #0
    or      v16, vz, #0
    or      v17, vz, #0
    or      v18, vz, #0
    or      v19, vz, #0
    or      v20, vz, #0
    or      v21, vz, #0
    or      v22, vz, #0
    or      v23, vz, #0
    or      v24, vz, #0
    or      v25, vz, #0
    or      v26, vz, #0
    or      v27, vz, #0
    or      v28, vz, #0
    or      v29, vz, #0
    or      v30, vz, #0
    or      v31, vz, #0


    ; ------------------------------------------------------------------------
    ; Set up the stack (0.5 KiB at top of VRAM).
    ; ------------------------------------------------------------------------

    ldi     r1, #MMIO_START
    ldw     r1, [r1, #VRAMSIZE]
    ldi     sp, #VRAM_START
    add     sp, sp, r1                  ; sp = Top of stack (top of VRAM)


    ; ------------------------------------------------------------------------
    ; Clear the BSS data (if any).
    ; ------------------------------------------------------------------------

    BOOTSTAGE   2, 0b1011011

    ldi     r2, #__bss_size
    bz      r2, bss_cleared
    lsr     r2, r2, #2      ; BSS size is always a multiple of 4 bytes.

    ldi     r1, #__bss_start
    getsr   vl, #0x10
clear_bss_loop:
    minu    vl, vl, r2
    sub     r2, r2, vl
    stw     vz, [r1, #4]
    ldea    r1, [r1, vl*4]
    bnz     r2, clear_bss_loop
bss_cleared:


    ; ------------------------------------------------------------------------
    ; Make both video layers "silent" (use no memory cycles).
    ; We also set the background color for both layers, since the content of
    ; the palette registers is undefined after reset.
    ; ------------------------------------------------------------------------

    BOOTSTAGE   3, 0b1001111

    ldi     r1, #0x60000000     ; SETPAL 0, 1
    ldi     r2, #0xff8080a0     ; Color 0 = red tint (ABGR32)
    ldi     r3, #0x50007fff     ; WAITY 32767 = wait forever
    ldi     r4, #VRAM_START
    stw     r1, [r4, #16]       ; Layer 1 VCP
    stw     r2, [r4, #20]
    stw     r3, [r4, #24]
    stw     r1, [r4, #32]       ; Layer 2 VCP
    stw     z, [r4, #36]        ; (fully transparent black for layer 2)
    stw     r3, [r4, #40]


    ; ------------------------------------------------------------------------
    ; Call main().
    ; Note: We don't do _init() / _fini() to reduce ROM size, and thus static
    ; C++ constructors are not supported in the ROM code.
    ; ------------------------------------------------------------------------

    BOOTSTAGE   4, 0b1100110

    ; r1 = argc, r2 = argv (these are invalid - don't use them!)
    ldi     r1, #0
    ldi     r2, #0

    ; Jump to main().
    bl      main


    ; Terminate the program: Loop forever...
    BOOTSTAGE   8, 0b1001001    ; 7-segment (three horizontal bars)
1$:
    b       1$

