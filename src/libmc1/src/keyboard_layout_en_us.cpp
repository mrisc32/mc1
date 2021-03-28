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

#include <mc1/keyboard_layout.h>

const keyboard_layout_t g_kb_layout_en_us = {
    {0, 0},        // 0x00
    {0, 0},        // 0x01 (f9)
    {0, 0},        //
    {0, 0},        // 0x03 (f5)
    {0, 0},        // 0x04 (f3)
    {0, 0},        // 0x05 (f1)
    {0, 0},        // 0x06 (f2)
    {0, 0},        // 0x07 (f12)
    {0, 0},        //
    {0, 0},        // 0x09 (f10)
    {0, 0},        // 0x0a (f8)
    {0, 0},        // 0x0b (f6)
    {0, 0},        // 0x0c (f4)
    {'\t', '\t'},  // 0x0d
    {'`', '~'},    // 0x0e
    {0, 0},        //
    {0, 0},        // 0x10
    {0, 0},        // 0x11 (lalt)
    {0, 0},        // 0x12 (lshift)
    {0, 0},        //
    {0, 0},        // 0x14 (lcrtl)
    {'q', 'Q'},    // 0x15
    {'1', '!'},    // 0x16
    {0, 0},        //
    {0, 0},        //
    {0, 0},        //
    {'z', 'Z'},    // 0x1a
    {'s', 'S'},    // 0x1b
    {'a', 'A'},    // 0x1c
    {'w', 'W'},    // 0x1d
    {'2', '@'},    // 0x1e
    {0, 0},        //
    {0, 0},        // 0x20
    {'c', 'C'},    // 0x21
    {'x', 'X'},    // 0x22
    {'d', 'D'},    // 0x23
    {'e', 'E'},    // 0x24
    {'4', '$'},    // 0x25
    {'3', '#'},    // 0x26
    {0, 0},        //
    {0, 0},        //
    {' ', ' '},    // 0x29
    {'v', 'V'},    // 0x2a
    {'f', 'F'},    // 0x2b
    {'t', 'T'},    // 0x2c
    {'r', 'R'},    // 0x2d
    {'5', '%'},    // 0x2e
    {0, 0},        //
    {0, 0},        // 0x30
    {'n', 'N'},    // 0x31
    {'b', 'B'},    // 0x32
    {'h', 'H'},    // 0x33
    {'g', 'G'},    // 0x34
    {'y', 'Y'},    // 0x35
    {'6', '^'},    // 0x36
    {0, 0},        //
    {0, 0},        //
    {0, 0},        //
    {'m', 'M'},    // 0x3a
    {'j', 'J'},    // 0x3b
    {'u', 'U'},    // 0x3c
    {'7', '&'},    // 0x3d
    {'8', '*'},    // 0x3e
    {0, 0},        //
    {0, 0},        // 0x40
    {',', '<'},    // 0x41
    {'k', 'K'},    // 0x42
    {'i', 'I'},    // 0x43
    {'o', 'O'},    // 0x44
    {'0', ')'},    // 0x45
    {'9', '('},    // 0x46
    {0, 0},        //
    {0, 0},        //
    {'.', '>'},    // 0x49
    {'/', '?'},    // 0x4a
    {'l', 'L'},    // 0x4b
    {';', ':'},    // 0x4c
    {'p', 'P'},    // 0x4d
    {'-', '_'},    // 0x4e
    {0, 0},        //
    {0, 0},        // 0x50
    {0, 0},        //
    {'\'', '"'},   // 0x52
    {0, 0},        //
    {'[', '{'},    // 0x54
    {'=', '+'},    // 0x55
    {0, 0},        //
    {0, 0},        //
    {0, 0},        // 0x58 (caps)
    {0, 0},        // 0x59 (rshift)
    {10, 10},      // 0x5a (enter)
    {']', '}'},    // 0x5b
    {0, 0},        //
    {'\\', '|'},   // 0x5d
    {0, 0},        //
    {0, 0},        //
    {0, 0},        // 0x60
    {0, 0},        //
    {0, 0},        //
    {0, 0},        //
    {0, 0},        //
    {0, 0},        //
    {0, 0},        // 0x66 (backspace)
    {0, 0},        //
    {0, 0},        //
    {'1', '1'},    // 0x69 (keypad)
    {0, 0},        //
    {'4', '4'},    // 0x6b (keypad)
    {'7', '7'},    // 0x6c (keypad)
    {'/', '/'},    // 0x6d (keypad) - Mapped from PS/2 scancode e0, 4a
    {10, 10},      // 0x6e (keypad) - Mapped from PS/2 scancode e0, 5a
    {0, 0},        //
    {'0', '0'},    // 0x70 (keypad)
    {'.', '.'},    // 0x71 (keypad)
    {'2', '2'},    // 0x72 (keypad)
    {'5', '5'},    // 0x73 (keypad)
    {'6', '6'},    // 0x74 (keypad)
    {'8', '8'},    // 0x75 (keypad)
    {27, 27},      // 0x76 (esc)
    {0, 0},        // 0x77 (numlock)
    {0, 0},        // 0x78 (f11)
    {'+', '+'},    // 0x79 (keypad)
    {'3', '3'},    // 0x7a (keypad)
    {'-', '-'},    // 0x7b (keypad)
    {'*', '*'},    // 0x7c (keypad)
    {'9', '9'},    // 0x7d (keypad)
    {0, 0},        // 0x7e (scrolllock)
    {0, 0},        //
};
