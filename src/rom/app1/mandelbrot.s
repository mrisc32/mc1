; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This is the main boot program.
; ----------------------------------------------------------------------------

.include "mc1/framebuffer.inc"
.include "mc1/memory.inc"
.include "mc1/mmio.inc"

WIDTH  = 416 ; 640
HEIGHT = 234 ; 360

    .lcomm  s_fb, 4


;---------------------------------------------------------------------------------------------------
; void mandelbrot_init(void)
;---------------------------------------------------------------------------------------------------

    .text
    .p2align 2
    .global mandelbrot_init
mandelbrot_init:
    add     sp, sp, #-4
    stw     lr, sp, #0

    addpchi s1, #s_fb@pchi
    ldw     s1, s1, #s_fb@pclo
    bnz     s1, mandelbrot_init_done

    ; s_fb = fb_create(WIDTH, HEIGHT, MODE_PAL8)
    ldi     s1, #WIDTH
    ldi     s2, #HEIGHT
    ldi     s3, #MODE_PAL8
    call    fb_create@pc
    addpchi s2, #s_fb@pchi
    stw     s1, s2, #s_fb@pclo

    ; set_mandelbrot_palette(s_fb)
    call    set_mandelbrot_palette@pc

mandelbrot_init_done:
    ldw     lr, sp, #0
    add     sp, sp, #4
    ret


;---------------------------------------------------------------------------------------------------
; void mandelbrot_deinit(void)
;---------------------------------------------------------------------------------------------------

    .text
    .p2align 2
    .global mandelbrot_deinit
mandelbrot_deinit:
    add     sp, sp, #-4
    stw     lr, sp, #0

    ; fb_destroy(s_fb)
    addpchi s1, #s_fb@pchi
    ldw     s1, s1, #s_fb@pclo
    bz      s1, mandelbrot_deinit_done
    call    fb_destroy@pc

    ; s_fb = NULL
    addpchi s1, #s_fb@pchi
    stw     z, s1, #s_fb@pclo

mandelbrot_deinit_done:
    ldw     lr, sp, #0
    add     sp, sp, #4
    ret


;---------------------------------------------------------------------------------------------------
; Draw a Mandelbrot fractal.
;
; void mandelbrot(int frame_no)
;   s1 = frame number (0, 1, ...)
;---------------------------------------------------------------------------------------------------

    .text
    .p2align 2
    .global mandelbrot
mandelbrot:
    add     sp, sp, #-20
    stw     s16, sp, #0
    stw     s17, sp, #4
    stw     s18, sp, #8
    stw     s20, sp, #12
    stw     lr, sp, #16

    mov     s20, s1
    addpchi s1, #s_fb@pchi
    ldw     s1, s1, #s_fb@pclo
    call    fb_show@pc
    mov     s1, s20

    addpchi s2, #s_fb@pchi
    ldw     s2, s2, #s_fb@pclo
    bz      s2, mandelbrot_fail
    ldw     s14, s2, #FB_PIXELS     ; s14 = pixel_data

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
    ldi     s2, #0x3c23c000         ; ~0.01
    fmul    s2, s1, s2
    ldi     s3, #0x3f800000         ; 1.0
    fadd    s2, s2, s3              ;
    fdiv    s20, s3, s2             ; s20 = 1.0 / (1.0 + frameno^2 * 0.01)

    fmul    s13, s13, s20           ; s13 = coord_step * zoom_factor

    ldi     s16, #HEIGHT    ; s16 = loop counter for y
    asr     s3, s16, #1
    itof    s3, s3, z
    fmul    s3, s13, s3
    ldw     s2, pc, #center_im@pc
    fsub    s2, s2, s3      ; s2 = min_im = center_im - coord_step * HEIGHT/2

outer_loop_y:
    ldi     s15, #WIDTH     ; s15 = loop counter for x
    asr     s3, s15, #1
    itof    s3, s3, z
    fmul    s3, s13, s3
    ldw     s1, pc, #center_re@pc
    fsub    s1, s1, s3      ; s1 = min_re = center_re - coord_step * WIDTH/2

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
    ldw     s16, sp, #0
    ldw     s17, sp, #4
    ldw     s18, sp, #8
    ldw     s20, sp, #12
    ldw     lr, sp, #16
    add     sp, sp, #20
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
    .float  0.011 ; 0.007

