// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
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

#ifndef MC1_KEYBOARD_LAYOUT_H_
#define MC1_KEYBOARD_LAYOUT_H_

#include <cstdint>

struct keyboard_layout_entry_t {
  uint8_t normal;
  uint8_t shifted;
  // TODO(m): Add more modifiers.
};

/// @brief A keyboard layout table.
///
/// A keyboard layout table esentially maps PS/2 scan codes (from PS/2 scan code set 2) to Latin 1
/// characters.
///
/// There are two exceptions, in order to make all printable scan codes fit in seven bits:
///   - Index 0x6d corresponds to PS/2 scan code e0, 4a
///   - Index 0x6e corresponds to PS/2 scan code e0, 5a
using keyboard_layout_t = keyboard_layout_entry_t[128];

// Defined keyboard layouts.
extern const keyboard_layout_t g_kb_layout_en_us;

#endif  // MC1_KEYBOARD_LAYOUT_H_
