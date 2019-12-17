; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: Video console output
; ----------------------------------------------------------------------------

.include "system/memory.inc"
.include "system/mmio.inc"

VCON_COLS = 40
VCON_ROWS = 22

VCON_WIDTH  = VCON_COLS*8
VCON_HEIGHT = VCON_ROWS*8

VCON_VCP_SIZE = (7 + VCON_HEIGHT*2) * 4
VCON_FB_SIZE  = (VCON_WIDTH * VCON_HEIGHT) / 8

VCON_COL0 = 0xff9b2c2e
VCON_COL1 = 0xffeb6d70


    .lcomm  vcon_vcp_start, 4
    .lcomm  vcon_pal_start, 4
    .lcomm  vcon_fb_start, 4

    .lcomm  vcon_col, 4
    .lcomm  vcon_row, 4

    .text

; ----------------------------------------------------------------------------
; unsigned vcon_memory_requirement(void)
; Determine the memory requirement for the video console.
; ----------------------------------------------------------------------------

    .globl  vcon_memory_requirement
    .p2align 2

vcon_memory_requirement:
    ldi     s1, #VCON_VCP_SIZE+VCON_FB_SIZE
    j       lr


; ----------------------------------------------------------------------------
; void vcon_init(void* addr)
; Create and activate a VCP, a text buffer and a frame buffer.
; ----------------------------------------------------------------------------

    .globl  vcon_init
    .p2align 2

vcon_init:
    add     sp, sp, #-4
    stw     lr, sp, #0

    ; Get the native resolution of the video logic.
    ldhi    s2, #MMIO_START
    ldw     s3, s2, #VIDWIDTH           ; s3 = native video width (e.g. 1920)
    ldw     s4, s2, #VIDHEIGHT          ; s4 = native video height (e.g. 1080)

    ; Calculate the VCP and FB base addresses.
    ; s1 = VCP base address
    ldhi    s7, #vcon_vcp_start@hi
    stw     s1, s7, #vcon_vcp_start@lo

    add     s6, s1, #VCON_VCP_SIZE      ; s6 = FB base address
    ldhi    s7, #vcon_fb_start@hi
    stw     s6, s7, #vcon_fb_start@lo

    ldhi    s7, #VRAM_START
    sub     s6, s6, s7
    lsr     s6, s6, #2                  ; s6 = FB base in video address space

    ; Generate the VCP: Prologue.
    ldhi    s8, #(0x010000*VCON_WIDTH)@hi
    or      s8, s8, #(0x010000*VCON_WIDTH)@lo
    div     s8, s8, s3                  ; s8 = (0x010000 * VCON_WIDTH) / native width
    ldhi    s7, #0x82000000
    or      s7, s7, s8
    stw     s7, s1, #0                  ; SETREG  XINCR, ...

    ldhi    s7, #0x84000000
    or      s7, s7, s3
    stw     s7, s1, #4                  ; SETREG  HSTOP, native width

    ldhi    s7, #0x85000000
    or      s7, s7, #5
    stw     s7, s1, #8                  ; SETREG  CMODE, 5

    ldhi    s7, #0x60000000
    or      s7, s7, #1
    stw     s7, s1, #12                 ; SETPAL  0, 1

    ldhi    s7, #VCON_COL0@hi
    or      s7, s7, #VCON_COL0@lo
    stw     s7, s1, #16                 ; COLOR 0

    ldhi    s7, #VCON_COL1@hi
    or      s7, s7, #VCON_COL1@lo
    stw     s7, s1, #20                 ; COLOR 1

    add     s7, s1, #16
    ldhi    s8, #vcon_pal_start@hi
    stw     s7, s8, #vcon_pal_start@lo  ; Store the palette address

    add     s1, s1, #24

    ; Generate the VCP: Per row memory pointers.
    ldhi    s7, #0x80000000
    ldhi    s8, #0x50000000
    ldi     s11, #VCON_HEIGHT
    ldi     s9, #0
