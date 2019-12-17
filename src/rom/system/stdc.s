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
    mov     s6, vl          ; Preserve vl

    bz      s3, memset_done ; Nothing to do?

    ; Start by filling up the vector regiser v1 with the fill value.
    cpuid   vl, z, z
    shuf    s2, s2, #0      ; Duplicate value to all four bytes of the word
    mov     v1, s2          ; Set all words of v1 to the value

    ; Is the target memory address aligned?
    and     s5, s1, #3
    bz      s5, memset_aligned

    ; Make aligned (1-3 bytes).
    sub     s5, #4, s5      ; s5 = bytes until aligned
    min     vl, s3, s5
    sub     s3, s3, vl
    stb     v1, s1, #1
    ldea    s1, s1, vl
    bz      s3, memset_done

memset_aligned:
    cpuid   s4, z, z        ; s4 = max vector length
    lsr     s5, s3, #2
    bz      s5, memset_tail
1$:
    min     vl, s5, s4
    sub     s5, s5, vl
    stw     v1, s1, #4
    ldea    s1, s1, vl*4
    bnz     s5, 1$

memset_tail:
    and     vl, s3, #3      ; vl = tail length (0-3 bytes)
    stb     v1, s1, #1

memset_done:
    mov     vl, s6          ; Restore vl
    j       lr


; ----------------------------------------------------------------------------
; void* memcpy(void* destination, const void* source, size_t num)
;  s1 = destination
;  s2 = source
;  s3 = num
; ----------------------------------------------------------------------------

    .globl  memcpy
    .p2align 2

memcpy:
    ; Nothing to do?
    bz      s3, memcpy_exit

    mov     s5, vl          ; Preserve vl (it's a callee-saved register).
    mov     s4, s1          ; s4 = dest (we need to preserve s1)

    ; Is the length long enough to bother with optizations?
    sltu    s7, s3, #24
    bs      s7, memcpy_slow

    ; Are src and dest equally aligned (w.r.t 4-byte boundaries).
    and     s6, s4, #3
    and     s7, s2, #3
    seq     s7, s6, s7
    bns     s7, memcpy_slow        ; Use the slow case unless equally aligned.

    ; Do we need to align before the main loop?
    bz      s6, memcpy_aligned

    ; Align: Do a 1-3 bytes copy via a vector register, and adjust the memory
    ; pointers and the count.
    sub     vl, #4, s6      ; vl = bytes left until aligned.
    sub     s3, s3, vl
    ldb     v1, s2, #1
    add     s2, s2, vl
    stb     v1, s4, #1
    add     s4, s4, vl

memcpy_aligned:
    ; Vectorized word-copying loop.
    lsr     s7, s3, #2      ; s7 > 0 due to earlier length requirement.
    cpuid   s6, z, z        ; s6 = max vector length.
1$:
    min     vl, s6, s7
    sub     s7, s7, vl
    ldw     v1, s2, #4
    ldea    s2, s2, vl*4
    stw     v1, s4, #4
    ldea    s4, s4, vl*4
    bnz     s7, 1$

    ; Check how many bytes are remaining.
    and     vl, s3, #3      ; vl = bytes left after the aligned loop.
    bz      vl, memcpy_done

    ; Tail: Do a 1-3 bytes copy via a vector register.
    ldb     v1, s2, #1
    stb     v1, s4, #1

memcpy_done:
    ; Post vector-operation: Clear v1 (reg. length optimization).
    ldi     vl, #0
    or      v1, vz, #0

    mov     vl, s5          ; Restore vl.

memcpy_exit:
    ; At this point s1 should contain it's original value (dest).
    j       lr


memcpy_slow:
    ; Simple vectorized byte-copy loop (this is typically 4x slower than a
    ; word-copy loop).
    cpuid   s6, z, z        ; s6 = max vector length.
1$:
    min     vl, s6, s3
    sub     s3, s3, vl
    ldb     v1, s2, #1
    add     s2, s2, vl
    stb     v1, s4, #1
    add     s4, s4, vl
    bnz     s3, 1$

    b       memcpy_done

