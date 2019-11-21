; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This is the main boot program.
; ----------------------------------------------------------------------------

.include "config.inc"

VCP_START = RAM_START
VCP_SIZE  = 1024 * 4
FB_START  = VCP_START + VCP_SIZE
FB_WIDTH  = 320
FB_HEIGHT = 180


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

    ; Draw something to the screen.
    jl      pc, #draw@pc

    ; Do some I/O.
    jl      pc, #do_io@pc

    ldw     lr, sp, #0
    add     sp, sp, #4
    j       lr


; ----------------------------------------------------------------------------
; void msleep(int milliseconds)
; ----------------------------------------------------------------------------

msleep:
    ble     s1, 3$

1$:
    ; This busy loop takes 1 ms on a 70 MHz MRISC32-A1.
    ldi     s2, #35000
2$:
    add     s2, s2, #-1
    bnz     s2, 2$

    add     s1, s1, #-1
    bnz     s1, 1$

3$:
    j       lr


; ----------------------------------------------------------------------------
; Do some memory mapped I/O.
; ----------------------------------------------------------------------------

do_io:
    add     sp, sp, #-16
    stw     lr, sp, #0
    stw     s20, sp, #4
    stw     s21, sp, #8
    stw     s22, sp, #12

    ldhi    s20, #MMIO_START
    ldi     s21, #0x00055555
    ldi     s22, #1000
1$:
    stw     s21, s20, #0

    ; Sleep for 1/3 s.
    ldi     s1, #333
    jl      pc, #msleep@pc

    add     s22, s22, #-1
    add     s21, s21, #0x1234
    bnz     s22, 1$

    ldw     lr, sp, #0
    ldw     s20, sp, #4
    ldw     s21, sp, #8
    ldw     s22, sp, #12
    add     sp, sp, #16
    j       lr


; ----------------------------------------------------------------------------
; Init video.
; ----------------------------------------------------------------------------

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

    ; Generate a color palette.
    ldhi    s15, #0x01020301@hi
    or      s15, s15, #0x01020301@lo
    ldi     s14, #255
1$:
    shuf    s12, s14, #0b0000000000000
    mul.b   s12, s12, s15
    stw     s12, s11, s14*4
    add     s14, s14, #-1
    bge     s14, 1$
    add     s11, s11, #256*4

    ; s1 = WAIT for line (start with line 0)
    ldhi    s1, #0x40000000@hi

    ; s2 = SETREG ADDR, (FB_START - RAM_START)/4
    ldhi    s2, #(0x80000000 + ((FB_START - RAM_START)/4))@hi
    add     s2, s2, #(0x80000000 + ((FB_START - RAM_START)/4))@lo

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
    add     s1, s1, #720/FB_HEIGHT  ; Increment WAIT line (vertical resolution)
    add     s2, s2, #FB_WIDTH/4     ; Increment row address (horizontal stride)
    slt     s13, s1, #720
    bs      s13, 2$

    ; VCP epilogue.
    ldhi    s12, #0x40000000@hi     ; WAIT 0 (will never happen)
    stw     s12, s11, #0

    ; Clear the frame buffer.
    ldhi    s9, #FB_START@hi
    add     s9, s9, #FB_START@lo                ; s9 = start of frame buffer
    cpuid   s10, z, z                           ; s10 = max vector length
    ldi     s11, #(FB_WIDTH * FB_HEIGHT) / 4    ; s11 = number of words
4$:
    min     vl, s10, s11
    sub     s11, s11, vl
    stw     vz, s9, #4              ; Store zeroes in the frame buffer
    ldea    s9, s9, vl * 4
    bnz     s11, 4$

    ldw     vl, sp, #0
    add     sp, sp, #4
    j       lr


; ----------------------------------------------------------------------------
; Draw something to the frame buffer.
; ----------------------------------------------------------------------------

draw:
    add     sp, sp, #-4
    stw     vl, sp, #0

    ldhi    s9, #FB_START@hi
    add     s9, s9, #FB_START@lo        ; s9 = start of frame buffer
    ldi     s10, #FB_HEIGHT
1$:
    ldi     s11, #FB_WIDTH
2$:
    add     s1, s10, s11                ; Calculate a color for this pixel
    stb     s1, s9, #0
    add     s9, s9, #1

    add     s11, s11, #-1
    bnz     s11, 2$

    add     s10, s10, #-1
    bnz     s10, 1$

    ldw     vl, sp, #0
    add     sp, sp, #4
    j       lr