1$:
    mul     s10, s9, s4
    div     s10, s10, s11               ; s10 = y * native_height / VCON_HEIGHT
    or      s10, s8, s10
    stw     s10, s1, #0                 ; WAITY   y * native_height / VCON_HEIGHT
    add     s9, s9, #1

    add     s10, s7, s6
    stw     s10, s1, #4                 ; SETREG  ADDR, ...
    add     s6, s6, #VCON_COLS/4

    add     s1, s1, #8

    seq     s10, s9, #VCON_HEIGHT
    bns     s10, 1$

    ; Generate the VCP: Epilogue.
    ldhi    s7, #0x50007fff@hi
    or      s7, s7, #0x50007fff@lo
    stw     s7, s1, #0                  ; WAITY  32767

    ; Clear the screen.
    jl      pc, #vcon_clear@pc

    ; Activate the vconsole VCP.
    jl      pc, #vcon_show@pc

    ldw     lr, sp, #0
    add     sp, sp, #4
    j       lr


; ----------------------------------------------------------------------------
; void vcon_show()
; Activate the vcon VCP.
; ----------------------------------------------------------------------------

    .globl  vcon_show
    .p2align 2

vcon_show:
    ldhi    s1, #vcon_vcp_start@hi
    ldw     s1, s1, #vcon_vcp_start@lo
    ldhi    s2, #VRAM_START
    sub     s1, s1, s2
    lsr     s1, s1, #2                ; s1 = (vcon_vcp_start - VRAM_START) / 4
    stw     s1, s2, #0                ; JMP vcp_start

    j       lr


; ----------------------------------------------------------------------------
; void vcon_clear()
; Clear the VCON frame buffer and reset the coordinates.
; ----------------------------------------------------------------------------

    .globl  vcon_clear
    .p2align 2

vcon_clear:
    ; Clear the col, row coordinate.
    ldhi    s1, #vcon_col@hi
    stw     z, s1, #vcon_col@lo
    ldhi    s1, #vcon_row@hi
    stw     z, s1, #vcon_row@lo

    ; Clear the frame buffer.
    ldhi    s1, #vcon_fb_start@hi
    ldw     s1, s1, #vcon_fb_start@lo
    ldi     s2, #0
    ldi     s3, #VCON_FB_SIZE

    j       pc, #mem_fill@pc


; ----------------------------------------------------------------------------
; void vcon_set_colors(unsigned col0, unsigned col1)
; Set the palette.
; ----------------------------------------------------------------------------

    .globl  vcon_set_colors
    .p2align 2

vcon_set_colors:
    ldhi    s3, #vcon_pal_start@hi
    ldw     s3, s3, #vcon_pal_start@lo
    stw     s1, s3, #0
    stw     s2, s3, #4

    j       lr


; ----------------------------------------------------------------------------
; void vcon_print(char* text)
; Print a zero-terminated string.
; ----------------------------------------------------------------------------

    .globl  vcon_print
    .p2align 2

vcon_print:
    mov     s10, vl                     ; Preserve vl (without using the stack)

    ldhi    s2, #mc1_font_8x8@hi
    or      s2, s2, #mc1_font_8x8@lo    ; s2 = font

    ldhi    s3, #vcon_col@hi
    ldw     s3, s3, #vcon_col@lo        ; s3 = col

    ldhi    s4, #vcon_row@hi
    ldw     s4, s4, #vcon_row@lo        ; s4 = row

    ldhi    s8, #vcon_fb_start@hi
    ldw     s8, s8, #vcon_fb_start@lo   ; s8 = frame buffer start

1$:
    ldub    s5, s1, #0
    add     s1, s1, #1
    bz      s5, 2$

    ; New line (LF)?
    seq     s6, s5, #10
    bs      s6, 3$

    ; Carriage return (CR)?
    seq     s6, s5, #13
    bns     s6, 4$
    ldi     s3, #0
    j       pc, #1$@pc

4$:
    ; Tab?
    seq     s6, s5, #9
    bns     s6, 5$
    add     s3, s3, #8
    bic     s3, s3, #7
    slt     s6, s3, #VCON_COLS
    bs      s6, 1$
    j       pc, #3$@pc

