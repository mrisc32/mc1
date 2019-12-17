; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: Standard C library routines
; ----------------------------------------------------------------------------

.include "system/memory.inc"


    .text

; ----------------------------------------------------------------------------
; void* malloc(size_t size)
;  s1 = size
; ----------------------------------------------------------------------------

    .globl  malloc
    .p2align 2

malloc:
    ldi     s2, #MEM_TYPE_ANY
    j       pc, #mem_alloc@pc


; ----------------------------------------------------------------------------
; void free(void* ptr)
;  s1 = ptr
; ----------------------------------------------------------------------------

    .globl  free
    .p2align 2

free:
    j       pc, #mem_free@pc


; ----------------------------------------------------------------------------
; void* memset(void* ptr, int value, size_t num)
;  s1 = ptr
;  s2 = value
;  s3 = num
; ----------------------------------------------------------------------------

    .globl  memset
    .p2align 2

memset:
    j       pc, #mem_fill@pc


; ----------------------------------------------------------------------------
; void* memcpy(void* destination, const void* source, size_t num)
;  s1 = destination
;  s2 = source
;  s3 = num
; ----------------------------------------------------------------------------

    .globl  memcpy
    .p2align 2

memcpy:
    j       pc, #mem_copy_fwd@pc

