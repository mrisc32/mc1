; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the common startup code. It defines _start, which does
; some initialization and then calls main.
; ----------------------------------------------------------------------------

.include "mc1/memory.inc"
.include "mc1/mmio.inc"

; Size of the boot code block (including header).
BOOT_CODE_SIZE = 512

; Heap layout.
HEAP_SDCTX = 0      ; sdctx_t
HEAP_SIZE = 64

; Size of stack (only relevant for the memory allocator).
STACK_SIZE = 448

    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Clear all CPU registers.
    ; ------------------------------------------------------------------------

    ; Set all the scalar registers (except Z, SP and VL) to a known state.
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
    ldi     s26, #0
    ldi     tp, #0
    ldi     fp, #0
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
    ; Set up the stack and heap (total of 1 KiB at top of VRAM):
    ;
    ; Top of VRAM ->  +---------------------------------------------+
    ;                 | 512 bytes: Boot code (loaded from SD card)  |
    ;                 +---------------------------------------------+
    ;                 |  64 bytes: Heap (for SD card routines)      |
    ;                 +---------------------------------------------+
    ;                 | 448 bytes: Stack                            |
    ;                 +---------------------------------------------+
    ; ------------------------------------------------------------------------

    ldi     s1, #MMIO_START
    ldw     s1, s1, #VRAMSIZE
    ldi     s25, #VRAM_START
    add     s25, s25, s1                ; s25 = Top of VRAM
    add     s24, s25, #-BOOT_CODE_SIZE  ; s24 = Start of boot code area
    add     s23, s24, #-HEAP_SIZE       ; s23 = Start of heap
    add     s22, s23, #-STACK_SIZE      ; s22 = Bottom of stack

    mov     sp, s23                     ; sp = Top of stack


    ; ------------------------------------------------------------------------
    ; Clear the BSS data (if any).
    ; ------------------------------------------------------------------------

    ldi     s2, #__bss_size
    bz      s2, bss_cleared
    lsr     s2, s2, #2      ; BSS size is always a multiple of 4 bytes.

    ldi     s1, #__bss_start
    cpuid   s3, z, z
clear_bss_loop:
    minu    vl, s2, s3
    sub     s2, s2, vl
    stw     vz, s1, #4
    ldea    s1, s1, vl*4
    bnz     s2, clear_bss_loop
bss_cleared:


    ; ------------------------------------------------------------------------
    ; Make both video layers "silent" (use no memory cycles).
    ; ------------------------------------------------------------------------

    ldi     s1, #0x50007fff     ; WAITY 32767 = wait forever
    ldi     s2, #VRAM_START
    stw     s1, s2, #16         ; Layer 1 VCP
    stw     s1, s2, #32         ; Layer 2 VCP


    ; ------------------------------------------------------------------------
    ; Try to boot from an SD card.
    ; ------------------------------------------------------------------------

    ; Initilize the SD card.
    ldea    s1, s23, #HEAP_SDCTX    ; sdctx_t
    ldi     s2, #0                  ; No logging function
    bl      sdcard_init
    bz      s1, bootloader_failed

    ; Load the boot code block.
    ldea    s1, s23, #HEAP_SDCTX    ; sdctx_t
    mov     s2, s24                 ; Load to start of boot code area in VRAM
    ldi     s3, #0                  ; Load the first block (block #0)
    ldi     s4, #1                  ; Load one block (512 bytes)
    bl      sdcard_read
    bz      s1, bootloader_failed

    ; Is the magic ID correct?
    ldw     s1, s24, #0             ; s1 = magic ID
    ldi     s2, #0x4231434d
    seq     s1, s1, s2
    bns     s1, bootloader_failed

    ; Is the checksum correct?
    ldea    s1, s24, #8             ; s1 = start of code
    ldi     s2, #BOOT_CODE_SIZE-8
    bl      crc32c
    ldw     s2, s24, #4             ; s2 = expected checksum
    seq     s1, s1, s2
    bns     s1, bootloader_failed

    ; Call the boot code (never return).
    ldi     s1, #rom_jump_table@pc  ; s1 = rom_jump_table (arg 1)
    j       s24, #8

bootloader_failed:

.ifdef ENABLE_DEMO
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
    sub     s2, s22, s1             ; s2 = Number of free VRAM bytes
    ldi     s3, #MEM_TYPE_VIDEO     ; s3 = The memory type
    bl      mem_add_pool


    ; ------------------------------------------------------------------------
    ; Call main().
    ; ------------------------------------------------------------------------

    ; s1 = argc, s2 = argv (these are invalid - don't use them!)
    ldi     s1, #0
    ldi     s2, #0

    ; Jump to main().
    call    #main@pc
.endif

    ; Terminate the program: Loop forever...
1$:
    b       1$


    ; ------------------------------------------------------------------------
    ; int blk_read(void* ptr,
    ;              int device,
    ;              size_t first_block,
    ;              size_t num_blocks)
    ; ------------------------------------------------------------------------

blk_read:
    ; We currently only support device == 0
    bnz     s2, 1f

    ; Start address = ptr
    mov     s2, s1

    ; Calculate the address of the ROM owned sdctx_t.
    ldi     s1, #VRAM_START-BOOT_CODE_SIZE-HEAP_SIZE+HEAP_SDCTX
    ldi     s15, #MMIO_START
    ldw     s15, s15, #VRAMSIZE
    add     s1, s1, s15

    ; Tail-call sdcard_read(ctx, ptr, first_block, num_blocks).
    b       sdcard_read

1:
    ldi     s1, #0
    ret


    ; ------------------------------------------------------------------------
    ; ROM jump table for the boot code.
    ; ------------------------------------------------------------------------

rom_jump_table:
    b       doh         ; Offset =  0
    b       blk_read    ; Offset =  4
    b       crc32c      ; Offset =  8
    b       LZG_Decode  ; Offset = 12

