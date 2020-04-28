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

#include <mc1/framebuffer.h>

void set_mandelbrot_palette(fb_t* fb) {
  uint32_t* pal = fb->palette;

  // Color 0 is black.
  pal[0] = 0xff000000;

  // Generate colorful gradients for colors 1..255.
  int32_t r = 100;
  int32_t g = 50;
  int32_t b = 0;
  int32_t r_inc = 2;
  int32_t g_inc = 3;
  int32_t b_inc = 1;

  for (int k = 1; k <= 255; ++k) {
    pal[k] = 0xff000000u | (b << 16) | (g << 8) | r;

    // Update R, and if necessary adjust the increment value.
    r += r_inc;
    if (r < 0 || r > 255) {
      r_inc = -r_inc;
      r += r_inc;
    }

    // Update G, and if necessary adjust the increment value.
    g += g_inc;
    if (g < 0 || g > 255) {
      g_inc = -g_inc;
      g += g_inc;
    }

    // Update B, and if necessary adjust the increment value.
    b += b_inc;
    if (b < 0 || b > 255) {
      b_inc = -b_inc;
      b += b_inc;
    }
  }
}

