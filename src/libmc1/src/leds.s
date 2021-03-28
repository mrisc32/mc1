; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: leds
; ----------------------------------------------------------------------------

.include "mc1/mmio.inc"

    .text

; ----------------------------------------------------------------------------
; void set_leds(unsigned bits)
; Set the leds of the boards.
; ----------------------------------------------------------------------------

    .globl set_leds
    .p2align 2

set_leds:
    ldhi    s2, #MMIO_START
    stw     s1, s2, #LEDS
    ret


; ----------------------------------------------------------------------------
; void sevseg_print_hex(unsigned number)
; Print a hexadecimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  sevseg_print_hex
    .p2align 2

sevseg_print_hex:
    ldi     s2, #glyph_lut@pc
    ldi     s3, #MMIO_START+SEGDISP0

    ldi     s5, #8
1$:
    and     s4, s1, #0x0f
    lsr     s1, s1, #4
    ldub    s4, s2, s4
    stw     s4, s3, #0
    add     s3, s3, #4
    add     s5, s5, #-1
    bnz     s5, 1$

    ret


; ----------------------------------------------------------------------------
; void sevseg_print_dec(int number)
; Print a decimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  sevseg_print_dec
    .p2align 2

sevseg_print_dec:
    ldi     s2, #glyph_lut@pc
    ldi     s3, #MMIO_START+SEGDISP0

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
    b       2$

3$:
    ret


; ----------------------------------------------------------------------------
; void sevseg_print(const char* text)
; Print a decimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  sevseg_print
    .p2align 2

sevseg_print:
    ldi     s2, #glyph_lut@pc
    ldi     s3, #MMIO_START+SEGDISP0
    ldi     s7, #alpha_to_glyph_lut@pc

1$:
    ; Get next char.
    ldub    s4, s1, #0
    add     s1, s1, #1
    ldi     s5, #0
    bz      s4, 3$

    slt     s6, s4, #48
    bs      s6, 2$
    slt     s6, s4, #58
    bns     s6, 6$
    ; It's a numeric glyph.
    add     s4, s4, #-48
    b       7$

6$:
    slt     s6, s4, #65
    bs      s6, 2$
    slt     s6, s4, #91
    bns     s6, 2$
    ; It's an alpha glyph.
    add     s4, s4, #-65
    ldub    s4, s7, s4

7$:
    ; Get glyph.
    ldub    s5, s2, s4

    ; Print glyph.
2$:
    stw     s5, s3, #0
    add     s3, s3, #4      ; TODO(m): We should reverse the order...

    b       1$

3$:
    ret


; ----------------------------------------------------------------------------
; 7-segment display bit encoding:
;
;              -0-
;             5   1
;             |-6-|
;             4   2
;              -3-
; ----------------------------------------------------------------------------

    .section .rodata

glyph_lut:
    .byte   0b0111111   ; 0, O
    .byte   0b0000110   ; 1, I
    .byte   0b1011011   ; 2, Z
    .byte   0b1001111   ; 3
    .byte   0b1100110   ; 4
    .byte   0b1101101   ; 5, S
    .byte   0b1111101   ; 6
    .byte   0b0000111   ; 7
    .byte   0b1111111   ; 8
    .byte   0b1101111   ; 9, g
    .byte   0b1110111   ; A
    .byte   0b1111100   ; b
    .byte   0b0111001   ; C
    .byte   0b1011110   ; d
    .byte   0b1111001   ; E
    .byte   0b1110001   ; F

    .byte   0b1110110   ; H
    .byte   0b0001110   ; J
    .byte   0b0111000   ; L
    .byte   0b1110011   ; P
    .byte   0b0110001   ; T
    .byte   0b0111110   ; U, V
    .byte   0b1110010   ; Y

    .byte   0b0000000   ; Space (and unprintable)


alpha_to_glyph_lut:
          ; A   b   C   d   E   F   g  H   I  J   K   L   M   N   O  P   Q
    .byte   10, 11, 12, 13, 14, 15, 9, 16, 1, 17, 23, 18, 23, 23, 0, 19, 23

          ; R   S  T   U   V   W   X   Y   Z
    .byte   23, 5, 20, 21, 21, 23, 23, 22, 2

