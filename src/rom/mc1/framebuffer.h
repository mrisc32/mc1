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

#ifndef MC1_FRAMEBUFFER_H_
#define MC1_FRAMEBUFFER_H_

#include <stddef.h>
#include <stdint.h>

typedef enum {
  MODE_RGBA8888 = 0,
  MODE_RGBA5551 = 1,
  MODE_PAL8 = 2,
  MODE_PAL4 = 3,
  MODE_PAL2 = 4,
  MODE_PAL1 = 5
} color_mode_t;

typedef struct {
  void* pixels;
  void* vcp;
  uint32_t* palette;
  size_t stride;
  int width;
  int height;
  color_mode_t mode;
} fb_t;

/// @brief Create a new framebuffer.
/// @param width The width of the framebuffer.
/// @param height The height of the framebuffer.
/// @param mode The color mode (see @c color_mode_t).
/// @returns a framebuffer object, or NULL if the framebuffer could not be
/// created.
fb_t* fb_create(int width, int height, color_mode_t mode);

/// @brief Free a framebuffer and associated memory.
/// @param fb The framebuffer object.
void fb_destroy(fb_t* fb);

/// @brief Show the framebuffer (i.e. make it current).
/// @param fb The framebuffer object.
void fb_show(fb_t* fb);

#endif // MC1_FRAMEBUFFER_H_

