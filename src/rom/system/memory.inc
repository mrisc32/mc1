; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; Memory defintions
; ----------------------------------------------------------------------------

; Memory areas

ROM_START     = 0x00000000
VRAM_START    = 0x40000000
XRAM_START    = 0x80000000

; Memory types (must match memory.h)
MEM_TYPE_VIDEO  = 0x00000001
MEM_TYPE_EXT    = 0x00000002
MEM_TYPE_ANY    = 0x00000003

