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
    .lcomm  current_prg_no, 4

    .text

; ----------------------------------------------------------------------------
; Main program entry.
; ----------------------------------------------------------------------------

    .globl  main
    .p2align 2
main:
    add     sp, sp, #-4
    stw     lr, sp, #0

    ; Set the default program.
    ldi     s1, #-1
    addpchi s15, #current_prg_no@pchi
    stw     s1, s15, #current_prg_no+4@pclo

    ; Enter the main loop.
    bl      main_loop

    ldw     lr, sp, #0
    add     sp, sp, #4
    ret


; ----------------------------------------------------------------------------
; Init video.
; ----------------------------------------------------------------------------

    .p2align 2
init_video:
    add     sp, sp, #-8
    stw     lr, sp, #0
    stw     vl, sp, #4

    ; Allocate memory for the VCP.
    ldi     s1, #16+256*4+4+FB_HEIGHT*12+4
    ldi     s2, #MEM_TYPE_VIDEO
    bl      mem_alloc
    ldhi    s15, #vcp_start@hi
    stw     s1, s15, #vcp_start@lo
    bz      s1, init_fail

    ; Allocate memory for the frame buffer.
    ldi     s1, #FB_WIDTH * FB_HEIGHT
    ldi     s2, #MEM_TYPE_VIDEO | MEM_CLEAR
    bl      mem_alloc
    ldhi    s15, #fb_start@hi
    stw     s1, s15, #fb_start@lo
    bnz     s1, init_allocation_ok

    ldhi    s1, #vcp_start@hi
    ldw     s1, s1, #vcp_start@lo
    bl      mem_free
    b       init_fail

init_allocation_ok:
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

    bl      fb_show

init_done:
    ldw     lr, sp, #0
    ldw     vl, sp, #4
    add     sp, sp, #8
    ret

init_fail:
    addpchi s1, #fail_text@pchi
    add     s1, s1, #fail_text+4@pclo
    bl      vcon_print
    b       init_done


; ----------------------------------------------------------------------------
; De-init video.
; ----------------------------------------------------------------------------

    .p2align 2
deinit_video:
    add     sp, sp, #-4
    stw     lr, sp, #0

    ldhi    s1, #vcp_start@hi
    ldw     s1, s1, #vcp_start@lo
    bz      s1, 1f
    bl      mem_free
    ldhi    s15, #vcp_start@hi
    stw     z, s15, #vcp_start@lo
1:

    ldhi    s1, #fb_start@hi
    ldw     s1, s1, #fb_start@lo
    bz      s1, 2f
    bl      mem_free
    ldhi    s15, #fb_start@hi
    stw     z, s15, #fb_start@lo
2:

    ldw     lr, sp, #0
    add     sp, sp, #4
    ret


; ----------------------------------------------------------------------------
; Show the application frame buffer.
; ----------------------------------------------------------------------------

    .p2align 2
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

    .p2align 2
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

    .p2align 2
draw:
    add     sp, sp, #-8
    stw     lr, sp, #0
    stw     s20, sp, #4

    mov     s20, s1

    ; Select drawing routine based on the state of SWITCHES.
    ldhi    s2, #MMIO_START
    ldw     s2, s2, #SWITCHES

    ; Is this the same program as the last frame?
    addpchi s3, #current_prg_no@pchi
    ldea    s3, s3, #current_prg_no+4@pclo
    ldw     s4, s3, #0
    stw     s2, s3, #0
    seq     s3, s2, s4      ; s3 = true for no program change

    seq     s4, s2, #0
    bs      s4, draw_vcon
    seq     s4, s2, #1
    bs      s4, draw_funky
    seq     s4, s2, #2
    bs      s4, draw_mandel

    ; Fall through to draw_vcon

draw_vcon:
    ; Show VCON.
    bs      s3, 1f
    bl      deinit_video
    bl      vcon_show
1:  b       draw_done

draw_funky:
    ; Show funky
    bs      s3, 1f
    bl      init_video
    bl      fb_show
1:  mov     s1, s20
    addpchi s2, #fb_start@pchi
    ldw     s2, s2, #fb_start+4@pclo
    bl      funky
    b       draw_done

draw_mandel:
    ; Show mandelbrot
    bs      s3, 1f
    bl      init_video
    bl      fb_show
1:  mov     s1, s20
    addpchi s2, #fb_start@pchi
    ldw     s2, s2, #fb_start+4@pclo
    bl      mandelbrot
    b       draw_done

draw_done:
    ldw     lr, sp, #0
    ldw     s20, sp, #4
    add     sp, sp, #8
    ret



    .section .rodata
    .p2align 2

fail_text:
    .asciz  "\nAPP1: Failed to initialize :-(\n"

