; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This is the main boot program.
; ----------------------------------------------------------------------------

.include "system/memory.inc"
.include "system/mmio.inc"

FB_WIDTH  = 640
FB_HEIGHT = 360

NATIVE_WIDTH = 1920
NATIVE_HEIGHT = 1080


; ----------------------------------------------------------------------------
; Static variables.
; ----------------------------------------------------------------------------

    .lcomm  vcp_start, 4
    .lcomm  fb_start, 4

    .text

; ----------------------------------------------------------------------------
; Main program entry.
; ----------------------------------------------------------------------------

    .globl  main

main:
    add     sp, sp, #-4
    stw     lr, sp, #0

    ; Init video.
    bl      init_video

    ; Enter the main loop.
    bl      main_loop

    ldw     lr, sp, #0
    add     sp, sp, #4
    ret


; ----------------------------------------------------------------------------
; Init video.
; ----------------------------------------------------------------------------

init_video:
    add     sp, sp, #-8
    stw     lr, sp, #0
    stw     vl, sp, #4

    ; Allocate memory for the VCP.
    ldi     s1, #16+256*4+4+FB_HEIGHT*12+4
    ldi     s2, #MEM_TYPE_VIDEO
    bl      mem_alloc
    ldhi    s2, #vcp_start@hi
    stw     s1, s2, #vcp_start@lo
    bz      s1, init_fail

    ; Allocate memory for the frame buffer.
    ldi     s1, #FB_WIDTH * FB_HEIGHT
    ldi     s2, #MEM_TYPE_VIDEO
    bl      mem_alloc
    ldhi    s2, #fb_start@hi
    stw     s1, s2, #fb_start@lo
    bz      s1, init_fail

    ldhi    s11, #vcp_start@hi
    ldw     s11, s11, #vcp_start@lo ; s11 = start of Video Control Program

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

    ldhi    s12, #fb_start@hi
    ldw     s12, s12, #fb_start@lo
    ldhi    s13, #VRAM_START
    sub     s12, s12, s13
    lsr     s12, s12, #2                ; s12 = (fb_start - VRAM_START)/4
    ldhi    s2, #0x80000000
    or      s2, s2, s12                 ; s2 = SETREG ADDR, (fb_start - VRAM_START)/4

    ; First line.
    stw     s1, s11, #0                 ; WAITY   ...
    stw     s2, s11, #4                 ; SETREG ADDR, ...
    ldhi    s12, #(0x84000000+NATIVE_WIDTH)@hi  ; SETREG HSTOP, NATIVE_WIDTH
    or      s12, s12, #(0x84000000+NATIVE_WIDTH)@lo
    stw     s12, s11, #8
    add     s11, s11, #12
    b       3$

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
    ldhi    s9, #fb_start@hi
    ldw     s9, s9, #fb_start@lo                ; s9 = start of frame buffer
    cpuid   s10, z, z                           ; s10 = max vector length
    ldi     s11, #(FB_WIDTH * FB_HEIGHT) / 4    ; s11 = number of words
4$:
    min     vl, s10, s11
    sub     s11, s11, vl
    stw     vz, s9, #4              ; Store zeroes in the frame buffer
    ldea    s9, s9, vl * 4
    bnz     s11, 4$

    bl      fb_show

init_fail:
    ldw     lr, sp, #0
    ldw     vl, sp, #4
    add     sp, sp, #8
    ret


; ----------------------------------------------------------------------------
; Show the application frame buffer.
; ----------------------------------------------------------------------------

fb_show:
    ; Set the VCP0 jump target to the start of our VCP.
    ldhi    s1, #vcp_start@hi
    ldw     s1, s1, #vcp_start@lo
    bz      s1, 1$
    ldhi    s2, #VRAM_START
    sub     s1, s1, s2
    lsr     s1, s1, #2              ; s1 = JMP (vcp_start - VRAM_START)/4
    stw     s1, s2, #0              ; Set VCP0 jump target

