; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the common startup code. It defines _start, which does
; some initialization and then calls main.
; ----------------------------------------------------------------------------

.include "system/memory.inc"
.include "system/mmio.inc"

STACK_SIZE = 4*1024


    .section .entry

    .globl  _start

_start:
    ; ------------------------------------------------------------------------
    ; Clear the BSS data.
    ; ------------------------------------------------------------------------

    ldhi    s2, #__bss_size@hi
    or      s2, s2, #__bss_size@lo
    bz      s2, bss_cleared
    lsr     s2, s2, #2      ; BSS size is always a multiple of 4 bytes.

    ldhi    s1, #__bss_start@hi
    or      s1, s1, #__bss_start@lo
    cpuid   s3, z, z
clear_bss_loop:
    min     vl, s2, s3
    stw     vz, s1, #4
    sub     s2, s2, vl
    ldea    s1, s1, vl*4
    bnz     s2, clear_bss_loop
bss_cleared:


    ; ------------------------------------------------------------------------
    ; Set up the stack area.
    ; ------------------------------------------------------------------------

    ; Initialize the stack: Place the stack at the top of VRAM.
    ; TODO(m): Use memory allocation for this instead.
    ; TODO(m): Set up the thread and frame pointers too.
    ldhi    s1, #MMIO_START
    ldw     s1, s1, #VRAMSIZE
    ldhi    sp, #VRAM_START
    add     sp, sp, s1


    ; ------------------------------------------------------------------------
    ; Initialize the video console.
    ; ------------------------------------------------------------------------

    jl      pc, #vcon_memory_requirement@pc
    mov     s21, s1                 ; s21 = vcon size

    ldhi    s2, #MMIO_START
    ldw     s2, s2, #VRAMSIZE
    ldhi    s3, #VRAM_START
    add     s3, s3, s2              ; s3 = VRAM end

    ldi     s2, #STACK_SIZE
    add     s2, s1, s2              ; s2 = stack size + vconsole size
    sub     s20, s3, s2             ; s20 = start of vcon (end of free VRAM)

    mov     s1, s20
    jl      pc, #vcon_init@pc


    ; ------------------------------------------------------------------------
    ; Boot text: Print some memory information etc.
    ; ------------------------------------------------------------------------

    ldea    s1, pc, #boot_text_1@pc
    jl      pc, #vcon_print@pc

    ldea    s1, pc, #vram_text_1@pc
    ldhi    s2, #VRAM_START
    ldhi    s3, #MMIO_START
    ldw     s3, s3, #VRAMSIZE
    jl      pc, #print_mem_info@pc

    ldea    s1, pc, #xram_text_1@pc
    ldhi    s2, #XRAM_START
    ldhi    s3, #MMIO_START
    ldw     s3, s3, #XRAMSIZE
    jl      pc, #print_mem_info@pc

    ldea    s1, pc, #bss_text_1@pc
    ldhi    s2, #__bss_start@hi
    or      s2, s2, #__bss_start@lo
    ldhi    s3, #__bss_size@hi
    or      s3, s3, #__bss_size@lo
    jl      pc, #print_mem_info@pc

    ldea    s1, pc, #vcon_text_1@pc
    mov     s2, s20
    mov     s3, s21
    jl      pc, #print_mem_info@pc

    ldea    s1, pc, #stack_text_1@pc
    ldi     s3, #STACK_SIZE
    mov     s2, sp
    sub     s2, s2, s3
    jl      pc, #print_mem_info@pc

    ldea    s1, pc, #boot_text_2@pc
    jl      pc, #vcon_print@pc


    ; ------------------------------------------------------------------------
    ; Initialize the memory allocator.
    ; ------------------------------------------------------------------------

    jl      pc, #mem_init@pc

    ; Add a memory allocation pool for the XRAM.
    ; Note: By adding this pool first, we give it the highest priority. This
    ; means that if anyone calls mem_alloc() with MEM_TYPE_ANY, the allocator
    ; will try to allocate XRAM first.
    ldhi    s1, #XRAM_START@hi
    or      s1, s1, #XRAM_START@lo
    ldhi    s2, #MMIO_START
    ldw     s2, s2, #XRAMSIZE
    ldi     s3, #MEM_TYPE_EXT
    jl      pc, #mem_add_pool@pc

    ; Add a memory allocation pool for the VRAM.
    ldhi    s1, #__vram_free_start@hi
    or      s1, s1, #__vram_free_start@lo   ; s1 = Start of free VRAM
    sub     s2, s20, s1                     ; s2 = Number of free VRAM bytes
    ldi     s3, #MEM_TYPE_VIDEO             ; s3 = The memory type.
    jl      pc, #mem_add_pool@pc


    ; ------------------------------------------------------------------------
    ; Clear all CPU registers.
    ; ------------------------------------------------------------------------

    ; Set all the scalar registers (except Z, SP and PC) to a known state.
    ldi     s1, #0
    ldi     s2, #0
    ldi     s3, #0
    ldi     s4, #0
    ldi     s5, #0
    ldi     s6, #0
    ldi     s7, #0
    ldi     s8, #0
    ldi     s9, #0
    ldi     s10, #0
    ldi     s11, #0
    ldi     s12, #0
    ldi     s13, #0
    ldi     s14, #0
    ldi     s15, #0
    ldi     s16, #0
    ldi     s17, #0
    ldi     s18, #0
    ldi     s19, #0
    ldi     s20, #0
    ldi     s21, #0
    ldi     s22, #0
    ldi     s23, #0
    ldi     s24, #0
    ldi     s25, #0
    ldi     fp, #0
    ldi     tp, #0
    ldi     vl, #0
    ldi     lr, #0

    ; Set all the vector registers to a known state: clear all elements.
    ; Also: The default vector length is the max vector register length.
    cpuid   vl, z, z
    or      v1, vz, #0
    or      v2, vz, #0
    or      v3, vz, #0
    or      v4, vz, #0
    or      v5, vz, #0
    or      v6, vz, #0
    or      v7, vz, #0
    or      v8, vz, #0
    or      v9, vz, #0
    or      v10, vz, #0
    or      v11, vz, #0
    or      v12, vz, #0
    or      v13, vz, #0
    or      v14, vz, #0
    or      v15, vz, #0
    or      v16, vz, #0
    or      v17, vz, #0
    or      v18, vz, #0
    or      v19, vz, #0
    or      v20, vz, #0
    or      v21, vz, #0
    or      v22, vz, #0
    or      v23, vz, #0
    or      v24, vz, #0
    or      v25, vz, #0
    or      v26, vz, #0
    or      v27, vz, #0
    or      v28, vz, #0
    or      v29, vz, #0
    or      v30, vz, #0
    or      v31, vz, #0


    ; ------------------------------------------------------------------------
    ; Call main().
    ; ------------------------------------------------------------------------

    ; s1 = argc
    ldi     s1, #1

    ; s2 = argv
    ldhi    s2, #argv@hi
    add     s2, s2, #argv@lo

    ; Jump to main().
    jl      pc, #main@pc


    ; ------------------------------------------------------------------------
    ; Terminate the program.
    ; ------------------------------------------------------------------------

    ; We use extra nop:s to flush the pipeline.
    nop
    nop
    nop
    nop
    nop

    ; Loop forever...
