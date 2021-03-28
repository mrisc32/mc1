; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: time
; ----------------------------------------------------------------------------

.include "mc1/mmio.inc"

    .text

; ----------------------------------------------------------------------------
; void msleep(int milliseconds)
; ----------------------------------------------------------------------------

    .globl  msleep
    .p2align 2

msleep:
    ble     s1, 3$

    ldi     s3, #MMIO_START
    ldw     s3, s3, #CPUCLK
    add     s3, s3, #500
    ldi     s4, #1000
    divu    s3, s3, s4          ; s3 = clock cycles / ms

1$:
    ; This busy loop takes 1 ms on an MRISC32-A1 (2 cycle per iteration).
    lsr     s2, s3, #1
2$:
    add     s2, s2, #-1
    bnz     s2, 2$

    add     s1, s1, #-1
    bnz     s1, 1$

3$:
    ret

