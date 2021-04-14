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
; This is a MC1 boot demo that loads and displays pictures from the boot image.
;--------------------------------------------------------------------------------------------------

    ; ROM routine table offsets.
    DOH = 0
    BLK_READ = 4
    CRC32C = 8
    LZG_DECODE = 12

    ; Picture dimensions.
    PIC_WIDTH = 1200
    PIC_HEIGHT = 675
    PIC_BPP = 2

    ; Picture VRAM addresses.
    PIC_ADDR = 0x40003000
    PIC_STRIDE = (PIC_WIDTH*PIC_BPP)/8

    ; ...in video address space.
    PIC_VADDR = (PIC_ADDR-0x40000000)/4
    PIC_VSTRIDE = PIC_STRIDE/4

    ; ...in the boot image.
    PIC_BLOCK = 1
    PIC_NUM_BLOCKS = 396

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

    ldi     s9, #0x50007fff     ; Wait forever
    stw     s9, s10, #16

    ; Generate VCP prologue layer 1.
    addpchi s2, #vcp_preamble@pchi
    add     s2, s2, #vcp_preamble+4@pclo
    add     s3, s10, #32
    ldi     s4, #0
1:
    ldw     s1, s2, s4*4
    stw     s1, s3, s4*4
    add     s4, s4, #1
    slt     s15, s4, #vcp_preamble_len
    bs      s15, 1b
    ldea    s3, s3, s4*4

    ; Generate line addresses.
    ldi     s2, #0x80000000+PIC_VADDR   ; SETREG ADDR, ...
    ldi     s5, #0x50000000             ; WAITY ...
    ldi     s6, #1080
    ldi     s4, #1
2:
    mul     s8, s4, s6
    ldi     s7, #PIC_HEIGHT
    div     s8, s8, s7          ; s8 = line to wait for
    add     s8, s5, s8
    stw     s2, s3, #0          ; SETREG ADDR, ...
    stw     s8, s3, #4          ; WAITY ...
    add     s2, s2, #PIC_VSTRIDE
    add     s3, s3, #8
    add     s4, s4, #1
    sle     s15, s4, #PIC_HEIGHT
    bs      s15, 2b

    ; VCP epilogue.
    stw     s9, s3, #0          ; Wait forever

    ; Clear the frame buffer.
    ldi     s1, #PIC_ADDR
    ldi     s2, #PIC_ADDR+PIC_HEIGHT*PIC_STRIDE
5:
    stw     z, s1, #0
    add     s1, s1, #4
    slt     s3, s1, s2
    bs      s3, 5b

    ldi     s25, #0
4:
    ; Load the picture into the frame buffer.
    ldi     s1, #PIC_ADDR
    ldi     s2, #0
    ldi     s3, #PIC_BLOCK
    ldi     s4, #PIC_NUM_BLOCKS
    mul     s5, s4, s25
    add     s3, s3, s5
    jl      s26, #BLK_READ

    ; Success?
    bnz     s1, 3f

    ; ...otherwise show a red background.
    ldi     s1, #0xff0000ff
    ldi     s2, #0x40000000
    stw     s1, s2, #16+2*4

    ; ...repeat.
3:
    add     s25, s25, #1
    and     s25, s25, #1
    b       4b

    .p2align 2
vcp_preamble:
    .word   0x85000004          ; SETREG CMODE, 4 (PAL2)
    .word   0x60000003          ; SETPAL 0, 4
    .word   0xff000000          ; Color 0
    .word   0xff555555          ; Color 1
    .word   0xffaaaaaa          ; Color 2
    .word   0xffffffff          ; Color 3
    .word   0x82000000+((65536*PIC_WIDTH)/1920)  ; SETREG XINCR, ...
    .word   0x50000000          ; Wait for Y=0
    .word   0x84000780          ; SETREG HSTOP, 1920
vcp_preamble_len = (.-vcp_preamble)/4