1$:
    ret


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
    ; Draw something to the screen.
    mov     s1, s21
    bl      draw

    ; Write the raster Y position to the segment displays.
    ldw     s1, s20, #VIDY
    bl      sevseg_print_dec

    ; Write the rendered frame count to LEDS.
    stw     s21, s20, #LEDS

    ; Wait for the next vertical blanking interval. We busy lopp since we
    ; don't have interrupts yet.
    ldw     s2, s20, #VIDFRAMENO
2$:
    ldw     s1, s20, #VIDFRAMENO
    sne     s1, s1, s2
    bns     s1, 2$

    add     s21, s21, #1
    b       1$                  ; Infinite loop...

    ldw     lr, sp, #0
    ldw     s20, sp, #4
    ldw     s21, sp, #8
    add     sp, sp, #12
    ret


; ----------------------------------------------------------------------------
; Draw something to the frame buffer.
; s1 = frame number (0, 1, ...)
; ----------------------------------------------------------------------------

draw:
    add     sp, sp, #-8
    stw     lr, sp, #0
    stw     s20, sp, #4

    mov     s20, s1

    ; Select drawing routine based on the state of SWITCHES.
    ldhi    s2, #MMIO_START
    ldw     s2, s2, #SWITCHES

    ; Show funky?
    and     s3, s2, #1
    bz      s3, 1$

    bl      fb_show
    mov     s1, s20
    bl      funky
    b       3$

1$:
    ; Show mandelbrot?
    and     s3, s2, #2
    bz      s3, 2$

    bl      fb_show
    mov     s1, s20
    bl      mandelbrot
    b       3$

2$:
    ; Default: Show VCON.
    bl      vcon_show

3$:
    ldw     lr, sp, #0
    ldw     s20, sp, #4
    add     sp, sp, #8
    ret


; ----------------------------------------------------------------------------
; Draw a Mandelbrot fractal.
; s1 = frame number (0, 1, ...)
; ----------------------------------------------------------------------------

mandelbrot:
    add     sp, sp, #-16
    stw     s16, sp, #12
    stw     s17, sp, #8
    stw     s18, sp, #4
    stw     s20, sp, #0

    and     s1, s1, #127
    slt     s2, s1, #64
    bs      s2, 1$
    sub     s1, #128, s1
1$:

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

    ldhi    s14, #fb_start@hi
    ldw     s14, s14, #fb_start@lo  ; s14 = pixel_data
    bz      s14, mandelbrot_fail

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

mandelbrot_fail:
    ldw     s20, sp, #0
    ldw     s18, sp, #4
    ldw     s17, sp, #8
    ldw     s16, sp, #12
    add     sp, sp, #16
    ret


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



; ----------------------------------------------------------------------------
; Some funky graphics.
; s1 = frame number (0, 1, ...)
; ----------------------------------------------------------------------------

funky:
    add     sp, sp, #-12
    stw     s20, sp, #0
    stw     s21, sp, #4
    stw     vl, sp, #8

    add     s1, s1, s1              ; Increase animation speed

    cpuid   s13, z, z               ; s13 = memory stride per vector operation
    mov     vl, s13

    add     s2, s13, #-1
    ldea    v4, s2, #-1             ; v4 is a ramp from vl-1 downto 0

    ldhi    s21, #sine1024@hi
    or      s21, s21, #sine1024@lo  ; s21 = start of 1024-entry sine table

    ldhi    s6, #fb_start@hi
    ldw     s6, s6, #fb_start@lo    ; s6 = video frame buffer
    bz      s6, funky_fail

    ldi     s8, #FB_HEIGHT          ; s8 = y counter
loop_y:
    add     s8, s8, #-1             ; Decrement the y counter

    add     s9, s8, s1

    ldi     s7, #FB_WIDTH/2         ; s7 = x counter
