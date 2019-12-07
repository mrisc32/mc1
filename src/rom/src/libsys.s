; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library routines.
; ----------------------------------------------------------------------------

.include "config.inc"

    .text

; ----------------------------------------------------------------------------
; void msleep(int milliseconds)
; ----------------------------------------------------------------------------

    .globl  msleep

msleep:
    ble     s1, 3$

    ldhi    s3, #MMIO_START
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
    j       lr


; ----------------------------------------------------------------------------
; void print_hex(unsigned number)
; Print a hexadecimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  print_hex

print_hex:
    ldea    s2, pc, #hex_to_segment_lut@pc
    ldhi    s3, #MMIO_START
    ldea    s3, s3, #SEGDISP0

    ldi     s5, #8
1$:
    and     s4, s1, #0x0f
    lsr     s1, s1, #4
    ldub    s4, s2, s4
2$:
    stw     s4, s3, #0
    add     s3, s3, #4
    add     s5, s5, #-1
    bz      s5, 3$

    bnz     s1, 1$
    ldi     s4, #0          ; Blank the upper digits.
    bz      s1, 2$

3$:
    j       lr


; ----------------------------------------------------------------------------
; void print_dec(unsigned number)
; Print a decimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  print_dec

print_dec:
    ldea    s2, pc, #hex_to_segment_lut@pc
    ldhi    s3, #MMIO_START
    ldea    s3, s3, #SEGDISP0

    ldi     s6, #10
    ldi     s5, #8
1$:
    remu    s4, s1, s6
    divu    s1, s1, s6
    ldub    s4, s2, s4
2$:
    stw     s4, s3, #0
    add     s3, s3, #4
    add     s5, s5, #-1
    bz      s5, 3$

    bnz     s1, 1$
    ldi     s4, #0          ; Blank the upper digits.
    bz      s1, 2$

3$:
    j       lr


; ----------------------------------------------------------------------------
; 7-segment display bit encoding:
;
;              -0-
;             5   1
;             |-6-|
;             4   2
;              -3-
; ----------------------------------------------------------------------------

hex_to_segment_lut:
    .byte   0b0111111   ; 0
    .byte   0b0000110   ; 1
    .byte   0b1011011   ; 2
    .byte   0b1001111   ; 3
    .byte   0b1100110   ; 4
    .byte   0b1101101   ; 5
    .byte   0b1111101   ; 6
    .byte   0b0000111   ; 7
    .byte   0b1111111   ; 8
    .byte   0b1101111   ; 9
    .byte   0b1110111   ; A
    .byte   0b1111100   ; b
    .byte   0b0111001   ; C
    .byte   0b1011110   ; d
    .byte   0b1111001   ; E
    .byte   0b1110001   ; F

