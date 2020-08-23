// -*- : c++; tab-width: 2; indent-tabs-: nil; -*-
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

#include "demo_select.h"

#include <mc1/framebuffer.h>
#include <mc1/glyph_renderer.h>
#include <mc1/keyboard.h>
#include <mc1/leds.h>
#include <mc1/memory.h>
#include <mc1/mmio.h>
#include <mc1/vcp.h>

#include <mr32intrin.h>

#include <cstdint>
#include <cstring>

namespace {

/// @brief A very simple pseudorandom number generator (PRNG).
///
/// This is a linear congruential generator (LCG) on the form Xn+1 = (a*Xn + c) mod m, where
/// a = 1103515245, c = 12345 and m = 2^32.
///
/// @see https://en.wikipedia.org/wiki/Linear_congruential_generator
class rnd_t {
public:
  /// @brief Constructor.
  /// @param seed The initial seed of the PRNG.
  explicit rnd_t(const uint32_t seed) : m_state(seed) {
  }

  /// @brief Get the next random number in the pseudorandom number sequence.
  /// @returns an unsigned number in the range [0, 2^32-1].
  uint32_t operator()() {
    m_state = 1103515245u * m_state + 12345u;
    return m_state;
  }

private:
  uint32_t m_state;
};

inline constexpr int16x2_t const16x2(const int x) {
  return _MR32_INT16X2(x, x);
}

inline int16x2_t set16x2(const int x) {
  return static_cast<int16x2_t>(_mr32_shuf(static_cast<uint32_t>(x), _MR32_SHUFCTL(0, 1, 0, 1, 0)));
}

void wait_for_row(const int row) {
  while (static_cast<int>(MMIO(VIDY)) < row)
    ;
}

/// @brief Implementation of the stars demo.
class stars_t {
public:
  void init(const char* text);
  void de_init();
  void draw(const int frame_no);

private:
  static const int STARS_WIDTH = 960;
  static const int STARS_HEIGHT = (STARS_WIDTH * 9) / 16;
  static const int STARS_STRIDE = (STARS_WIDTH * 2) / 8;  // 2 bpp
  static const int LOG2_NUM_STARS = 15;
  static const int NUM_STARS = 1 << LOG2_NUM_STARS;

  static const int LOG2_GLYPH_SIZE = 6;
  static const int GLYPH_SIZE = 1 << 6;
  static const int TEXT_NUM_ROWS = 4;
  static const int TEXT_NUM_COLS = 24;
  static const int TEXT_ROW_WIDTH = TEXT_NUM_COLS * GLYPH_SIZE;
  static const int TEXT_ROW_STRIDE = (TEXT_ROW_WIDTH * 2) / 8;  // 2 bpp
  static const int TEXT_ROW_HEIGHT = GLYPH_SIZE;
  static const int TEXT_ROW_SPACING = 64;

  void plot(const int x, const int y, const int z, uint8_t* pix_buf) const;

  void draw_half_of_the_stars(const int frame_no, uint8_t* pix_buf, const bool flip_y);
  void draw_stars(const int frame_no);
  void draw_text(const int frame_no);

  mc1::glyph_renderer_t m_glyph_renderer;
  fb_t* m_stars_fb;
  void* m_text_mem;
  uint32_t* m_text_vcp;
  uint8_t* m_text_pixels;

