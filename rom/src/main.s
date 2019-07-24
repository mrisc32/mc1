; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the common startup code. It defines _start, which does
; some initialization and then calls main.
; ----------------------------------------------------------------------------

.include "config.inc"

    .text

    .globl  main

main:
    ; TODO(m): Implement me!
    j       lr

