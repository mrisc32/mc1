// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
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

#ifndef MC1_VCONSOLE_H_
#define MC1_VCONSOLE_H_

#include <mc1/framebuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

// These are meant to be called from the ROM boot routine.
unsigned vcon_memory_requirement(void);
void vcon_init(void* addr);

// Public API.
void vcon_show(layer_t layer);
void vcon_clear();
void vcon_set_colors(unsigned col0, unsigned col1);
void vcon_print(const char* text);
void vcon_print_hex(unsigned x);
void vcon_print_dec(int x);
int vcon_putc(const int c);

#ifdef __cplusplus
}
#endif

#endif  // MC1_VCONSOLE_H_