  const char* m_text;
  int m_text_idx;
  int m_text_row;
  int m_text_col;
  int m_text_glyph_phase;
};

void stars_t::init(const char* text) {
  if (m_stars_fb != nullptr) {
    return;
  }

  // Create the star framebuffer.
  m_stars_fb = fb_create(STARS_WIDTH, STARS_HEIGHT, CMODE_PAL2);
  if (m_stars_fb == nullptr) {
    return;
  }
  m_stars_fb->palette[0] = 0x00000000u;
  m_stars_fb->palette[1] = 0x44303f49u;
  m_stars_fb->palette[2] = 0x77707a87u;
  m_stars_fb->palette[3] = 0xffffffffu;

  // Set up the text VCP.
  {
    // Allocate memory.
    const auto text_vcp_size =
        sizeof(uint32_t) * (8 + (2 + 2 * TEXT_ROW_HEIGHT + 3) * TEXT_NUM_ROWS + 1);
    const auto text_pixels_size =
        static_cast<size_t>(TEXT_ROW_STRIDE * TEXT_ROW_HEIGHT * TEXT_NUM_ROWS);
    const auto total_size = text_vcp_size + text_pixels_size;
    m_text_mem = mem_alloc(total_size, MEM_TYPE_VIDEO | MEM_CLEAR);
    if (m_text_mem != nullptr) {
      auto* ptr = reinterpret_cast<uint8_t*>(m_text_mem);
      m_text_vcp = reinterpret_cast<uint32_t*>(ptr);
      m_text_pixels = ptr + text_vcp_size;

      // Create the VCP.
      const auto native_width = MMIO(VIDWIDTH);
      const auto native_height = MMIO(VIDHEIGHT);

      const auto horiz_margin = (1920u - TEXT_ROW_WIDTH) / 2u;
      const auto hstrt = (horiz_margin * native_width) / 1920u;
      const auto hstop = ((1920u - horiz_margin) * native_width) / 1920u;
      const auto first_ypos =
          (static_cast<int>(native_height) -
           (TEXT_ROW_HEIGHT * TEXT_NUM_ROWS + TEXT_ROW_SPACING * (TEXT_NUM_ROWS - 1))) /
          2;

      // Prologue.
      auto* vcp = m_text_vcp;
      *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);  // Set the blend mode
      *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_PAL2);
      *vcp++ = vcp_emit_setreg(VCR_XINCR, (0x010000u * 1920u) / native_width);
      *vcp++ = vcp_emit_setpal(0, 4);
      *vcp++ = 0x00000000u;
      *vcp++ = 0x44ffaa80u;
      *vcp++ = 0x77ffaa80u;
      *vcp++ = 0xffffaa80u;

      // Text rows.
      // TODO(m): Make this resolution independent (vertically).
      auto* pixels = m_text_pixels;
      auto y = first_ypos;
      for (int k = TEXT_NUM_ROWS; k > 0; --k) {
        *vcp++ = vcp_emit_waity(y);
        *vcp++ = vcp_emit_setreg(VCR_ADDR, to_vcp_addr(reinterpret_cast<uintptr_t>(pixels)));
        *vcp++ = vcp_emit_setreg(VCR_HSTRT, hstrt);
        *vcp++ = vcp_emit_setreg(VCR_HSTOP, hstop);
        ++y;
        pixels += TEXT_ROW_STRIDE;

        for (int i = 1; i < TEXT_ROW_HEIGHT; ++i) {
          *vcp++ = vcp_emit_waity(y);
          *vcp++ = vcp_emit_setreg(VCR_ADDR, to_vcp_addr(reinterpret_cast<uintptr_t>(pixels)));
          ++y;
          pixels += TEXT_ROW_STRIDE;
        }

        *vcp++ = vcp_emit_waity(y);
        *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0);
        *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);
        y += TEXT_ROW_SPACING;
      }

      // Epilogue.
      *vcp = vcp_emit_waity(32767);
    }
  }

  // Initiate the glyph renderer.
  m_glyph_renderer.init(LOG2_GLYPH_SIZE, LOG2_GLYPH_SIZE);

  // Initialize the text.
  m_text = text;
  m_text_idx = 0;
  m_text_row = 0;
  m_text_col = 0;
  m_text_glyph_phase = 0;
}

void stars_t::de_init() {
  if (m_stars_fb != nullptr) {
    m_glyph_renderer.deinit();

    mem_free(m_text_mem);
    m_text_mem = nullptr;

    fb_destroy(m_stars_fb);
    m_stars_fb = nullptr;

    vcp_set_prg(LAYER_1, nullptr);
    vcp_set_prg(LAYER_2, nullptr);
  }
}

