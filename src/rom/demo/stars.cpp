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
#include <mc1/mmio.h>

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

/// @brief Implementation of the stars demo.
class stars_t {
public:
  void init();
  void de_init();
  void draw(const int frame_no);

private:
  static const int STARS_WIDTH = 960;
  static const int STARS_HEIGHT = (STARS_WIDTH * 9) / 16;
  static const int STARS_STRIDE = (STARS_WIDTH * 2) / 8;  // 2 bpp
  static const int LOG2_NUM_STARS = 15;
  static const int NUM_STARS = 1 << LOG2_NUM_STARS;

  void plot(const int x, const int y, const int z, uint8_t* pix_buf) const;

  void draw_half_of_the_stars(const int frame_no, uint8_t* pix_buf, const bool flip_y);
  void draw_stars(const int frame_no);

  mc1::glyph_renderer_t m_glyph_renderer;
  fb_t* m_fb;
};

void stars_t::init() {
  if (m_fb != nullptr) {
    return;
  }

  // Create the star framebuffer.
  m_fb = fb_create(STARS_WIDTH, STARS_HEIGHT, CMODE_PAL2);
  if (m_fb == nullptr) {
    return;
  }
  m_fb->palette[0] = 0x00000000u;
  m_fb->palette[1] = 0x44303f49u;
  m_fb->palette[2] = 0x77707a87u;
  m_fb->palette[3] = 0xffffffffu;

  // Initiate the glyph renderer.
  m_glyph_renderer.init(6, 6);

  // TODO(m): Set up the text VCP.
}

void stars_t::de_init() {
  if (m_fb != nullptr) {
    m_glyph_renderer.deinit();

    fb_destroy(m_fb);
    m_fb = nullptr;

    vcp_set_prg(LAYER_1, nullptr);
    vcp_set_prg(LAYER_2, nullptr);
  }
}

void stars_t::draw(const int frame_no) {
  if (m_fb == nullptr) {
    return;
  }

  fb_show(m_fb, LAYER_1);

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
    auto* pix_buf = reinterpret_cast<uint8_t*>(m_fb->pixels) + STARS_STRIDE * (STARS_HEIGHT / 2);
    draw_half_of_the_stars(frame_no, pix_buf, false);
  }

  // Wait for the raster beam to reach the the middle of the screen.
  {
    auto wait_for = static_cast<int>(MMIO(VIDHEIGHT) / 2);
    while (static_cast<int>(MMIO(VIDY)) < wait_for)
      ;
  }

  // Now draw the upper half (for the next frame).
  {
    auto* pix_buf = reinterpret_cast<uint8_t*>(m_fb->pixels);
    draw_half_of_the_stars(frame_no + 1, pix_buf, true);
  }
}

stars_t s_stars;

}  // namespace

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

extern "C" void stars_init(void) {
  s_stars.init();
}

extern "C" void stars_deinit(void) {
  s_stars.de_init();
}

extern "C" void stars(int frame_no) {
  s_stars.draw(frame_no);
}