1$:
    j       pc, #1$@pc

    nop
    nop
    nop
    nop
    nop


    ; ------------------------------------------------------------------------
    ; Print memory area info.
    ; s1 = text
    ; s2 = mem start
    ; s3 = mem size
    ; ------------------------------------------------------------------------

print_mem_info:
    add     sp, sp, #-12
    stw     lr, sp, #0
    stw     s20, sp, #4
    stw     s21, sp, #8

    mov     s20, s2
    mov     s21, s3

    jl      pc, #vcon_print@pc

    mov     s1, s20
    jl      pc, #vcon_print_hex@pc

    ldea    s1, pc, #mem_info_text_2@pc
    jl      pc, #vcon_print@pc

    mov     s1, s21
    jl      pc, #vcon_print_dec@pc

    ldea    s1, pc, #mem_info_text_3@pc
    jl      pc, #vcon_print@pc

    ldw     lr, sp, #0
    ldw     s20, sp, #4
    ldw     s21, sp, #8
    add     sp, sp, #12
    j       lr


    .section .rodata

    .p2align 2
argv:
    .word   arg0

arg0:
    ; We provide a fake program name (just to have a valid call to main).
    .asciz  "program"

boot_text_1:
    .asciz  "MC1: An MRISC32 computer\n"

boot_text_2:
    .ascii  "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
    .ascii  "abcdefghijklmnopqrstuvwxyz\n"
    .ascii  "0123456789\n"
    .ascii  ",.!?\"#$%&()[]{}<>=+-*/|\\~\n"
    .ascii  "A\tB\tC\tD\n"
    .byte   0

vram_text_1:
    .asciz  "VRAM:  0x"
xram_text_1:
    .asciz  "XRAM:  0x"
bss_text_1:
    .asciz  "BSS:   0x"
vcon_text_1:
    .asciz  "VCON:  0x"
stack_text_1:
    .asciz  "Stack: 0x"
mem_info_text_2:
    .asciz  ", "
mem_info_text_3:
    .asciz  " bytes\n"