void stars_t::draw(const int frame_no) {
  if (m_stars_fb == nullptr) {
    return;
  }

  fb_show(m_stars_fb, LAYER_1);
  vcp_set_prg(LAYER_2, m_text_vcp);

  // Draw the text.
  draw_text(frame_no);

  // Draw the stars.
  draw_stars(frame_no);

  // For profiling: Show current raster Y position.
  sevseg_print_dec(static_cast<int>(MMIO(VIDY)));

  // Check for keyboard ESC press.
  while (auto event = kb_get_next_event()) {
    if (kb_event_is_press(event) && kb_event_scancode(event) == KB_ESC) {
      g_demo_select = DEMO_NONE;
    }
  }
}

inline void stars_t::plot(const int x, const int y, const int z, uint8_t* pix_buf) const {
#if 0
  const auto color = 3 - ((z * 3) >> (LOG2_NUM_STARS - 1));
  auto* bptr = pix_buf + (STARS_STRIDE * y) + (x >> 2);
  *bptr |= color << (2 * (x & 3));
#else
  const int three = 3;
  uint32_t color;
  uint32_t offset;
  uint32_t pix;
  uint32_t pix_shift;
  uint32_t tmp;
  __asm__ volatile(
      // Do multiplications early.
      "mul     %[color], %[z], %[three]\n\t"
      "mul     %[offset], %[y], %[STARS_STRIDE]\n\t"

      // Calculate the pixel shift.
      "and     %[pix_shift], %[x], #3\n\t"
      "lsl     %[pix_shift], %[pix_shift], #1\n\t"

      // Calculate the color.
      "asr     %[color], %[color], #%[LOG2_NUM_STARS]-1\n\t"
      "sub     %[color], #3, %[color]\n\t"

      // Calculate the byte pointer offset.
      "asr     %[tmp], %[x], #2\n\t"
      "add     %[offset], %[offset], %[tmp]\n\t"
      "ldub    %[pix], %[pix_buf], %[offset]\n\t"

      // Shift the color into the right bit-position.
      "lsl     %[color], %[color], %[pix_shift]\n\t"

      // Update the color
      "or      %[pix], %[pix], %[color]\n\t"
      "stb     %[pix], %[pix_buf], %[offset]\n\t"
      : [color] "=&r"(color),
        [offset] "=&r"(offset),
        [pix] "=&r"(pix),
        [pix_shift] "=&r"(pix_shift),
        [tmp] "=&r"(tmp)
      : [x] "r"(x),
        [y] "r"(y),
        [z] "r"(z),
        [pix_buf] "r"(pix_buf),
        [LOG2_NUM_STARS] "i"(LOG2_NUM_STARS),
        [STARS_STRIDE] "r"(STARS_STRIDE),
        [three] "r"(three)
      : "memory");
#endif
}

