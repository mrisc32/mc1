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
    ldi     r1, #0
    ldi     r2, #0
    ldi     r3, #0
    ldi     r4, #0
    ldi     r5, #0
    ldi     r6, #0
    ldi     r7, #0
    ldi     r8, #0
    ldi     r9, #0
    ldi     r10, #0
    ldi     r11, #0
    ldi     r12, #0
    ldi     r13, #0
    ldi     r14, #0
    ldi     r15, #0
    ldi     r16, #0
    ldi     r17, #0
    ldi     r18, #0
    ldi     r19, #0
    ldi     r20, #0
    ldi     r21, #0
    ldi     r22, #0
    ldi     r23, #0
    ldi     r24, #0
    ldi     r25, #0
    ldi     r26, #0
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

    ldi     r1, #MMIO_START
    ldw     r1, [r1, #VRAMSIZE]
    ldi     r25, #VRAM_START
    add     r25, r25, r1                ; r25 = Top of VRAM
    add     r24, r25, #-BOOT_CODE_SIZE  ; r24 = Start of boot code area
    add     r23, r24, #-HEAP_SIZE       ; r23 = Start of heap
    add     r22, r23, #-STACK_SIZE      ; r22 = Bottom of stack

    mov     sp, r23                     ; sp = Top of stack


    ; ------------------------------------------------------------------------
    ; Clear the BSS data (if any).
    ; ------------------------------------------------------------------------

    ldi     r2, #__bss_size
    bz      r2, bss_cleared
    lsr     r2, r2, #2      ; BSS size is always a multiple of 4 bytes.

    ldi     r1, #__bss_start
    cpuid   r3, z, z
clear_bss_loop:
    minu    vl, r2, r3
    sub     r2, r2, vl
    stw     vz, [r1, #4]
    ldea    r1, [r1, vl*4]
    bnz     r2, clear_bss_loop
bss_cleared:


    ; ------------------------------------------------------------------------
    ; Make both video layers "silent" (use no memory cycles).
    ; We also set the background color for both layers, since the content of
    ; the palette registers is undefined after reset.
    ; ------------------------------------------------------------------------

    ldi     r1, #0x60000000     ; SETPAL 0, 1
    ldi     r2, #0x00000000     ; Color 0 = fully transparent black
    ldi     r3, #0x50007fff     ; WAITY 32767 = wait forever
    ldi     r4, #VRAM_START
    stw     r1, [r4, #16]       ; Layer 1 VCP
    stw     r2, [r4, #20]
    stw     r3, [r4, #24]
    stw     r1, [r4, #32]       ; Layer 2 VCP
    stw     r2, [r4, #36]
    stw     r3, [r4, #40]


    ; ------------------------------------------------------------------------
    ; Try to boot from an SD card.
    ; ------------------------------------------------------------------------

    ; Initilize the SD card.
    ldea    r1, [r23, #HEAP_SDCTX]  ; sdctx_t
    ldi     r2, #0                  ; No logging function
    bl      sdcard_init
    bz      r1, bootloader_failed

    ; Load the boot code block.
    ldea    r1, [r23, #HEAP_SDCTX]  ; sdctx_t
    mov     r2, r24                 ; Load to start of boot code area in VRAM
    ldi     r3, #0                  ; Load the first block (block #0)
    ldi     r4, #1                  ; Load one block (512 bytes)
    bl      sdcard_read
    bz      r1, bootloader_failed

    ; Is the magic ID correct?
    ldw     r1, [r24]               ; r1 = magic ID
    ldi     r2, #0x4231434d
    seq     r1, r1, r2
    bns     r1, bootloader_failed

    ; Is the checksum correct?
    ldea    r1, [r24, #8]           ; r1 = start of code
    ldi     r2, #BOOT_CODE_SIZE-8
    bl      crc32c
    ldw     r2, [r24, #4]           ; r2 = expected checksum
    seq     r1, r1, r2
    bns     r1, bootloader_failed

    ; Call the boot code (never return).
    ldi     r1, #rom_jump_table@pc  ; r1 = rom_jump_table (arg 1)
    j       r24, #8

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
    ldi     r1, #XRAM_START
    ldi     r2, #MMIO_START
    ldw     r2, [r2, #XRAMSIZE]
    ldi     r3, #MEM_TYPE_EXT
    bl      mem_add_pool

    ; Add a memory allocation pool for the VRAM.
    ldi     r1, #__vram_free_start  ; r1 = Start of free VRAM
    sub     r2, r22, r1             ; r2 = Number of free VRAM bytes
    ldi     r3, #MEM_TYPE_VIDEO     ; r3 = The memory type
    bl      mem_add_pool


    ; ------------------------------------------------------------------------
    ; Call main().
    ; ------------------------------------------------------------------------

    ; r1 = argc, r2 = argv (these are invalid - don't use them!)
    ldi     r1, #0
    ldi     r2, #0

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
    bnz     r2, 1f

    ; Start address = ptr
    mov     r2, r1

    ; Calculate the address of the ROM owned sdctx_t.
    ldi     r1, #VRAM_START-BOOT_CODE_SIZE-HEAP_SIZE+HEAP_SDCTX
    ldi     r15, #MMIO_START
    ldw     r15, [r15, #VRAMSIZE]
    add     r1, r1, r15

    ; Tail-call sdcard_read(ctx, ptr, first_block, num_blocks).
    b       sdcard_read

1:
    ldi     r1, #0
    ret


    ; ------------------------------------------------------------------------
    ; ROM jump table for the boot code.
    ; ------------------------------------------------------------------------

rom_jump_table:
    b       doh         ; Offset =  0
    b       blk_read    ; Offset =  4
    b       crc32c      ; Offset =  8
    b       LZG_Decode  ; Offset = 12

