; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This is the main boot program.
; ----------------------------------------------------------------------------

.include "config.inc"

VCP_START = RAM_START
VCP_SIZE  = 1024 * 4
FB_START  = VCP_START + VCP_SIZE
FB_WIDTH  = 640
FB_HEIGHT = 360

NATIVE_WIDTH = 1920
NATIVE_HEIGHT = 1080


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

    ; Enter the main loop.
    jl      pc, #main_loop@pc

    ldw     lr, sp, #0
    add     sp, sp, #4
    j       lr


; ----------------------------------------------------------------------------
; Init video.
; ----------------------------------------------------------------------------

init_video:
    add     sp, sp, #-4
    stw     vl, sp, #0

    ldhi    s11, #VCP_START@hi
    or      s11, s11, #VCP_START@lo ; s11 = start of Video Control Program

    ; VCP prologue.
    ldhi    s12, #(0x82000000 + 0x010000 * FB_WIDTH / NATIVE_WIDTH)@hi  ; SETREG XINCR, ...
    or      s12, s12, #(0x82000000 + 0x010000 * FB_WIDTH / NATIVE_WIDTH)@lo
    stw     s12, s11, #0
    ldhi    s12, #0x85000002@hi     ; SETREG CMODE, 2
    or      s12, s12, #0x85000002@lo
    stw     s12, s11, #4
    ldhi    s12, #0x86000001@hi     ; SETREG RMODE, 1
    or      s12, s12, #0x86000001@lo
    stw     s12, s11, #8
    ldhi    s12, #0x600000ff@hi     ; SETPAL 0, 255
    or      s12, s12, #0x600000ff@lo
    stw     s12, s11, #12

    add     s11, s11, #16

    ; Generate a color palette.
    ldi     s1, #100    ; R
    ldi     s2, #50     ; G
    ldi     s3, #0      ; B
    ldi     s4, #2      ; R increment
    ldi     s5, #3      ; G increment
    ldi     s6, #1      ; B increment

    ldhi    s15, #0xff000000
    stw     s15, s11, #0         ; Color 0 is black

    ldi     s14, #1
1$:
    lsl     s7, s3, #16
    lsl     s8, s2, #8
    or      s7, s7, s8
    or      s7, s7, s1
    or      s7, s7, s15        ; s7 = 255 << 24 | s3 << 16 | s2 << 8 | s1
    stw     s7, s11, s14*4

    ; Update R, and if necessary adjust the increment value.
    add     s1, s1, s4
    sle     s7, s1, #255
    sle     s8, z, s1
    and     s7, s7, s8
    bs      s7, 5$
    sub     s4, z, s4
    add     s1, s1, s4
5$:
    ; Update G, and if necessary adjust the increment value.
    add     s2, s2, s5
    sle     s7, s2, #255
    sle     s8, z, s2
    and     s7, s7, s8
    bs      s7, 6$
    sub     s5, z, s5
    add     s2, s2, s5
6$:
    ; Update B, and if necessary adjust the increment value.
    add     s3, s3, s6
    sle     s7, s3, #255
    sle     s8, z, s3
    and     s7, s7, s8
    bs      s7, 7$
    sub     s6, z, s6
    add     s3, s3, s6
7$:

    add     s14, s14, #1
    sne     s7, s14, #256
    bs      s7, 1$

    add     s11, s11, #256*4

    ; s1 = WAITY for line (start with line 0)
    ldhi    s1, #0x50000000@hi

    ; s2 = SETREG ADDR, (FB_START - RAM_START)/4
    ldhi    s2, #(0x80000000 + ((FB_START - RAM_START)/4))@hi
    or      s2, s2, #(0x80000000 + ((FB_START - RAM_START)/4))@lo

    ; First line.
    stw     s1, s11, #0                 ; WAITY   ...
    stw     s2, s11, #4                 ; SETREG ADDR, ...
    ldhi    s12, #(0x84000000+NATIVE_WIDTH)@hi  ; SETREG HSTOP, NATIVE_WIDTH
    or      s12, s12, #(0x84000000+NATIVE_WIDTH)@lo
    stw     s12, s11, #8
    add     s11, s11, #12
    j       pc, #3$@pc

    ; Consecutive lines.
2$:
    stw     s1, s11, #0             ; WAITY   ...
    stw     s2, s11, #4             ; SETREG ADDR, ...
    add     s11, s11, #8
3$:
    add     s1, s1, #NATIVE_HEIGHT/FB_HEIGHT  ; Increment WAITY line (vertical resolution)
    add     s2, s2, #FB_WIDTH/4               ; Increment row address (horizontal stride)
    and     s13, s1, #0x3fff
    slt     s13, s13, #NATIVE_HEIGHT
    bs      s13, 2$

    ; VCP epilogue.
    ldhi    s12, #0x50007fff@hi     ; WAITY 32767 (will never happen)
    or      s12, s12, #0x50007fff@lo
    stw     s12, s11, #0

    ; Clear the frame buffer.
    ldhi    s9, #FB_START@hi
    or      s9, s9, #FB_START@lo                ; s9 = start of frame buffer
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
; Main loop.
; ----------------------------------------------------------------------------

