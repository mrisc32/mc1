; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: mem_fill()
; ----------------------------------------------------------------------------

    .text

; ----------------------------------------------------------------------------
; void* mem_fill(void* ptr, int value, size_t num)
;  s1 = ptr
;  s2 = value
;  s3 = num
; ----------------------------------------------------------------------------

    .globl  mem_fill
    .p2align 2

mem_fill:
    mov     s6, vl          ; Preserve vl

    bz      s3, done        ; Nothing to do?

    ; We don't want to touch s1, as the function must return ptr.
    mov     s7, s1

    ; Start by filling up the vector regiser v1 with the fill value.
    cpuid   vl, z, z
    shuf    s2, s2, #0      ; Duplicate value to all four bytes of the word
    mov     v1, s2          ; Set all words of v1 to the value

    ; Is the target memory address aligned?
    and     s5, s7, #3
    bz      s5, aligned

    ; Make aligned (1-3 bytes).
    sub     s5, #4, s5      ; s5 = bytes until aligned
    min     vl, s3, s5
    sub     s3, s3, vl
    stb     v1, s7, #1
    ldea    s7, s7, vl
    bz      s3, done

aligned:
    cpuid   s4, z, z        ; s4 = max vector length
    lsr     s5, s3, #2
    bz      s5, tail
1$:
    min     vl, s5, s4
    sub     s5, s5, vl
    stw     v1, s7, #4
    ldea    s7, s7, vl*4
    bnz     s5, 1$

tail:
    and     vl, s3, #3      ; vl = tail length (0-3 bytes)
    stb     v1, s7, #1

done:
    mov     vl, s6          ; Restore vl
    j       lr

