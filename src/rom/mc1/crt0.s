; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the common startup code. It defines _start, which does
; some initialization and then calls main.
; ----------------------------------------------------------------------------

.include "mc1/memory.inc"
.include "mc1/mmio.inc"

STACK_SIZE = 4*1024


    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Clear the BSS data.
    ; ------------------------------------------------------------------------

    ldi     s2, #__bss_size
    bz      s2, bss_cleared
    lsr     s2, s2, #2      ; BSS size is always a multiple of 4 bytes.

    ldi     s1, #__bss_start
    cpuid   s3, z, z
clear_bss_loop:
    min     vl, s2, s3
    sub     s2, s2, vl
    stw     vz, s1, #4
    ldea    s1, s1, vl*4
    bnz     s2, clear_bss_loop
bss_cleared:


    ; ------------------------------------------------------------------------
    ; Set up the stack area.
    ; ------------------------------------------------------------------------

    ; Initialize the stack: Place the stack at the top of VRAM.
    ldhi    s1, #MMIO_START
    ldw     s1, s1, #VRAMSIZE
    ldhi    sp, #VRAM_START
    add     sp, sp, s1


    ; ------------------------------------------------------------------------
    ; Initialize the video console.
    ; ------------------------------------------------------------------------

    bl      vcon_memory_requirement
    mov     s21, s1                 ; s21 = vcon size

    ldhi    s2, #MMIO_START
    ldw     s2, s2, #VRAMSIZE
    ldhi    s3, #VRAM_START
    add     s3, s3, s2              ; s3 = VRAM end

    ldi     s2, #STACK_SIZE
    add     s2, s1, s2              ; s2 = stack size + vconsole size
    sub     s20, s3, s2             ; s20 = start of vcon (end of free VRAM)

    mov     s1, s20
    bl      vcon_init


    ; ------------------------------------------------------------------------
    ; Boot text: Print some memory information etc.
    ; ------------------------------------------------------------------------

    ldi     s1, #boot_text_1@pc
    bl      vcon_print

    ldi     s1, #rom_text_1@pc
    ldi     s2, #ROM_START
    ldi     s3, #__rom_size
    bl      print_mem_info

    ldi     s1, #vram_text_1@pc
    ldi     s2, #VRAM_START
    ldi     s3, #MMIO_START
    ldw     s3, s3, #VRAMSIZE
    bl      print_mem_info

    ldi     s1, #bss_text_1@pc
    ldi     s2, #__bss_start
    ldi     s3, #__bss_size
    bl      print_mem_info

    ldi     s1, #vcon_text_1@pc
    mov     s2, s20
    mov     s3, s21
    bl      print_mem_info

    ldi     s1, #stack_text_1@pc
    ldi     s3, #STACK_SIZE
    mov     s2, sp
    sub     s2, s2, s3
    bl      print_mem_info

    ldi     s1, #xram_text_1@pc
    ldi     s2, #XRAM_START
    ldi     s3, #MMIO_START
    ldw     s3, s3, #XRAMSIZE
    bl      print_mem_info


    ; ------------------------------------------------------------------------
    ; Run the selftest.
    ; ------------------------------------------------------------------------

    ldi     s1, #selftest_text@pc
    bl      vcon_print
    ldi     s1, #selftest_callback@pc
    bl      selftest_run

    ; s1 contains the pass/fail status (pass = all bits set).
    ldi     s2, #selftest_pass_text@pc
    bs      s1, 1f
    ldi     s2, #selftest_fail_text@pc
1:
    mov     s1, s2
    bl      vcon_print


    ; ------------------------------------------------------------------------
    ; Initialize the memory allocator.
    ; ------------------------------------------------------------------------

    bl      mem_init

    ; Add a memory allocation pool for the XRAM.
    ; Note: By adding this pool first, we give it the highest priority. This
    ; means that if anyone calls mem_alloc() with MEM_TYPE_ANY, the allocator
    ; will try to allocate XRAM first.
    ldi     s1, #XRAM_START
    ldi     s2, #MMIO_START
    ldw     s2, s2, #XRAMSIZE
    ldi     s3, #MEM_TYPE_EXT
    bl      mem_add_pool

    ; Add a memory allocation pool for the VRAM.
    ldi     s1, #__vram_free_start  ; s1 = Start of free VRAM
    sub     s2, s20, s1             ; s2 = Number of free VRAM bytes
    ldi     s3, #MEM_TYPE_VIDEO     ; s3 = The memory type
    bl      mem_add_pool


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
    ldi     s2, #argv@pc

    ; Jump to main().
    call    #main@pc


    ; ------------------------------------------------------------------------
    ; Terminate the program.
    ; ------------------------------------------------------------------------

    ; Loop forever...
1$:
    b       1$


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

    bl      vcon_print

    mov     s1, s20
    bl      vcon_print_hex

    ldi     s1, #mem_info_text_2@pc
    bl      vcon_print

    mov     s1, s21
    bl      vcon_print_dec

    ldi     s1, #mem_info_text_3@pc
    bl      vcon_print

    ldw     lr, sp, #0
    ldw     s20, sp, #4
    ldw     s21, sp, #8
    add     sp, sp, #12
    ret


    ; ------------------------------------------------------------------------
    ; Callback routine for the selftest.
    ; s1 = pass/fail (-1/0)
    ; s2 = test no.
    ; ------------------------------------------------------------------------

selftest_callback:
    ldi     s3, #33
    and     s4, s1, #9
    add     s1, s3, s4      ; s1 = "*" for pass, "!" for fail
    b       vcon_putc


    .section .rodata

    .p2align 2
argv:
    .word   arg0

arg0:
    ; We provide a fake program name (just to have a valid call to main).
    .asciz  "program"

boot_text_1:
    .asciz  "MC1 - The MRISC32 computer\n\n"

rom_text_1:
    .asciz  "ROM:     0x"
vram_text_1:
    .asciz  "VRAM:    0x"
bss_text_1:
    .asciz  "  BSS:   0x"
vcon_text_1:
    .asciz  "  VCON:  0x"
stack_text_1:
    .asciz  "  Stack: 0x"
xram_text_1:
    .asciz  "XRAM:    0x"
mem_info_text_2:
    .asciz  ", "
mem_info_text_3:
    .asciz  " bytes\n"

selftest_text:
    .asciz  "\nSelftest: "
selftest_pass_text:
    .asciz  " PASS\n\n"
selftest_fail_text:
    .asciz  " FAIL\n\n"

