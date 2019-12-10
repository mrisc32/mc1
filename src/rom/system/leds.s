; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: leds
; ----------------------------------------------------------------------------

.include "system/mmio.inc"

    .text

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
    stw     s4, s3, #0
    add     s3, s3, #4
    add     s5, s5, #-1
    bnz     s5, 1$

    j       lr


; ----------------------------------------------------------------------------
; void print_dec(int number)
; Print a decimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  print_dec

print_dec:
    ldea    s2, pc, #hex_to_segment_lut@pc
    ldhi    s3, #MMIO_START
    ldea    s3, s3, #SEGDISP0

    ; Determine the sign of the number.
    slt     s7, s1, z
    bns     s7, 4$
    sub     s1, z, s1
    ldi     s7, #0b1000000  ; s7 = "-" if s1 is negative
4$:

    ldi     s6, #10
    ldi     s5, #8
1$:
    remu    s4, s1, s6      ; s4 = 0..9
    divu    s1, s1, s6
    ldub    s4, s2, s4
2$:
    stw     s4, s3, #0
    add     s3, s3, #4
    add     s5, s5, #-1
    bz      s5, 3$

    bnz     s1, 1$          ; Print more digits as long as the remainder != 0

    mov     s4, s7          ; Leftmost character is the sign (" " or "-").
    ldi     s7, #0          ; Blank the upper digits.
    j       pc, #2$@pc

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