main_loop:
    add     sp, sp, #-12
    stw     lr, sp, #0
    stw     s20, sp, #4
    stw     s21, sp, #8

    ldhi    s20, #MMIO_START
    ldi     s21, #1
1$:
    ; Write the rendered frame count to LEDS.
    stw     s21, s20, #LEDS

    ; Write the machine frame number to some of the 6 segment displays.
    ldw     s1, s20, #VIDFRAMENO
    stw     s1, s20, #SEGDISP0
    lsr     s1, s1, #7
    stw     s1, s20, #SEGDISP1
    lsr     s1, s1, #7
    stw     s1, s20, #SEGDISP2
    lsr     s1, s1, #7
    stw     s1, s20, #SEGDISP3
    lsr     s1, s1, #7
    stw     s1, s20, #SEGDISP4

    ; Draw something to the screen.
    mov     s1, s21
    jl      pc, #draw@pc

    ; Sleep for 1/60 s.
    ldi     s1, #16
    jl      pc, #msleep@pc

    add     s21, s21, #1
    bz      z, 1$

    ldw     lr, sp, #0
    ldw     s20, sp, #4
    ldw     s21, sp, #8
    add     sp, sp, #12
    j       lr


; ----------------------------------------------------------------------------
; Draw something to the frame buffer.
; ----------------------------------------------------------------------------

draw:
    add     sp, sp, #-4
    stw     lr, sp, #0

    jl      pc, #mandelbrot@pc

    ldw     lr, sp, #0
    add     sp, sp, #4
    j       lr


; ----------------------------------------------------------------------------
; Draw a Mandelbrot fractal.
; s1 = frame number (0, 1, ...)
; ----------------------------------------------------------------------------

mandelbrot:
    add     sp, sp, #-4
    stw     s20, sp, #0

    ldw     s13, pc, #coord_step@pc
    ldw     s17, pc, #max_num_iterations@pc
    ldw     s18, pc, #max_distance_sqr@pc

    ; Calculate a zoom factor.
    itof    s1, s1, z
    fmul    s1, s1, s1
    ldhi    s2, #0x3c23c000         ; ~0.01
    fmul    s2, s1, s2
    ldhi    s3, #0x3f800000         ; 1.0
    fadd    s2, s2, s3              ;
    fdiv    s20, s3, s2             ; s20 = 1.0 / (1.0 + frameno^2 * 0.01)

    fmul    s13, s13, s20           ; s13 = coord_step * zoom_factor

    ldhi    s14, #FB_START@hi       ; s14 = pixel_data
    or      s14, s14, #FB_START@lo

    ldi     s16, #FB_HEIGHT ; s16 = loop counter for y
    asr     s3, s16, #1
    itof    s3, s3, z
    fmul    s3, s13, s3
    ldw     s2, pc, #center_im@pc
    fsub    s2, s2, s3      ; s2 = min_im = center_im - coord_step * FB_HEIGHT/2

outer_loop_y:
    ldi     s15, #FB_WIDTH  ; s15 = loop counter for x
    asr     s3, s15, #1
    itof    s3, s3, z
    fmul    s3, s13, s3
    ldw     s1, pc, #center_re@pc
    fsub    s1, s1, s3      ; s1 = min_re = center_re - coord_step * FB_WIDTH/2

outer_loop_x:
    or      s3, z, z        ; s3 = re(z) = 0.0
    or      s4, z, z        ; s4 = im(z) = 0.0

    ldi     s9, #0          ; Iteration count.

inner_loop:
    fmul    s5, s3, s3      ; s5 = re(z)^2
    fmul    s6, s4, s4      ; s6 = im(z)^2
    add     s9, s9, #1
    fmul    s4, s3, s4
    fsub    s3, s5, s6
    fadd    s5, s5, s6      ; s5 = |z|^2
    fadd    s4, s4, s4      ; s4 = 2*re(z)*im(z)
    fadd    s3, s3, s1      ; s3 = re(z)^2 - im(z)^2 + re(c)
    slt     s10, s9, s17    ; num_iterations < max_num_iterations?
    fadd    s4, s4, s2      ; s4 = 2*re(z)*im(z) + im(c)
    fslt    s5, s5, s18     ; |z|^2 < 4.0?

    bns     s5, inner_loop_done
    bs      s10, inner_loop   ; max_num_iterations no reached yet?

    ldi     s9, #0          ; This point is part of the set -> color = 0

inner_loop_done:
    lsl     s9, s9, #1      ; color * 2 for more intense levels

    ; Write color to pixel matrix.
    stb     s9, s14, #0
    add     s14, s14, #1

    ; Increment along the x axis.
    add     s15, s15, #-1
    fadd    s1, s1, s13     ; re(c) = re(c) + coord_step
    bgt     s15, outer_loop_x

    ; Increment along the y axis.
    add     s16, s16, #-1
    fadd    s2, s2, s13     ; im(c) = im(c) + coord_step
    bgt     s16, outer_loop_y

    ldw     s20, sp, #0
    add     sp, sp, #4
    j       lr


max_num_iterations:
    .word   127

max_distance_sqr:
    .float  4.0

center_re:
    .float  -1.156362697351

center_im:
    .float  -0.279199711590

coord_step:
    .float  0.007

