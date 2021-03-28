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

#ifndef MC1_GLYPH_RENDERER_H_
#define MC1_GLYPH_RENDERER_H_

#include <cstdint>

namespace mc1 {

class glyph_renderer_t {
public:
  void init(const unsigned log2_width, const unsigned log2_height);
  void deinit();

  void draw_char(const char c);
  void grow();

  void paint_8bpp(uint8_t* pix, const unsigned stride);
  void paint_2bpp(uint8_t* pix, const unsigned stride);

private:
  void draw_glyph(const uint8_t *points);
  void draw_line(const float x0, const float y0, const float x1, const float y1);
  void draw_bez3(const float x0,
                 const float y0,
                 const float x1,
                 const float y1,
                 const float x2,
                 const float y2);

  unsigned m_log2_width;
  unsigned m_log2_height;
  unsigned m_width;
  unsigned m_height;
  uint8_t* m_pixels;
  uint8_t* m_work_rows;
};

}  // namespace mc1

#endif  // MC1_GLYPH_RENDERER_H_