5$:
    ; Printable char.
    max     s5, s5, #32
    min     s5, s5, #127

    add     s5, s5, #-32
    ldea    s5, s2, s5*8                ; s5 = start of glyph

    ; Copy glyph (8 bytes) from the font to the frame buffer.
    ldi     vl, #8
    ldi     s7, #VCON_COLS
    mul     s6, s4, s7
    ldub    v1, s5, #1                  ; Load entire glyph (8 bytes)
    lsl     s6, s6, #3
    add     s6, s6, s3
    add     s6, s8, s6                  ; s6 = FB + (row * VCON_COLS * 8) + col
    stb     v1, s6, s7                  ; Store glyph with stride = VCON_COLS

    add     s3, s3, #1
    slt     s5, s3, #VCON_COLS
    bs      s5, 1$

3$:
    ; New line
    ldi     s3, #0
    add     s4, s4, #1
    slt     s5, s4, #VCON_ROWS
    bs      s5, 1$

    ; End of frame buffer.
    ldi     s4, #VCON_ROWS-1

    ; Scroll screen up one row.

    ; 1) Move entire frame buffer.
    ; Clobbered registers: s5, s6, s7, s9
    add     s7, s8, #VCON_COLS*8        ; s7 = source (start of FB + one row)
    mov     s9, s8                      ; s9 = target (start of FB)
    cpuid   s5, z, z
    ldi     s6, #(VCON_COLS*8 * (VCON_ROWS-1)) / 4  ; Number of words to move
6$:
    min     vl, s5, s6
    sub     s6, s6, vl
    ldw     v1, s7, #4
    ldea    s7, s7, vl*4
    stw     v1, s9, #4
    ldea    s9, s9, vl*4
    bnz     s6, 6$

    ; 2) Clear last row (continue writing at s9 and forward).
    ldi     s6, #(VCON_COLS*8) / 4      ; Number of words to clear
7$:
    min     vl, s5, s6
    sub     s6, s6, vl
    stw     vz, s9, #4
    ldea    s9, s9, vl*4
    bnz     s6, 7$

    j       pc, #1$@pc

2$:
    ldhi    s5, #vcon_col@hi
    stw     s3, s5, #vcon_col@lo

    ldhi    s5, #vcon_row@hi
    stw     s4, s5, #vcon_row@lo

    mov     vl, s10                     ; Restore vl
    j       lr


; ----------------------------------------------------------------------------
; void vcon_print_hex(unsigned x)
; Print a hexadecimal number.
; ----------------------------------------------------------------------------

    .globl  vcon_print_hex
    .p2align 2

vcon_print_hex:
    add     sp, sp, #-16
    stw     lr, sp, #12

    ; Build an ASCII string on the stack.
    ldhi    s4, #hex_to_ascii@hi
    or      s4, s4, #hex_to_ascii@lo
    ldi     s2, #8
    stb     z, sp, s2           ; Zero termination
1$:
    and     s3, s1, #0x0f
    ldub    s3, s4, s3
    add     s2, s2, #-1
    lsr     s1, s1, #4
    stb     s3, sp, s2
    bgt     s2, 1$

    ; Call the regular printing routine.
    mov     s1, sp
    jl      pc, #vcon_print@pc

    ldw     lr, sp, #12
    add     sp, sp, #16
    j       lr


; ----------------------------------------------------------------------------
; void vcon_print_dec(int x)
; Print a signed decimal number.
; ----------------------------------------------------------------------------

    .globl  vcon_print_dec
    .p2align 2

vcon_print_dec:
    add     sp, sp, #-16
    stw     lr, sp, #12

    ; Build an ASCII string on the stack.
    ldhi    s4, #hex_to_ascii@hi
    or      s4, s4, #hex_to_ascii@lo
    ldi     s2, #11
    stb     z, sp, s2           ; Zero termination

    ldi     s6, #10

    ; Negative?
    slt     s5, s1, z
    bns     s5, 1$
    sub     s1, z, s1

1$:
    remu    s3, s1, s6
    add     s2, s2, #-1
    ldub    s3, s4, s3
    divu    s1, s1, s6
    stb     s3, sp, s2
    bnz     s1, 1$

    ; Prepend a minus sign?
    bns     s5, 2$
    ldi     s3, #45             ; Minus sign (-)
    add     s2, s2, #-1
    stb     s3, sp, s2

2$:
    ; Call the regular printing routine.
    ldea    s1, sp, s2
    jl      pc, #vcon_print@pc

    ldw     lr, sp, #12
    add     sp, sp, #16
    j       lr


    .section .rodata
hex_to_ascii:
    .ascii  "0123456789ABCDEF"

