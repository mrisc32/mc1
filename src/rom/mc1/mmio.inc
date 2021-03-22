; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MMIO definitions.
; ----------------------------------------------------------------------------

; MMIO base address
MMIO_START = 0xc0000000

; MMIO registers
CLKCNTLO   = 0
CLKCNTHI   = 4
CPUCLK     = 8
VRAMSIZE   = 12
XRAMSIZE   = 16
VIDWIDTH   = 20
VIDHEIGHT  = 24
VIDFPS     = 28
VIDFRAMENO = 32
VIDY       = 36
SWITCHES   = 40
BUTTONS    = 44
KEYCODE    = 48
MOUSEPOS   = 52
MOUSEBTNS  = 56
SDIN       = 60
SEGDISP0   = 64
SEGDISP1   = 68
SEGDISP2   = 72
SEGDISP3   = 76
SEGDISP4   = 80
SEGDISP5   = 84
SEGDISP6   = 88
SEGDISP7   = 92
LEDS       = 96
SDOUT      = 100
SDWE       = 104

