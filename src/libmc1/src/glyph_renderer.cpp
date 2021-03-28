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

#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef __MRISC32__
#include <mc1/memory.h>
#include <mr32intrin.h>
#else
#include <cstdlib>
#endif

namespace mc1 {
namespace {

//--------------------------------------------------------------------------------------------------
// Helpers.
//--------------------------------------------------------------------------------------------------

float sqr(const float x) {
  return x * x;
}

int round_to_int(const float x) {
#ifdef __MRISC32__
  int32_t r;
  __asm__("ftoir\t%0, %1, z" : "=r"(r) : "r"(x));
  return r;
#else
  return static_cast<int>(x + 0.5f);
#endif
}

uint8_t addsu(const uint8_t a, const uint8_t b) {
#ifdef __MRISC32__
  return static_cast<uint8_t>(_mr32_addsu_b(static_cast<uint8x4_t>(a), static_cast<uint8x4_t>(b)));
#else
  const uint32_t sum = static_cast<uint32_t>(a) + static_cast<uint32_t>(b);
  return sum > 255u ? 255u : static_cast<uint8_t>(sum);
#endif
}

//--------------------------------------------------------------------------------------------------
// Font definition.
//--------------------------------------------------------------------------------------------------

enum point_kind_t { PNT_REGULAR = 0, PNT_BEZIER = 1, PNT_END = 2, PNT_LAST = 3 };

struct point_t {
  point_t(const uint32_t p);
  point_t(const uint32_t p, const uint32_t shift_x, const uint32_t shift_y);

  float x;
  float y;
  point_kind_t kind;
};

point_t::point_t(const uint32_t p) {
  kind = static_cast<point_kind_t>((p >> 6) & 3u);
}

point_t::point_t(const uint32_t p, const uint32_t shift_x, const uint32_t shift_y) {
  x = static_cast<float>((((p >> 3) & 7u) + 1) << shift_x);
  y = static_cast<float>(((p & 7u) + 1) << shift_y);
  kind = static_cast<point_kind_t>((p >> 6) & 3u);
}

constexpr uint8_t PP(const uint8_t x, const uint8_t y, const point_kind_t kind) {
  return static_cast<uint8_t>(((static_cast<uint8_t>(kind) & 3u) << 6) | ((x & 7u) << 3) | (y & 7));
}

const uint8_t FONT[] = {'A',
                        PP(0, 6, PNT_REGULAR),
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 6, PNT_END),
                        PP(2, 4, PNT_REGULAR),
                        PP(4, 4, PNT_LAST),

