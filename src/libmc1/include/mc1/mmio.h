// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#ifndef MC1_MMIO_H_
#define MC1_MMIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MMIO registers.
#define CLKCNTLO   0
#define CLKCNTHI   4
#define CPUCLK     8
#define VRAMSIZE   12
#define XRAMSIZE   16
#define VIDWIDTH   20
#define VIDHEIGHT  24
#define VIDFPS     28
#define VIDFRAMENO 32
#define VIDY       36
#define SWITCHES   40
#define BUTTONS    44
#define KEYPTR     48
#define MOUSEPOS   52
#define MOUSEBTNS  56
#define SDIN       60
#define SEGDISP0   64
#define SEGDISP1   68
#define SEGDISP2   72
#define SEGDISP3   76
#define SEGDISP4   80
#define SEGDISP5   84
#define SEGDISP6   88
#define SEGDISP7   92
#define LEDS       96
#define SDOUT      100
#define SDWE       104

// Macro for accessing MMIO registers.
#ifdef __cplusplus
#define MMIO(reg) \
  *reinterpret_cast<volatile uint32_t*>(&reinterpret_cast<volatile uint8_t*>(0xc0000000)[reg])
#else
#define MMIO(reg) *(volatile uint32_t*)(&((volatile uint8_t*)0xc0000000)[reg])
#endif

// Macro for reading the key event buffer.
// The key event buffer is a 16-entry circular buffer (each entry is a 32-bit word), starting at
// 0xc00080.
#ifdef __cplusplus
#define KEYBUF(ptr) \
  (reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(0xc0000080)))[ptr]
#else
#define KEYBUF(ptr) ((volatile uint32_t*)(((volatile uint8_t*)0xc0000080)))[ptr]
#endif

// Number of entires in the key event buffer.
#define KEYBUF_SIZE 16

// SPI SD card I/O bits (SDIN, SDOUT, SDWE).
#define SD_MISO_BIT_NO 0
#define SD_MISO_BIT    (1 << SD_MISO_BIT_NO)
#define SD_CS_BIT_NO   3
#define SD_CS_BIT      (1 << SD_CS_BIT_NO)
#define SD_MOSI_BIT_NO 4
#define SD_MOSI_BIT    (1 << SD_MOSI_BIT_NO)
#define SD_SCK_BIT_NO  5
#define SD_SCK_BIT     (1 << SD_SCK_BIT_NO)

// SD mode SD card I/O bits (SDIN, SDOUT, SDWE).
#define SD_DAT0_BIT_NO 0
#define SD_DAT0_BIT    (1 << SD_DAT0_BIT_NO)
#define SD_DAT1_BIT_NO 1
#define SD_DAT1_BIT    (1 << SD_DAT1_BIT_NO)
#define SD_DAT2_BIT_NO 2
#define SD_DAT2_BIT    (1 << SD_DAT2_BIT_NO)
#define SD_DAT3_BIT_NO 3
#define SD_DAT3_BIT    (1 << SD_DAT3_BIT_NO)
#define SD_CMD_BIT_NO  4
#define SD_CMD_BIT     (1 << SD_CMD_BIT_NO)
#define SD_CLK_BIT_NO  5
#define SD_CLK_BIT     (1 << SD_CLK_BIT_NO)

#ifdef __cplusplus
}
#endif

#endif  // MC1_MMIO_H_

