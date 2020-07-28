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

#ifndef MC1_KEYBOARD_H_
#define MC1_KEYBOARD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scancode constants.
#define KB_BACKSPACE 0x66
#define KB_SPACE 0x29
#define KB_LSHIFT 0x12
#define KB_LCTRL 0x14
#define KB_LALT 0x11
#define KB_RSHIFT 0x59
#define KB_RCTRL 0x114
#define KB_RALT 0x111
#define KB_ENTER 0x5a
#define KB_ESC 0x76
#define KB_F1 0x05
#define KB_F2 0x06
#define KB_F3 0x04
#define KB_F4 0x0c
#define KB_F5 0x03
#define KB_F6 0x0b
#define KB_F7 0x83
#define KB_F8 0x0a
#define KB_F9 0x01
#define KB_F10 0x09
#define KB_F11 0x78
#define KB_F12 0x07

#define KB_INSERT 0x170
#define KB_HOME 0x16c
#define KB_PGUP 0x17d
#define KB_DEL 0x171
#define KB_END 0x169
#define KB_PGDN 0x17a
#define KB_UP 0x175
#define KB_LEFT 0x16b
#define KB_DOWN 0x172
#define KB_RIGHT 0x174

// Keyboard layout identifiers.
#define KB_LAYOUT_EN_US 0x0001  ///< English (US).

/// @brief Initialize the keyboard driver.
void kb_init();

/// @brief Poll for new keyboard events.
/// @note Call this frequently.
void kb_poll();

/// @brief Set the keyboard layout.
/// @param layout_id The keyboard layout identifier.
void kb_set_layout(const uint32_t layout_id);

/// @brief Get the next keyboard event, if any.
/// @returns The keyboard event in the form of an unsigned value.
/// @note Use the @c kb_event_* macros to decode a keyboard event.
uint32_t kb_get_next_event();

/// @brief Convert an event to a character.
/// @param event The keyboard event (e.g. returned by @c kb_get_next_event()).
/// @returns The keyboard character in Latin-1 encoding, or zero if the event did not correspond
/// to a character.
uint32_t kb_event_to_char(const uint32_t event);

/// @brief Check if a key is currently held down.
/// @param scancode The scancode of the key to be checked.
/// @returns a non-zero value if the key is currently held down.
int kb_is_pressed(const uint32_t scancode);

// Helper macros for decoding an event.
#define kb_event_scancode(event) ((event)&0x01ffu)
#define kb_event_is_press(event) (((event)&0x0200u) == 0u)
#define kb_event_is_release(event) (((event)&0x0200u) != 0u)
#define kb_event_has_shift(event) (((event)&0x0400u) != 0u)
#define kb_event_has_alt(event) (((event)&0x0800u) != 0u)
#define kb_event_has_ctrl(event) (((event)&0x1000u) != 0u)

#ifdef __cplusplus
}
#endif

#endif  // MC1_KEYBOARD_H_