                        'B',
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(3, 0, PNT_REGULAR),
                        PP(5, 0, PNT_BEZIER),
                        PP(5, 2, PNT_REGULAR),
                        PP(5, 3, PNT_BEZIER),
                        PP(3, 3, PNT_END),
                        PP(1, 3, PNT_REGULAR),
                        PP(3, 3, PNT_REGULAR),
                        PP(6, 3, PNT_BEZIER),
                        PP(6, 5, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(4, 6, PNT_REGULAR),
                        PP(0, 6, PNT_LAST),

                        'C',
                        PP(6, 0, PNT_REGULAR),
                        PP(4, 0, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(0, 3, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(4, 6, PNT_REGULAR),
                        PP(6, 6, PNT_LAST),

                        'D',
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 3, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(0, 6, PNT_LAST),

                        'E',
                        PP(6, 6, PNT_REGULAR),
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 0, PNT_END),
                        PP(1, 3, PNT_REGULAR),
                        PP(4, 3, PNT_LAST),

                        'F',
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 0, PNT_END),
                        PP(1, 3, PNT_REGULAR),
                        PP(4, 3, PNT_LAST),

                        'G',
                        PP(6, 0, PNT_REGULAR),
                        PP(4, 0, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(0, 3, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(4, 6, PNT_REGULAR),
                        PP(6, 6, PNT_REGULAR),
                        PP(6, 3, PNT_REGULAR),
                        PP(3, 3, PNT_LAST),

                        'H',
                        PP(0, 0, PNT_REGULAR),
                        PP(0, 6, PNT_END),
                        PP(6, 0, PNT_REGULAR),
                        PP(6, 6, PNT_END),
                        PP(1, 3, PNT_REGULAR),
                        PP(5, 3, PNT_LAST),

                        'I',
                        PP(2, 0, PNT_REGULAR),
                        PP(4, 0, PNT_END),
                        PP(2, 6, PNT_REGULAR),
                        PP(4, 6, PNT_END),
                        PP(3, 1, PNT_REGULAR),
                        PP(3, 5, PNT_LAST),

                        'J',
                        PP(1, 0, PNT_REGULAR),
                        PP(5, 0, PNT_REGULAR),
                        PP(5, 4, PNT_REGULAR),
                        PP(5, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(1, 6, PNT_BEZIER),
                        PP(1, 4, PNT_LAST),

                        'K',
                        PP(0, 0, PNT_REGULAR),
                        PP(0, 6, PNT_END),
                        PP(6, 0, PNT_REGULAR),
                        PP(1, 3, PNT_REGULAR),
                        PP(6, 6, PNT_LAST),

                        'L',
                        PP(0, 0, PNT_REGULAR),
                        PP(0, 6, PNT_REGULAR),
                        PP(6, 6, PNT_LAST),

                        'M',
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(3, 4, PNT_REGULAR),
                        PP(6, 0, PNT_REGULAR),
                        PP(6, 6, PNT_LAST),

                        'N',
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 6, PNT_REGULAR),
                        PP(6, 0, PNT_LAST),

                        'O',
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 3, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 3, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(3, 0, PNT_LAST),

                        'P',
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 1, PNT_REGULAR),
                        PP(6, 3, PNT_BEZIER),
                        PP(1, 3, PNT_LAST),

                        'Q',
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 3, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 3, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(3, 0, PNT_END),
                        PP(3, 4, PNT_REGULAR),
                        PP(6, 6, PNT_LAST),

                        'R',
                        PP(0, 6, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 1, PNT_REGULAR),
                        PP(6, 3, PNT_BEZIER),
                        PP(1, 3, PNT_END),
                        PP(2, 3, PNT_REGULAR),
                        PP(6, 6, PNT_LAST),

                        'S',
                        PP(6, 1, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(3, 0, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(0, 1, PNT_REGULAR),
                        PP(0, 3, PNT_BEZIER),
                        PP(3, 3, PNT_REGULAR),
                        PP(6, 3, PNT_BEZIER),
                        PP(6, 5, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 5, PNT_LAST),

                        'T',
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 0, PNT_END),
                        PP(3, 1, PNT_REGULAR),
                        PP(3, 6, PNT_LAST),

                        'U',
                        PP(0, 0, PNT_REGULAR),
                        PP(0, 3, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(6, 3, PNT_REGULAR),
                        PP(6, 0, PNT_LAST),

                        'V',
                        PP(0, 0, PNT_REGULAR),
                        PP(3, 6, PNT_REGULAR),
                        PP(6, 0, PNT_LAST),

                        'W',
                        PP(0, 0, PNT_REGULAR),
                        PP(1, 6, PNT_REGULAR),
                        PP(3, 3, PNT_REGULAR),
                        PP(5, 6, PNT_REGULAR),
                        PP(6, 0, PNT_LAST),

                        'X',
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 6, PNT_END),
                        PP(6, 0, PNT_REGULAR),
                        PP(0, 6, PNT_LAST),

                        'Y',
                        PP(0, 0, PNT_REGULAR),
                        PP(3, 4, PNT_REGULAR),
                        PP(3, 6, PNT_END),
                        PP(6, 0, PNT_REGULAR),
                        PP(3, 4, PNT_LAST),

                        'Z',
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 0, PNT_REGULAR),
                        PP(0, 6, PNT_REGULAR),
                        PP(6, 6, PNT_END),
                        PP(1, 3, PNT_REGULAR),
                        PP(5, 3, PNT_LAST),

                        '0',
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 3, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 3, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(3, 0, PNT_END),
                        PP(3, 2, PNT_REGULAR),
                        PP(3, 4, PNT_LAST),

                        '1',
                        PP(1, 2, PNT_REGULAR),
                        PP(3, 0, PNT_REGULAR),
                        PP(3, 6, PNT_LAST),

                        '2',
                        PP(0, 1, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 1, PNT_REGULAR),
                        PP(6, 2, PNT_BEZIER),
                        PP(4, 3, PNT_REGULAR),
                        PP(0, 6, PNT_REGULAR),
                        PP(6, 6, PNT_LAST),

                        '3',
                        PP(0, 1, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 1, PNT_REGULAR),
                        PP(6, 3, PNT_BEZIER),
                        PP(3, 3, PNT_REGULAR),
                        PP(6, 3, PNT_BEZIER),
                        PP(6, 5, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 5, PNT_LAST),

                        '4',
                        PP(6, 4, PNT_REGULAR),
                        PP(0, 4, PNT_REGULAR),
                        PP(5, 0, PNT_REGULAR),
                        PP(5, 6, PNT_LAST),

                        '5',
                        PP(6, 0, PNT_REGULAR),
                        PP(0, 0, PNT_REGULAR),
                        PP(0, 2, PNT_REGULAR),
                        PP(2, 2, PNT_REGULAR),
                        PP(6, 2, PNT_BEZIER),
                        PP(6, 4, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 5, PNT_LAST),

                        '6',
                        PP(5, 0, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(0, 4, PNT_REGULAR),
                        PP(0, 3, PNT_BEZIER),
                        PP(3, 3, PNT_REGULAR),
                        PP(6, 3, PNT_BEZIER),
                        PP(6, 4, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 3, PNT_LAST),

                        '7',
                        PP(0, 0, PNT_REGULAR),
                        PP(6, 0, PNT_REGULAR),
                        PP(4, 2, PNT_REGULAR),
                        PP(2, 3, PNT_BEZIER),
                        PP(2, 6, PNT_END),
                        PP(1, 3, PNT_REGULAR),
                        PP(5, 3, PNT_LAST),

                        '8',
                        PP(3, 2, PNT_REGULAR),
                        PP(1, 2, PNT_BEZIER),
                        PP(1, 1, PNT_REGULAR),
                        PP(1, 0, PNT_BEZIER),
                        PP(3, 0, PNT_REGULAR),
                        PP(5, 0, PNT_BEZIER),
                        PP(5, 1, PNT_REGULAR),
                        PP(5, 2, PNT_BEZIER),
                        PP(3, 2, PNT_REGULAR),
                        PP(6, 2, PNT_BEZIER),
                        PP(6, 4, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(3, 6, PNT_REGULAR),
                        PP(0, 6, PNT_BEZIER),
                        PP(0, 4, PNT_REGULAR),
                        PP(0, 2, PNT_BEZIER),
                        PP(3, 2, PNT_LAST),

                        '9',
                        PP(1, 6, PNT_REGULAR),
                        PP(6, 6, PNT_BEZIER),
                        PP(6, 2, PNT_REGULAR),
                        PP(6, 4, PNT_BEZIER),
                        PP(3, 4, PNT_REGULAR),
                        PP(0, 4, PNT_BEZIER),
                        PP(0, 2, PNT_REGULAR),
                        PP(0, 0, PNT_BEZIER),
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 3, PNT_LAST),

                        ',',
                        PP(3, 5, PNT_REGULAR),
                        PP(2, 6, PNT_LAST),

                        '.',
                        PP(2, 6, PNT_REGULAR),
                        PP(2, 6, PNT_LAST),

                        '!',
                        PP(2, 0, PNT_REGULAR),
                        PP(2, 4, PNT_END),
                        PP(2, 6, PNT_REGULAR),
                        PP(2, 6, PNT_LAST),

                        '?',
                        PP(1, 1, PNT_REGULAR),
                        PP(1, 0, PNT_BEZIER),
                        PP(3, 0, PNT_REGULAR),
                        PP(6, 0, PNT_BEZIER),
                        PP(6, 2, PNT_REGULAR),
                        PP(6, 4, PNT_BEZIER),
                        PP(3, 4, PNT_REGULAR),
                        PP(3, 5, PNT_END),
                        PP(3, 6, PNT_REGULAR),
                        PP(3, 6, PNT_LAST),

                        ':',
                        PP(2, 1, PNT_REGULAR),
                        PP(2, 1, PNT_END),
                        PP(2, 5, PNT_REGULAR),
                        PP(2, 5, PNT_LAST),

                        '"',
                        PP(2, 0, PNT_REGULAR),
                        PP(2, 1, PNT_END),
                        PP(3, 0, PNT_REGULAR),
                        PP(3, 1, PNT_LAST),

                        '\'',
                        PP(3, 0, PNT_REGULAR),
                        PP(2, 1, PNT_LAST),

                        '+',
                        PP(1, 3, PNT_REGULAR),
                        PP(5, 3, PNT_END),
                        PP(3, 1, PNT_REGULAR),
                        PP(3, 5, PNT_LAST),

                        '-',
                        PP(1, 3, PNT_REGULAR),
                        PP(5, 3, PNT_LAST),

                        '*',
                        PP(2, 2, PNT_REGULAR),
                        PP(4, 4, PNT_END),
                        PP(2, 4, PNT_REGULAR),
                        PP(4, 2, PNT_LAST),

                        '/',
                        PP(6, 0, PNT_REGULAR),
                        PP(0, 6, PNT_LAST),

                        // No more glyphs...
                        0};

}  // namespace

void glyph_renderer_t::init(const unsigned log2_width, const unsigned log2_height) {
  m_log2_width = log2_width;
  m_log2_height = log2_height;
  m_width = 1u << log2_width;
  m_height = 1u << log2_height;
  const size_t mem_required = m_width * (m_height + 2);
#ifdef __MRISC32__
  m_pixels = reinterpret_cast<uint8_t*>(mem_alloc(mem_required, MEM_TYPE_ANY | MEM_CLEAR));
#else
  m_pixels = reinterpret_cast<uint8_t*>(malloc(mem_required));
#endif
  m_work_rows = m_pixels + (m_width * m_height);
}

void glyph_renderer_t::deinit() {
  if (m_pixels != nullptr) {
#ifdef __MRISC32__
    mem_free(m_pixels);
#else
    free(m_pixels);
#endif
    m_pixels = nullptr;
  }
}

void glyph_renderer_t::draw_char(const char c) {
  if (m_pixels == nullptr) {
    return;
  }

  // Start by clearing the pixel buffer.
  memset(m_pixels, 0, m_width * m_height);

  // Find the glyph corresponding to the char c.
  const uint8_t* ptr = &FONT[0];
  while (*ptr != c && *ptr != 0) {
    ++ptr;
    for (; point_t(*ptr).kind != PNT_LAST; ++ptr)
      ;
    ++ptr;
  }
  if (*ptr == 0) {
    // We didn't have a glyph for the requested character, so don't do anything.
    return;
  }
  ++ptr;

  // Draw the glyph.
  draw_glyph(ptr);
}

void glyph_renderer_t::grow() {
  if (m_pixels == nullptr) {
    return;
  }

  // TODO(m): Grow out to the edges too (x,y = 0,0 etc).
  uint8_t* work_row = &m_work_rows[0];
  uint8_t* prev_work_row = &m_work_rows[m_width];
  const uint8_t* row1 = &m_pixels[0];
  const uint8_t* row2 = &m_pixels[m_width];
  const uint8_t* row3 = &m_pixels[2 * m_width];
  for (unsigned y = 1u; y < m_height - 1u; ++y) {
    // Read the first two columns of the 3x3 area.
    uint32_t p11 = static_cast<uint32_t>(row1[0]);
    uint32_t p12 = static_cast<uint32_t>(row1[1]);
    uint32_t p21 = static_cast<uint32_t>(row2[0]);
    uint32_t p22 = static_cast<uint32_t>(row2[1]);
    uint32_t p31 = static_cast<uint32_t>(row3[0]);
    uint32_t p32 = static_cast<uint32_t>(row3[1]);

    for (unsigned x = 1u; x < m_width - 1u; ++x) {
      // Read the last column of the 3x3 area.
      uint32_t p13 = static_cast<uint32_t>(row1[x + 1]);
      uint32_t p23 = static_cast<uint32_t>(row2[x + 1]);
      uint32_t p33 = static_cast<uint32_t>(row3[x + 1]);

      // 3x3 Gaussian kernel.
      const uint32_t d0 = p22;
      const uint32_t d1 = p12 + p21 + p23 + p32;
      const uint32_t d2 = p11 + p13 + p31 + p33;
      work_row[x] = static_cast<uint8_t>(12 * (d0 >> 6) + 8 * (d1 >> 6) + 5 * (d2 >> 6));

      // Shift all pixels to the left.
      p11 = p12;
      p12 = p13;
      p21 = p22;
      p22 = p23;
      p31 = p32;
      p32 = p33;
    }

    if (y > 1u) {
      for (unsigned x = 1; x < m_width - 1; ++x) {
        m_pixels[(y - 1) * m_width + x] = addsu(m_pixels[(y - 1) * m_width + x], prev_work_row[x]);
      }
    }

    std::swap(prev_work_row, work_row);
    row1 = row2;
    row2 = row3;
    row3 += m_width;
  }
}

#ifndef __MRISC32__
void glyph_renderer_t::paint_8bpp(uint8_t* pix, const unsigned stride) {
  if (m_pixels == nullptr) {
    return;
  }

  // TODO(m): Optimize this loop.
  const uint8_t* src = m_pixels;
  uint8_t* dst = pix;
  for (unsigned y = 0; y < m_height; ++y) {
    for (unsigned x = 0; x < m_width; ++x) {
      *dst++ = *src++;
    }
    dst += stride - m_width;
  }
}
#endif

void glyph_renderer_t::paint_2bpp(uint8_t* pix, const unsigned stride) {
  if (m_pixels == nullptr) {
    return;
  }

  // TODO(m): Optimize this loop.
  const uint8_t* src = m_pixels;
  uint8_t* dst = pix;
  for (unsigned y = 0; y < m_height; ++y) {
    for (unsigned x = 0; x < m_width; x += 4) {
      const uint32_t c1 = static_cast<uint32_t>(*src++);
      const uint32_t c2 = static_cast<uint32_t>(*src++);
      const uint32_t c3 = static_cast<uint32_t>(*src++);
      const uint32_t c4 = static_cast<uint32_t>(*src++);
      *dst++ = static_cast<uint8_t>((c4 & 0xc0u) | ((c3 & 0xc0u) >> 2) | ((c2 & 0xc0u) >> 4) |
                                    (c1 >> 6));
    }
    dst += stride - (m_width / 4);
  }
}

void glyph_renderer_t::draw_glyph(const uint8_t* points) {
  // Draw the lines and curves, 1 pixel wide.
  const uint32_t shift_x = m_log2_width - 3u;   // width / 8
  const uint32_t shift_y = m_log2_height - 3u;  // height / 8
  point_t p1 = point_t(*points++, shift_x, shift_y);
  while (p1.kind != PNT_LAST) {
    const point_t p2 = point_t(*points++, shift_x, shift_y);
    if (p2.kind == PNT_BEZIER) {
      const point_t p3 = point_t(*points++, shift_x, shift_y);
      draw_bez3(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
      p1 = p3;
    } else {
      draw_line(p1.x, p1.y, p2.x, p2.y);
      p1 = p2;
    }
    if (p1.kind == PNT_END) {
      p1 = point_t(*points++, shift_x, shift_y);
    }
  }
}

void glyph_renderer_t::draw_line(const float x0, const float y0, const float x1, const float y1) {
  const float dx = x1 - x0;
  const float dy = y1 - y0;
#ifdef __MRISC32__
  int num_steps = round_to_int(_mr32_fmax(fabsf(dx), fabsf(dy))) + 1;
#else
  int num_steps = round_to_int(fmaxf(fabsf(dx), fabsf(dy))) + 1;
#endif

  const float step_size = 1.0f / static_cast<float>(num_steps - 1);
  const float step_x = dx * step_size;
  const float step_y = dy * step_size;
  float x = x0;
  float y = y0;
#ifdef __MRISC32_HARD_FLOAT__
  // TODO(m): Vectorize this loop.
  __asm__ volatile(
      "ldi     s8, #255\n"
      "1:\n\t"
      "ftoir   s7, %2, z\n\t"
      "ftoir   s6, %1, z\n\t"
      "fadd    %2, %2, %6\n\t"
      "fadd    %1, %1, %5\n\t"
      "add     %0, %0, #-1\n\t"
      "lsl     s7, s7, %4\n\t"
      "add     s7, s7, s6\n\t"
      "stb     s8, %3, s7\n\t"
      "bnz     %0, 1b"
      : "+r"(num_steps),    // %0
        "+r"(x),            // %1
        "+r"(y)             // %2
      : "r"(m_pixels),      // %3
        "r"(m_log2_width),  // %4
        "r"(step_x),        // %5
        "r"(step_y)         // %6
      : "s6", "s7", "s8");
#else
  do {
    const int ix = round_to_int(x);
    const int iy = round_to_int(y);
    m_pixels[(iy << m_log2_width) + ix] = 255;
    x += step_x;
    y += step_y;
    --num_steps;
  } while (num_steps != 0);
#endif
}

void glyph_renderer_t::draw_bez3(const float x0,
                                 const float y0,
                                 const float x1,
                                 const float y1,
                                 const float x2,
                                 const float y2) {
  const int num_steps = 10;
  const float step_size = 1.0f / static_cast<float>(num_steps);
  float last_x = x0;
  float last_y = y0;
  float t = step_size;
  for (int i = num_steps; i != 0; --i) {
    const float x = sqr(1.0f - t) * x0 + (2.0f * t * (1.0f - t)) * x1 + sqr(t) * x2;
    const float y = sqr(1.0f - t) * y0 + (2.0f * t * (1.0f - t)) * y1 + sqr(t) * y2;
    draw_line(last_x, last_y, x, y);
    last_x = x;
    last_y = y;
    t += step_size;
  }
}

}  // namespace mc1
