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

#ifndef SYSTEM_MMIO_H_
#define SYSTEM_MMIO_H_

#include <system/types.h>

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
#define SEGDISP0   64
#define SEGDISP1   68
#define SEGDISP2   72
#define SEGDISP3   76
#define SEGDISP4   80
#define SEGDISP5   84
#define SEGDISP6   88
#define SEGDISP7   92
#define LEDS       96

// Macro for accessing MMIO registers.
#define MMIO(reg) *(volatile uint32_t*)(&((volatile uint8_t*)0xc0000000)[reg])

#endif // SYSTEM_MMIO_H_