void stars_t::draw_half_of_the_stars(const int frame_no, uint8_t* pix_buf, const bool flip_y) {
  // Start by clearing the pixel framebuffer.
  memset(pix_buf, 0, STARS_STRIDE * (STARS_HEIGHT / 2));

  // Draw them stars.
  // TODO(m): Vectorize this routine.
  rnd_t random(flip_y ? 0x48376213u : 0xe9a7663bu);
  const auto scale_x = static_cast<uint32_t>(STARS_WIDTH);
  const auto scale_y =
      static_cast<uint32_t>(static_cast<uint16_t>(flip_y ? -STARS_WIDTH : STARS_WIDTH));
  const auto scale = _mr32_itof_h((scale_y << 16) | scale_x, const16x2(0));
  const auto screen_offset = _MR32_UINT16X2(flip_y ? STARS_HEIGHT / 2 : 0, STARS_WIDTH / 2);
  const auto screen_yx_limits = _MR32_UINT16X2(STARS_HEIGHT / 2, STARS_WIDTH);
  const auto z_offset = 37 * frame_no;
  for (int i = (NUM_STARS / 2) - 1; i >= 0; --i) {
    // Generate a 3D position for the star.
    const auto r = random() >> 1;
    const auto yx_2d = _mr32_itof_h(r, const16x2(16));
    const auto z = (i - z_offset) & ((NUM_STARS / 2) - 1);

    // Do perspective division.
    const auto z_f = _mr32_utof_h(set16x2(z + 1), const16x2(LOG2_NUM_STARS - 1));
    const auto yx_f = _mr32_fdiv_h(yx_2d, z_f);

    // Convert the coordinate to screen space (integer).
    const auto yx =
        _mr32_add_h(_mr32_ftoir_h(_mr32_fmul_h(yx_f, scale), const16x2(0)), screen_offset);

    // If the star is within the screen limits, plot it.
    if (_MR32_ALL_TRUE(_mr32_sltu_h(yx, screen_yx_limits))) {
      const auto y = static_cast<int32_t>(yx) >> 16;
      const auto x = static_cast<int32_t>(static_cast<int16_t>(yx));
      plot(x, y, z, pix_buf);
    }
  }
}

void stars_t::draw_stars(const int frame_no) {
  // Start by drawing the bottom half.
  {
    auto* pix_buf =
        reinterpret_cast<uint8_t*>(m_stars_fb->pixels) + STARS_STRIDE * (STARS_HEIGHT / 2);
    draw_half_of_the_stars(frame_no, pix_buf, false);
  }

  // Wait for the raster beam to reach the the middle of the screen.
  wait_for_row(static_cast<int>(MMIO(VIDHEIGHT) / 2));

  // Now draw the upper half (for the next frame).
  {
    auto* pix_buf = reinterpret_cast<uint8_t*>(m_stars_fb->pixels);
    draw_half_of_the_stars(frame_no + 1, pix_buf, true);
  }
}

void stars_t::draw_text(const int frame_no) {
  if (m_text_mem == nullptr) {
    return;
  }

  // Clear the buffer on the first frame.
  if (frame_no == 0) {
    memset(m_text_pixels, 0, TEXT_NUM_ROWS * TEXT_ROW_HEIGHT * TEXT_ROW_STRIDE);
  }

  if (m_text_glyph_phase == 0) {
    // Get the next character, and handle control characters.
    auto c = m_text[m_text_idx];
    while (c != 0) {
      if (c == '\n') {
        ++m_text_row;
        m_text_col = 0;
      } else if (c == ' ') {
        ++m_text_col;
      } else {
        ++m_text_idx;
        ++m_text_col;
        break;
      }
      c = m_text[++m_text_idx];
    }

    // Draw the glyph.
    m_glyph_renderer.draw_char(c);
  } else {
    // Grow the glyph.
    m_glyph_renderer.grow();
  }

  // Paint the glyph to the correct place in the text framebuffer.
  if (m_text_row < TEXT_NUM_ROWS && m_text_col <= TEXT_NUM_COLS) {
    const auto offs =
        (m_text_row * GLYPH_SIZE * TEXT_ROW_STRIDE) + ((m_text_col - 1) << (LOG2_GLYPH_SIZE - 2));
    m_glyph_renderer.paint_2bpp(m_text_pixels + offs, TEXT_ROW_STRIDE);
  }

  // Advance the glyph phase.
  m_text_glyph_phase = (m_text_glyph_phase + 1) & 7;
}

stars_t s_stars;

}  // namespace

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

extern "C" void stars_init(void) {
  // TODO(m): Pass this string as a function argument.
  const char* THE_TEXT =
      "------------------------\n"
      " MEET THE WORLD'S FIRST\n"
      "    MRISC32 COMPUTER!\n"
      "------------------------ ";

  s_stars.init(THE_TEXT);
}

extern "C" void stars_deinit(void) {
  s_stars.de_init();
}

extern "C" void stars(int frame_no) {
  s_stars.draw(frame_no);
}
