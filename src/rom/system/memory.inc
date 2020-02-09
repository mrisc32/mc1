; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; Memory defintions
; ----------------------------------------------------------------------------

; Fixed memory areas
ROM_START     = 0x00000000
VRAM_START    = 0x40000000
XRAM_START    = 0x80000000

; Memory types and flags (must match memory.h)
MEM_TYPE_VIDEO  = 0x00000001
MEM_TYPE_EXT    = 0x00000002
MEM_TYPE_ANY    = 0x00000003
MEM_CLEAR       = 0x00000100
