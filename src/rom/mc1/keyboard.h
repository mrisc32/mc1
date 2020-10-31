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
// clang-format off
#define KB_A                0x01c
#define KB_B                0x032
#define KB_C                0x021
#define KB_D                0x023
#define KB_E                0x024
#define KB_F                0x02b
#define KB_G                0x034
#define KB_H                0x033
#define KB_I                0x043
#define KB_J                0x03b
#define KB_K                0x042
#define KB_L                0x04b
#define KB_M                0x03a
#define KB_N                0x031
#define KB_O                0x044
#define KB_P                0x04d
#define KB_Q                0x015
#define KB_R                0x02d
#define KB_S                0x01b
#define KB_T                0x02c
#define KB_U                0x03c
#define KB_V                0x02a
#define KB_W                0x01d
#define KB_X                0x022
#define KB_Y                0x035
#define KB_Z                0x01a
#define KB_0                0x045
#define KB_1                0x016
#define KB_2                0x01e
#define KB_3                0x026
#define KB_4                0x025
#define KB_5                0x02e
#define KB_6                0x036
#define KB_7                0x03d
#define KB_8                0x03e
#define KB_9                0x046

#define KB_SPACE            0x029
#define KB_BACKSPACE        0x066
#define KB_TAB              0x00d
#define KB_LSHIFT           0x012
#define KB_LCTRL            0x014
#define KB_LALT             0x011
#define KB_LMETA            0x11f
#define KB_RSHIFT           0x059
#define KB_RCTRL            0x114
#define KB_RALT             0x111
#define KB_RMETA            0x127
#define KB_ENTER            0x05a
#define KB_ESC              0x076
#define KB_F1               0x005
#define KB_F2               0x006
#define KB_F3               0x004
#define KB_F4               0x00c
#define KB_F5               0x003
#define KB_F6               0x00b
#define KB_F7               0x083
#define KB_F8               0x00a
#define KB_F9               0x001
#define KB_F10              0x009
#define KB_F11              0x078
#define KB_F12              0x007

#define KB_INSERT           0x170
#define KB_HOME             0x16c
#define KB_DEL              0x171
#define KB_END              0x169
#define KB_PGUP             0x17d
#define KB_PGDN             0x17a
#define KB_UP               0x175
#define KB_LEFT             0x16b
#define KB_DOWN             0x172
#define KB_RIGHT            0x174

#define KB_KP_0             0x070
#define KB_KP_1             0x069
#define KB_KP_2             0x072
#define KB_KP_3             0x07a
#define KB_KP_4             0x06b
#define KB_KP_5             0x073
#define KB_KP_6             0x074
#define KB_KP_7             0x06c
#define KB_KP_8             0x075
#define KB_KP_9             0x07d
#define KB_KP_PERIOD        0x071
#define KB_KP_PLUS          0x079
#define KB_KP_MINUS         0x07b
#define KB_KP_MUL           0x07c
#define KB_KP_DIV           0x06d
#define KB_KP_ENTER         0x06e

#define KB_ACPI_POWER       0x137
#define KB_ACPI_SLEEP       0x13f
#define KB_ACPI_WAKE        0x15e

#define KB_MM_NEXT_TRACK    0x14d
#define KB_MM_PREV_TRACK    0x115
#define KB_MM_STOP          0x13b
#define KB_MM_PLAY_PAUSE    0x134
#define KB_MM_MUTE          0x123
#define KB_MM_VOL_UP        0x132
#define KB_MM_VOL_DOWN      0x121
#define KB_MM_MEDIA_SEL     0x150
#define KB_MM_EMAIL         0x148
#define KB_MM_CALCULATOR    0x12b
#define KB_MM_MY_COMPUTER   0x140

#define KB_WWW_SEARCH       0x110
#define KB_WWW_HOME         0x13a
#define KB_WWW_BACK         0x138
#define KB_WWW_FOWRARD      0x130
#define KB_WWW_STOP         0x128
#define KB_WWW_REFRESH      0x120
#define KB_WWW_FAVORITES    0x118
// clang-format on

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
