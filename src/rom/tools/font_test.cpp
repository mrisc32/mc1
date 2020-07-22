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

#include <mc1/glyph_renderer.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define LOG2_WIDTH 6
#define LOG2_HEIGHT 6
#define WIDTH (1u << LOG2_WIDTH)
#define HEIGHT (1u << LOG2_HEIGHT)

#if !defined(__MRISC32__)
//--------------------------------------------------------------------------------------------------
// A test that exports graphics to a file.
//--------------------------------------------------------------------------------------------------

int main() {
  const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.!?:\"+-*/ HELLO WORLD!";
  const auto NUM_GLYPHS = sizeof(chars) - 1;
  const size_t BITMAP_WIDTH = 4096;
  std::vector<uint8_t> rendered_font(BITMAP_WIDTH * HEIGHT);

  mc1::glyph_renderer_t renderer;
  renderer.init(LOG2_WIDTH, LOG2_HEIGHT);

  for (size_t i = 0u; i < NUM_GLYPHS; ++i) {
    // Draw the glyph.
    renderer.draw_char(chars[i]);
    for (int i = 0; i < 8; ++i) {
      renderer.grow();
    }
    renderer.paint_8bpp(&rendered_font[WIDTH * i], BITMAP_WIDTH);
  }

  // Write the font map to a file.
  auto* f = fopen("/tmp/font.data", "wb");
  if (f != NULL) {
    fwrite(rendered_font.data(), 1, rendered_font.size(), f);
    fclose(f);
  }

  renderer.deinit();

  return 0;
}
#endif
