; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the common startup code. It defines _start, which does
; some initialization and then calls main.
; ----------------------------------------------------------------------------

.include "config.inc"

    .text

; ----------------------------------------------------------------------------
; Main program entry.
; ----------------------------------------------------------------------------

    .globl  main

main:
    add     sp, sp, #-4
    stw     lr, sp, #0

    ; Init video.
    jl      pc, #init_video@pc

    ; TODO(m): Implement me!

    ldw     lr, sp, #0
    add     sp, sp, #4
    j       lr


; ----------------------------------------------------------------------------
; Init video.
; ----------------------------------------------------------------------------

VCP_START = RAM_START
VCP_SIZE  = 1024 * 4
FB_START  = VCP_START + VCP_SIZE

init_video:
    add     sp, sp, #-4
    stw     vl, sp, #0

    ldhi    s11, #VCP_START@hi
    add     s11, s11, #VCP_START@lo   ; s11 = start of Video Control Program

    ; VCP prologue.
    ldhi    s12, #0x81000000@hi     ; SETREG XOFFS, 0x00.0000
    stw     s12, s11, #0
    ldhi    s12, #0x82004000@hi     ; SETREG XINCR, 0x00.4000
    stw     s12, s11, #4
    ldhi    s12, #0x83000000@hi     ; SETREG HSTRT, 0
    stw     s12, s11, #8
    ldhi    s12, #0x84000000@hi     ; SETREG HSTOP, 0
    stw     s12, s11, #12
    ldhi    s12, #0x85000002@hi     ; SETREG CMODE, 2
    add     s12, s12, #0x85000002@lo
    stw     s12, s11, #16
    ldhi    s12, #0xc00000ff@hi     ; SETPAL 0, 255
    add     s12, s12, #0xc00000ff@lo
    stw     s12, s11, #20
    add     s11, s11, #24

    ; Generate a gray scale palette.
    ldi     s14, #255
1$:
    shuf    s12, s14, #0b0000000000000
    stw     s12, s11, s14*4
    add     s14, s14, #-1
    bge     s14, 1$
    add     s11, s11, #256*4

    ; s1 = WAIT for line (start with line 0)
    ldhi    s1, #0x40000000@hi

    ; s2 = SETREG ADDR, FB_START/4
    ldhi    s2, #(0x80000000 + (FB_START/4))@hi
    add     s2, s2, #(0x80000000 + (FB_START/4))@lo

    ; First line.
    stw     s1, s11, #0                 ; WAIT   ...
    stw     s2, s11, #4                 ; SETREG ADDR, ...
    ldhi    s12, #(0x84000000+1280)@hi  ; SETREG HSTOP, 1280
    add     s12, s12, #(0x84000000+1280)@lo
    stw     s12, s11, #8
    add     s11, s11, #12
    j       pc, #3$@pc

    ; Consecutive lines.
2$:
    stw     s1, s11, #0             ; WAIT   ...
    stw     s2, s11, #4             ; SETREG ADDR, ...
    add     s11, s11, #8
3$:
    add     s1, s1, #4              ; Increment WAIT line (vertical resolution)
    add     s2, s2, #320            ; Increment row address (horizontal stride)
    slt     s13, s1, #720
    bs      s13, 2$

    ; VCP epilogue.
    ldhi    s12, #0x40000000@hi     ; WAIT 0 (will never happen)
    stw     s12, s11, #0

    ; Clear the frame buffer.
    ldhi    s9, #FB_START@hi
    add     s9, s9, #FB_START@lo    ; s9 = start of frame buffer
    cpuid   s10, z, z               ; s10 = max vector length
    ldi     s11, #320 * 180 / 4     ; s11 = number of words
4$:
    min     vl, s10, s11
    sub     s11, s11, vl
    stw     vz, s9, #4              ; Store zeroes to the frame buffer
    ldea    s9, s9, vl * 4
    bnz     s11, 4$

    ldw     vl, sp, #0
    add     sp, sp, #4
    j       lr
