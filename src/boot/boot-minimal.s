; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
;--------------------------------------------------------------------------------------------------
; Copyright (c) 2021 Marcus Geelnard
;
; This software is provided 'as-is', without any express or implied warranty. In no event will the
; authors be held liable for any damages arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose, including commercial
; applications, and to alter it and redistribute it freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not claim that you wrote
;     the original software. If you use this software in a product, an acknowledgment in the
;     product documentation would be appreciated but is not required.
;
;  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
;     being the original software.
;
;  3. This notice may not be removed or altered from any source distribution.
;--------------------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------------------
; This is a minimal MC1 boot routine that displays a pattern on the screen. It can be used for
; testing that the MC1 boot process works properly.
;--------------------------------------------------------------------------------------------------

    .section .text.start, "ax"
    .globl  _boot
    .p2align 2

;--------------------------------------------------------------------------------------------------
; void _boot(const void* rom_base);
;  s1 = rom_base
;--------------------------------------------------------------------------------------------------
_boot:
    ; Store the ROM jump table address in s26.
    mov     s26, s1

    ; s10 = start of VRAM
    ldi     s10, #0x40000000

    ; Configure VCP for layer 1 to be silent.
    ldi     s1, #0x50007fff     ; Wait forever
    stw     s1, s10, #16

    ; Configure VCP for layer 2 to jump to our VCP.
    ldi     s1, #vcp@pc
    sub     s1, s1, s10
    lsr     s1, s1, #2          ; s1 = JMP vcp (in video address space)
    stw     s1, s10, #32

    ; Define the frame buffer (just a horizontal bit pattern).
    ldi     s1, #0x55555555
    stw     s1, s10, #64        ; Frame buffer @ 0x40000040

    ; Loop forever.
1:
    b       1b

    .p2align 2
vcp:
    .word   0x85000005          ; SETREG CMODE, 5 (PAL1)
    .word   0x60000001          ; SETPAL 0, 2
    .word   0xff80ff80          ; Color 0
    .word   0xff600020          ; Color 1
    .word   0x82000422          ; SETREG XINCR, 0x00.0422 (31/1920)
    .word   0x50000000          ; Wait for Y=0
    .word   0x84000780          ; SETREG HSTOP, 1920
    .word   0x80000010          ; SETREG ADDR, 0x000010 (0x40000040)
    .word   0x5000021c          ; Wait for Y=540 (1080/2)
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50007fff          ; Wait forever