loop_x:
    min     vl, s13, s7
    sub     s7, s7, vl              ; Decrement the x counter

    ; Some funky kind of test pattern...
    add     v7, v4, s7

    ldea    s20, s7, s1*2
    add     v9, v4, s20
    and     v8, v9, #1023
    ldh     v8, s21, v8*2
    mulq.h  v7, v7, v8
    add.h   v1, v7, s9

    stw     v1, s6, #2
    ldea    s6, s6, s13*2           ; Increment the memory pointer

    bgt     s7, #loop_x
    bgt     s8, #loop_y

funky_fail:
    ldw     s20, sp, #0
    ldw     s21, sp, #4
    ldw     vl, sp, #8
    add     sp, sp, #12
    ret


    .section .rodata
    .p2align 2

sine1024:
    ; This is a 1024-entry LUT of sin(x), in Q15 format.
    .half   0, 201, 402, 603, 804, 1005, 1206, 1407, 1608, 1809, 2009, 2210, 2410, 2611, 2811
    .half   3012, 3212, 3412, 3612, 3811, 4011, 4210, 4410, 4609, 4808, 5007, 5205, 5404, 5602
    .half   5800, 5998, 6195, 6393, 6590, 6786, 6983, 7179, 7375, 7571, 7767, 7962, 8157, 8351
    .half   8545, 8739, 8933, 9126, 9319, 9512, 9704, 9896, 10087, 10278, 10469, 10659, 10849
    .half   11039, 11228, 11417, 11605, 11793, 11980, 12167, 12353, 12539, 12725, 12910, 13094
    .half   13279, 13462, 13645, 13828, 14010, 14191, 14372, 14553, 14732, 14912, 15090, 15269
    .half   15446, 15623, 15800, 15976, 16151, 16325, 16499, 16673, 16846, 17018, 17189, 17360
    .half   17530, 17700, 17869, 18037, 18204, 18371, 18537, 18703, 18868, 19032, 19195, 19357
    .half   19519, 19680, 19841, 20000, 20159, 20317, 20475, 20631, 20787, 20942, 21096, 21250
    .half   21403, 21554, 21705, 21856, 22005, 22154, 22301, 22448, 22594, 22739, 22884, 23027
    .half   23170, 23311, 23452, 23592, 23731, 23870, 24007, 24143, 24279, 24413, 24547, 24680
    .half   24811, 24942, 25072, 25201, 25329, 25456, 25582, 25708, 25832, 25955, 26077, 26198
    .half   26319, 26438, 26556, 26674, 26790, 26905, 27019, 27133, 27245, 27356, 27466, 27575
    .half   27683, 27790, 27896, 28001, 28105, 28208, 28310, 28411, 28510, 28609, 28706, 28803
    .half   28898, 28992, 29085, 29177, 29268, 29358, 29447, 29534, 29621, 29706, 29791, 29874
    .half   29956, 30037, 30117, 30195, 30273, 30349, 30424, 30498, 30571, 30643, 30714, 30783
    .half   30852, 30919, 30985, 31050, 31113, 31176, 31237, 31297, 31356, 31414, 31470, 31526
    .half   31580, 31633, 31685, 31736, 31785, 31833, 31880, 31926, 31971, 32014, 32057, 32098
    .half   32137, 32176, 32213, 32250, 32285, 32318, 32351, 32382, 32412, 32441, 32469, 32495
    .half   32521, 32545, 32567, 32589, 32609, 32628, 32646, 32663, 32678, 32692, 32705, 32717
    .half   32728, 32737, 32745, 32752, 32757, 32761, 32765, 32766, 32767, 32766, 32765, 32761
    .half   32757, 32752, 32745, 32737, 32728, 32717, 32705, 32692, 32678, 32663, 32646, 32628
    .half   32609, 32589, 32567, 32545, 32521, 32495, 32469, 32441, 32412, 32382, 32351, 32318
    .half   32285, 32250, 32213, 32176, 32137, 32098, 32057, 32014, 31971, 31926, 31880, 31833
    .half   31785, 31736, 31685, 31633, 31580, 31526, 31470, 31414, 31356, 31297, 31237, 31176
    .half   31113, 31050, 30985, 30919, 30852, 30783, 30714, 30643, 30571, 30498, 30424, 30349
    .half   30273, 30195, 30117, 30037, 29956, 29874, 29791, 29706, 29621, 29534, 29447, 29358
    .half   29268, 29177, 29085, 28992, 28898, 28803, 28706, 28609, 28510, 28411, 28310, 28208
    .half   28105, 28001, 27896, 27790, 27683, 27575, 27466, 27356, 27245, 27133, 27019, 26905
    .half   26790, 26674, 26556, 26438, 26319, 26198, 26077, 25955, 25832, 25708, 25582, 25456
    .half   25329, 25201, 25072, 24942, 24811, 24680, 24547, 24413, 24279, 24143, 24007, 23870
    .half   23731, 23592, 23452, 23311, 23170, 23027, 22884, 22739, 22594, 22448, 22301, 22154
    .half   22005, 21856, 21705, 21554, 21403, 21250, 21096, 20942, 20787, 20631, 20475, 20317
    .half   20159, 20000, 19841, 19680, 19519, 19357, 19195, 19032, 18868, 18703, 18537, 18371
    .half   18204, 18037, 17869, 17700, 17530, 17360, 17189, 17018, 16846, 16673, 16499, 16325
    .half   16151, 15976, 15800, 15623, 15446, 15269, 15090, 14912, 14732, 14553, 14372, 14191
    .half   14010, 13828, 13645, 13462, 13279, 13094, 12910, 12725, 12539, 12353, 12167, 11980
    .half   11793, 11605, 11417, 11228, 11039, 10849, 10659, 10469, 10278, 10087, 9896, 9704
    .half   9512, 9319, 9126, 8933, 8739, 8545, 8351, 8157, 7962, 7767, 7571, 7375, 7179, 6983
    .half   6786, 6590, 6393, 6195, 5998, 5800, 5602, 5404, 5205, 5007, 4808, 4609, 4410, 4210
    .half   4011, 3811, 3612, 3412, 3212, 3012, 2811, 2611, 2410, 2210, 2009, 1809, 1608, 1407
    .half   1206, 1005, 804, 603, 402, 201, 0, -201, -402, -603, -804, -1005, -1206, -1407
    .half   -1608, -1809, -2009, -2210, -2410, -2611, -2811, -3012, -3212, -3412, -3612, -3811
    .half   -4011, -4210, -4410, -4609, -4808, -5007, -5205, -5404, -5602, -5800, -5998, -6195
    .half   -6393, -6590, -6786, -6983, -7179, -7375, -7571, -7767, -7962, -8157, -8351, -8545
    .half   -8739, -8933, -9126, -9319, -9512, -9704, -9896, -10087, -10278, -10469, -10659
    .half   -10849, -11039, -11228, -11417, -11605, -11793, -11980, -12167, -12353, -12539
    .half   -12725, -12910, -13094, -13279, -13462, -13645, -13828, -14010, -14191, -14372
    .half   -14553, -14732, -14912, -15090, -15269, -15446, -15623, -15800, -15976, -16151
    .half   -16325, -16499, -16673, -16846, -17018, -17189, -17360, -17530, -17700, -17869
    .half   -18037, -18204, -18371, -18537, -18703, -18868, -19032, -19195, -19357, -19519
    .half   -19680, -19841, -20000, -20159, -20317, -20475, -20631, -20787, -20942, -21096
    .half   -21250, -21403, -21554, -21705, -21856, -22005, -22154, -22301, -22448, -22594
    .half   -22739, -22884, -23027, -23170, -23311, -23452, -23592, -23731, -23870, -24007
    .half   -24143, -24279, -24413, -24547, -24680, -24811, -24942, -25072, -25201, -25329
    .half   -25456, -25582, -25708, -25832, -25955, -26077, -26198, -26319, -26438, -26556
    .half   -26674, -26790, -26905, -27019, -27133, -27245, -27356, -27466, -27575, -27683
    .half   -27790, -27896, -28001, -28105, -28208, -28310, -28411, -28510, -28609, -28706
    .half   -28803, -28898, -28992, -29085, -29177, -29268, -29358, -29447, -29534, -29621
    .half   -29706, -29791, -29874, -29956, -30037, -30117, -30195, -30273, -30349, -30424
    .half   -30498, -30571, -30643, -30714, -30783, -30852, -30919, -30985, -31050, -31113
    .half   -31176, -31237, -31297, -31356, -31414, -31470, -31526, -31580, -31633, -31685
    .half   -31736, -31785, -31833, -31880, -31926, -31971, -32014, -32057, -32098, -32137
    .half   -32176, -32213, -32250, -32285, -32318, -32351, -32382, -32412, -32441, -32469
    .half   -32495, -32521, -32545, -32567, -32589, -32609, -32628, -32646, -32663, -32678
    .half   -32692, -32705, -32717, -32728, -32737, -32745, -32752, -32757, -32761, -32765
    .half   -32766, -32767, -32766, -32765, -32761, -32757, -32752, -32745, -32737, -32728
    .half   -32717, -32705, -32692, -32678, -32663, -32646, -32628, -32609, -32589, -32567
    .half   -32545, -32521, -32495, -32469, -32441, -32412, -32382, -32351, -32318, -32285
    .half   -32250, -32213, -32176, -32137, -32098, -32057, -32014, -31971, -31926, -31880
    .half   -31833, -31785, -31736, -31685, -31633, -31580, -31526, -31470, -31414, -31356
    .half   -31297, -31237, -31176, -31113, -31050, -30985, -30919, -30852, -30783, -30714
    .half   -30643, -30571, -30498, -30424, -30349, -30273, -30195, -30117, -30037, -29956
    .half   -29874, -29791, -29706, -29621, -29534, -29447, -29358, -29268, -29177, -29085
    .half   -28992, -28898, -28803, -28706, -28609, -28510, -28411, -28310, -28208, -28105
    .half   -28001, -27896, -27790, -27683, -27575, -27466, -27356, -27245, -27133, -27019
    .half   -26905, -26790, -26674, -26556, -26438, -26319, -26198, -26077, -25955, -25832
    .half   -25708, -25582, -25456, -25329, -25201, -25072, -24942, -24811, -24680, -24547
    .half   -24413, -24279, -24143, -24007, -23870, -23731, -23592, -23452, -23311, -23170
    .half   -23027, -22884, -22739, -22594, -22448, -22301, -22154, -22005, -21856, -21705
    .half   -21554, -21403, -21250, -21096, -20942, -20787, -20631, -20475, -20317, -20159
    .half   -20000, -19841, -19680, -19519, -19357, -19195, -19032, -18868, -18703, -18537
    .half   -18371, -18204, -18037, -17869, -17700, -17530, -17360, -17189, -17018, -16846
    .half   -16673, -16499, -16325, -16151, -15976, -15800, -15623, -15446, -15269, -15090
    .half   -14912, -14732, -14553, -14372, -14191, -14010, -13828, -13645, -13462, -13279
    .half   -13094, -12910, -12725, -12539, -12353, -12167, -11980, -11793, -11605, -11417
    .half   -11228, -11039, -10849, -10659, -10469, -10278, -10087, -9896, -9704, -9512, -9319
    .half   -9126, -8933, -8739, -8545, -8351, -8157, -7962, -7767, -7571, -7375, -7179, -6983
    .half   -6786, -6590, -6393, -6195, -5998, -5800, -5602, -5404, -5205, -5007, -4808, -4609
    .half   -4410, -4210, -4011, -3811, -3612, -3412, -3212, -3012, -2811, -2611, -2410, -2210
    .half   -2009, -1809, -1608, -1407, -1206, -1005, -804, -603, -402, -201
