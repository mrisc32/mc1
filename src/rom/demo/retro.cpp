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

#include "demo_select.h"

#include <mc1/fast_math.h>
#include <mc1/glyph_renderer.h>
#include <mc1/keyboard.h>
#include <mc1/leds.h>
#include <mc1/mci_decode.h>
#include <mc1/mmio.h>
#include <mc1/vcp.h>

#include <mr32intrin.h>

#include <cstddef>
#include <cstdint>
#include <algorithm>

// The MRISC32 logo is defined in another compilation unit.
extern const unsigned char mrisc32_logo[];

namespace {

uint8x4_t lerp8(uint8x4_t a, uint8x4_t b, int w) {
  uint8x4_t w1 = _mr32_shuf(255 - w, _MR32_SHUFCTL(0, 0, 0, 0, 0));
  uint8x4_t w2 = _mr32_shuf(w, _MR32_SHUFCTL(0, 0, 0, 0, 0));
  return _mr32_addsu_b(_mr32_mulhiu_b(a, w1), _mr32_mulhiu_b(b, w2));
}

/// @brief Implementation of the retro demo.
class retro_t {
public:
  void init();
  void de_init();
  void draw(const int frame_no);

private:
  static const int LOG2_SINE_LUT_ENTIRES = 10;
  static const int SINE_LUT_ENTIRES = (1 << LOG2_SINE_LUT_ENTIRES);
  static const int PIXEL_WORDS = 16;

  static const int LOG2_GLYPH_WIDTH = 6;
  static const int LOG2_GLYPH_HEIGHT = 6;
  static const int GLYPH_WIDTH = 1 << LOG2_GLYPH_WIDTH;
  static const int GLYPH_HEIGHT = 1 << LOG2_GLYPH_HEIGHT;

  int sin16(const int x) const;
  int sun_width(int y) const;

  void draw_sky(const int frame_no);
  void draw_checkerboard(const int frame_no);
  void draw_logo_and_raster_bars(const int frame_no);
  void draw_text(const int frame_no);

  void* m_base_ptr;
  uint32_t* m_vcp1;
  uint32_t* m_vcp2;
  uint32_t* m_vcp3;
  uint32_t* m_vcp3_rows;
  uint32_t* m_vcp4;
  uint32_t* m_vcp4_xoffs;
  uint32_t* m_pixels1;
  int16_t* m_sine_lut;
  uint16_t* m_sun_lut;
  const mci_header_t* m_logo_hdr;
  uint32_t* m_logo_pixels;
  uint8_t* m_text_pixels;
  int32_t m_width;
  int32_t m_height;
  int32_t m_sky_height;
  int32_t m_sun_radius;
  int32_t m_sun_max_height;
  int32_t m_vcp3_height;
  uint32_t m_text_pix_stride;

  mc1::glyph_renderer_t m_glyph_renderer;
};

void retro_t::init() {
  if (m_base_ptr != nullptr) {
    return;
  }

  // Get the native video resolution.
  m_width = static_cast<int>(MMIO(VIDWIDTH));
  m_height = static_cast<int>(MMIO(VIDHEIGHT));
  m_sky_height = (m_height * 5) >> 3;
  m_sun_radius = (m_width * 3) >> 4;
  m_sun_max_height = (m_sun_radius * 3) >> 1;

  m_vcp3_height = m_height - GLYPH_HEIGHT;

  // Get information about MCI images.
  m_logo_hdr = mci_get_header(mrisc32_logo);

  // VCP 1 (sky - top of layer 1).
  const auto vcp1_height = m_sky_height;
  const auto vcp1_size = sizeof(uint32_t) * static_cast<size_t>(4 + vcp1_height * 6);

  // VCP 2 (checker board - bottom of layer 1).
  const auto vcp2_height = m_height - m_sky_height;
  const auto vcp2_size = sizeof(uint32_t) * static_cast<size_t>(3 + vcp2_height * 6 + 1);

  // VCP 3 (logo & raster bars - top of layer 2).
  const auto vcp3_size =
      sizeof(uint32_t) * static_cast<size_t>(4 + m_logo_hdr->num_pal_colors + m_vcp3_height * 6);

  // VCP 4 (text - bottom of layer 2).
  const auto vcp4_size = sizeof(uint32_t) * static_cast<size_t>(7 + 4 + GLYPH_HEIGHT * 2 + 1);

  // Pixels for layer 1.
  const auto pix1_size = sizeof(uint32_t) * PIXEL_WORDS;

  // Sine LUT.
  const auto sine_size = sizeof(int16_t) * SINE_LUT_ENTIRES;

  // Sun outline LUT.
  const auto sun_size = sizeof(uint16_t) * static_cast<size_t>(m_sun_radius);

  // Logo memory requirement.
  const auto logo_size = mci_get_pixels_size(m_logo_hdr);

  // Pixels for the text.
  m_text_pix_stride = static_cast<uint32_t>(m_width + GLYPH_WIDTH) / 4u;
  const auto text_pixels_size = static_cast<size_t>(GLYPH_HEIGHT) * m_text_pix_stride;

  // Calculate the required memory size.
  const auto total_size = vcp1_size + vcp2_size + vcp3_size + vcp4_size + pix1_size + sine_size +
                          sun_size + logo_size + text_pixels_size;

  // Allocate memory and define all memory pointers.
  {
    auto* mem = reinterpret_cast<uint8_t*>(mem_alloc(total_size, MEM_TYPE_VIDEO | MEM_CLEAR));
    if (mem == nullptr) {
      return;
    }
    m_base_ptr = mem;

    m_vcp1 = reinterpret_cast<uint32_t*>(mem);
    mem += vcp1_size;
    m_vcp2 = reinterpret_cast<uint32_t*>(mem);
    mem += vcp2_size;
    m_vcp3 = reinterpret_cast<uint32_t*>(mem);
    mem += vcp3_size;
    m_vcp4 = reinterpret_cast<uint32_t*>(mem);
    mem += vcp4_size;
    m_pixels1 = reinterpret_cast<uint32_t*>(mem);
    mem += pix1_size;
    m_sine_lut = reinterpret_cast<int16_t*>(mem);
    mem += sine_size;
    m_sun_lut = reinterpret_cast<uint16_t*>(mem);
    mem += sun_size;
    m_logo_pixels = reinterpret_cast<uint32_t*>(mem);
    mem += logo_size;
    m_text_pixels = reinterpret_cast<uint8_t*>(mem);
  }

  // Create the VCP for layer 1 (VCP 1 + VCP 2).
  {
    auto* vcp = m_vcp1;

    // Prologue.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);  // Set the dither mode
    *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_PAL1);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, to_vcp_addr(reinterpret_cast<uintptr_t>(m_pixels1)));
    *vcp++ = vcp_emit_setreg(VCR_XINCR, 0x000000);

    // The sky.
    int y = 0;
    {
      const auto sun_top_y = m_sky_height - m_sun_max_height;
      for (; y < m_sky_height; ++y) {
        const auto w = (255 * (y - sun_top_y)) / m_sun_max_height;
        const auto sun_col = lerp8(0x0019ffff, 0x009c09fd, w);

        *vcp++ = vcp_emit_waity(y);
        *vcp++ = vcp_emit_setpal(0, 2);
        ++vcp;             // Palette color 0
        *vcp++ = sun_col;  // Palette color 1
        *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0);
        *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);
      }
    }

    // Checkerboard prologue.
    // Note: This is where m_vcp2 points.
    *vcp++ = vcp_emit_waity(y);
    *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0u);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, static_cast<uint32_t>(m_width));

    // The checker board.
    {
      for (; y < m_height; ++y) {
        *vcp++ = vcp_emit_waity(y);
        *vcp++ = vcp_emit_setreg(VCR_XOFFS, 0);
        *vcp++ = vcp_emit_setreg(VCR_XINCR, 0x000400);
        *vcp++ = vcp_emit_setpal(0, 2);
        vcp += 2;  // Palette colors 0 & 1
      }
    }

    // Epilogue.
    *vcp = vcp_emit_waity(32767);
  }

  // Create the VCP for layer 2 (vcp3 + vcp4).
  {
    auto* vcp = m_vcp3;

    // Prologue.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);  // Set the blend mode.
    *vcp++ = vcp_emit_setreg(VCR_CMODE, m_logo_hdr->pixel_format);
    *vcp++ = vcp_emit_setreg(VCR_XINCR, 0x010000u);

    // Define the palette.
    const auto num_pal_colors = static_cast<uint32_t>(m_logo_hdr->num_pal_colors);
    if (num_pal_colors > 0u) {
      *vcp++ = vcp_emit_setpal(0, num_pal_colors);
      mci_decode_palette(mrisc32_logo, vcp);
      vcp += num_pal_colors;
    }

    // Per-line commands.
    m_vcp3_rows = vcp;
    for (int y = 0; y < m_vcp3_height; ++y) {
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setpal(0, 1);
      ++vcp;  // Palette color 0
      *vcp++ = vcp_emit_setreg(VCR_ADDR, 0);
      *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0);
      *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);
    }

    // Define text render mode and palette.
    *vcp++ = vcp_emit_waity(m_vcp3_height);
    m_vcp4_xoffs = vcp;
    *vcp++ = vcp_emit_setreg(VCR_XOFFS, 0);
    *vcp++ = vcp_emit_setreg(VCR_XINCR, 0x010000u);
    *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, static_cast<uint32_t>(m_width));
    *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_PAL2);
    *vcp++ = vcp_emit_setpal(0, 4);
    *vcp++ = 0xa0000000u;
    *vcp++ = 0xb54a5545u;
    *vcp++ = 0xca95aa8au;
    *vcp++ = 0xe0e0ffd0u;

    // Text per-line commands.
    {
      auto addr = reinterpret_cast<uintptr_t>(m_text_pixels);
      for (int y = m_vcp3_height; y < m_height; ++y) {
        *vcp++ = vcp_emit_waity(y);
        *vcp++ = vcp_emit_setreg(VCR_ADDR, to_vcp_addr(addr));
        addr += m_text_pix_stride;
      }
    }

    // Epilogue.
    *vcp = vcp_emit_waity(32767);
  }

  // Create the pixels for the checker board.
  for (int k = 0; k < PIXEL_WORDS; ++k) {
    m_pixels1[k] = 0x0f0f0f0fu;
  }

  // Create the sine LUT.
  for (int k = 0; k < SINE_LUT_ENTIRES; ++k) {
    const auto y = fast_sin(k * (6.283185307f / static_cast<float>(SINE_LUT_ENTIRES)));
    m_sine_lut[k] = static_cast<int16_t>(32767.0f * y);
  }

  // Create the sun outline LUT.
  {
    const float sun_width = static_cast<float>(m_sun_radius);
    for (int k = 0; k < m_sun_radius; ++k) {
      const auto y = (1.0f / sun_width) * static_cast<float>(m_sun_radius - k);
      const auto x = fast_sqrt(1.0f - y * y);
      m_sun_lut[k] = static_cast<uint16_t>(sun_width * x);
    }
  }

  // Decode images.
  mci_decode_pixels(mrisc32_logo, m_logo_pixels);

  // Initiate the glyph renderer.
  m_glyph_renderer.init(6, 6);
}

void retro_t::de_init() {
  if (m_base_ptr != nullptr) {
    m_glyph_renderer.deinit();

    mem_free(m_base_ptr);
    m_base_ptr = nullptr;

    vcp_set_prg(LAYER_1, nullptr);
    vcp_set_prg(LAYER_2, nullptr);
  }
}

void retro_t::draw(const int frame_no) {
  if (m_base_ptr == nullptr) {
    return;
  }

  vcp_set_prg(LAYER_1, m_vcp1);
  vcp_set_prg(LAYER_2, m_vcp3);

  // We draw top-to-bottom (roughly) in order to be done with the work before the raster beam
  // catches up (to avoid tearing).
  draw_sky(frame_no);
  draw_logo_and_raster_bars(frame_no);
  draw_checkerboard(frame_no);
  draw_text(frame_no);

  // Light up the leds.
  {
    int led_pos = (frame_no / 8) % 18;
    if (led_pos >= 10) {
      led_pos = 18 - led_pos;
    }
    set_leds(1 << led_pos);
  }

  // For profiling: Show current raster Y position.
  sevseg_print_dec(static_cast<int>(MMIO(VIDY)));

  // Check for keyboard ESC press.
  while (auto event = kb_get_next_event()) {
    if (kb_event_is_press(event) && kb_event_scancode(event) == KB_ESC) {
      g_demo_select = DEMO_NONE;
    }
  }
}

int retro_t::sin16(const int x) const {
  return static_cast<int>(m_sine_lut[x & (SINE_LUT_ENTIRES - 1)]);
}

int retro_t::sun_width(int y) const {
  if (y >= m_sun_radius) {
    y = 2 * m_sun_radius - 1 - y;
  }
  return y >= 0 ? static_cast<int>(m_sun_lut[y]) : 0;
}

void retro_t::draw_sky(const int frame_no) {
  static const uint8x4_t SKY_COLS[] = {
      0x00000000u, 0x00080002u, 0x00200010u, 0x00601020u, 0x00802060u, 0x00802880u, 0x00c030f0u};

  // TODO(m): Make the sun stop smoothly.
  const auto w_scale = static_cast<float>(sizeof(SKY_COLS) / sizeof(SKY_COLS[0]) - 1) /
                       static_cast<float>(m_sky_height);
  auto sun_rise = _mr32_min(frame_no, SINE_LUT_ENTIRES / 2);
  sun_rise = (m_sun_max_height * sin16(sun_rise >> 1)) >> 15;
  const auto horiz_mid = static_cast<uint32_t>(m_width >> 1);
  auto* vcp = m_vcp1 + 4;
  for (int y = 0; y < m_sky_height; ++y) {
    // Modulate the sky with a slow blue sine.
    // TODO(m): Make this resolution-independent.
    const auto s = 128 + (sin16(frame_no * 2 + y * 3) >> 8);
    const auto sin_mod = _mr32_mulhiu_b(0x0040160eu, _mr32_shuf(s, _MR32_SHUFCTL(0, 0, 0, 0, 0)));

    // Generate a sweet sky gradient.
    const auto w = _mr32_ftoi(w_scale * static_cast<float>(y), 8);
    const auto idx = w >> 8;
    const auto sky_col = lerp8(SKY_COLS[idx], SKY_COLS[idx + 1], w & 255);
    vcp[2] = _mr32_addsu_b(sky_col, sin_mod);

    // Draw the sun.
    const auto sun_y = y - (m_sky_height - sun_rise);
    auto sun_w = static_cast<uint32_t>(sun_width(sun_y));

    // Add horizontal masks, retro style!
    // TODO(m): Make this resolution-independent.
    if ((y & 31) < ((y - 320) >> 4)) {
      sun_w = 0u;
    }

    vcp[4] = vcp_emit_setreg(VCR_HSTRT, horiz_mid - sun_w);
    vcp[5] = vcp_emit_setreg(VCR_HSTOP, horiz_mid + sun_w);

    vcp += 6;
  }
}

void retro_t::draw_checkerboard(const int frame_no) {
  // Create the checker board at the bottom of the screen.
  const auto checker_height = m_height - m_sky_height;
  const auto width_div2 = _mr32_itof(m_width, 1);
  const auto scale_step = 10.0f / static_cast<float>(checker_height);
  auto scale_div = 1.0f;
  const auto offs_base = 0x10000000 + (sin16(frame_no) * 32);
  const auto check_fade_scale = 255.0f / static_cast<float>(checker_height);
  auto* vcp = m_vcp2 + 3;
  for (int y = 0; y < checker_height; ++y) {
    // Calculate and set the XOFFS and XINCR registers.
    const auto scale = (1.0f / 8.0f) / scale_div;
    scale_div += scale_step;
    const auto offs = scale * width_div2;
    const auto xoffs = offs_base - _mr32_ftoir(offs, 16);
    const auto xincr = _mr32_ftoir(scale, 16);
    vcp[1] = vcp_emit_setreg(VCR_XOFFS, static_cast<uint32_t>(xoffs) & 0x000fffffu);
    vcp[2] = vcp_emit_setreg(VCR_XINCR, static_cast<uint32_t>(xincr) & 0x00ffffffu);

    // Calculate and set the palette colors.
    // 1) Use alternating colors to achieve the checker effect.
    auto color0 = static_cast<uint8x4_t>(0x00ffc0d0u);
    auto color1 = static_cast<uint8x4_t>(0x00302010u);
    if (((_mr32_ftoi(scale, 13) + frame_no) & 32) != 0) {
      const auto tmp = color1;
      color1 = color0;
      color0 = tmp;
    }
    // 2) Fade towards a common color at the horizon.
    const auto w = static_cast<int>(check_fade_scale * static_cast<float>(y));
    vcp[4] = lerp8(0x006060a0u, color0, w);
    vcp[5] = lerp8(0x006060a0u, color1, w);

    vcp += 6;
  }
}

void retro_t::draw_logo_and_raster_bars(const int frame_no) {
  // Clear the raster colors.
  {
    // TODO(m): Do this in a vector loop instead.
    auto* vcp = m_vcp3_rows;
    for (int y = 0; y < m_vcp3_height; ++y) {
      vcp[2] = 0u;
      vcp[4] = vcp_emit_setreg(VCR_HSTRT, 0);
      vcp[5] = vcp_emit_setreg(VCR_HSTOP, 0);
      vcp += 6;
    }
  }

  // Update the image.
  {
    // Calculate the image position and properties.
    const auto width_div2 = m_width >> 1;
    const auto height_div2 = m_height >> 1;
    const auto img_x = width_div2 + ((width_div2 * sin16(frame_no * 2)) >> 16);
    const auto img_y = height_div2 + ((height_div2 * sin16(frame_no * 3)) >> 16);
    const auto img_w = static_cast<int>(m_logo_hdr->width);
    const auto img_h = static_cast<int>(m_logo_hdr->height);

    const int BANNER_H = 16;
    const auto y0 = img_y - (img_h >> 1) - BANNER_H;
    uint32_t* vcp = m_vcp3_rows + 6 * y0;

    // The banner top.
    for (int y = 0; y < BANNER_H; ++y) {
      vcp[2] = lerp8(0x08ffffffu, 0x80ffffffu, y << 4);
      vcp += 6;
    }

    const auto hstop_0 = img_x + (img_w >> 1);
    const auto row_stride = mci_get_stride(m_logo_hdr) / 4u;
    auto row_adr = to_vcp_addr(reinterpret_cast<uintptr_t>(m_logo_pixels));

    // The image.
    const auto wiggle_x0 = frame_no * 13 + 123;
    for (int y = 0; y < img_h; ++y) {
      // Apply some horizontal wiggle.
      auto wiggle_x = wiggle_x0 + y * 3;
      if ((wiggle_x & (3 << LOG2_SINE_LUT_ENTIRES)) != 0) {
        wiggle_x = 0;
      }
      wiggle_x = sin16(wiggle_x + (SINE_LUT_ENTIRES / 4)) >> 10;

      const auto hstop = static_cast<uint32_t>(hstop_0 + wiggle_x);
      const auto hstrt = hstop - static_cast<uint32_t>(img_w);

      vcp[2] = 0x80ffffffu;
      vcp[3] = vcp_emit_setreg(VCR_ADDR, row_adr);
      vcp[4] = vcp_emit_setreg(VCR_HSTRT, hstrt);
      vcp[5] = vcp_emit_setreg(VCR_HSTOP, hstop);
      row_adr += row_stride;
      vcp += 6;
    }

    // The banner bottom.
    for (int y = 0; y < BANNER_H; ++y) {
      vcp[2] = lerp8(0x80ffffffu, 0x08ffffffu, y << 4);
      vcp += 6;
    }
  }

  // Draw a few raster bars.
  {
    const int NUM_BARS = 16;
    const auto bar_color_1 = static_cast<uint8x4_t>(0xff44ffc7u);
    const auto bar_color_2 = static_cast<uint8x4_t>(0xffff43ffu);

    // Calculate the bar alpha.
    auto alpha = (sin16((frame_no - 800) >> 1) >> 7) + 100;
    alpha = (alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha));

    for (int k = 0; k < NUM_BARS; ++k) {
      // Calculate the bar position.
      auto pos = sin16((frame_no + 4 * k) * (SINE_LUT_ENTIRES / 256));
      pos = (m_height >> 1) + (((m_height * 3) * pos) >> 18);

      // Calculate the bar color.
      const auto w1 = static_cast<uint32_t>(k * (255 / (NUM_BARS - 1)));
      const auto w2 = 255u - w1;
      const auto bar_color =
          _mr32_addsu_b(_mr32_mulhiu_b(bar_color_1, _mr32_shuf(w1, _MR32_SHUFCTL(0, 0, 0, 0, 0))),
                        _mr32_mulhiu_b(bar_color_2, _mr32_shuf(w2, _MR32_SHUFCTL(0, 0, 0, 0, 0))));

      // Draw the bar.
      for (int i = -32; i <= 32; ++i) {
        const auto y = pos + i;
        const auto intensity = static_cast<uint32_t>(alpha * (32 - std::abs(i))) >> 5;
        const auto color = _mr32_mulhiu_b(bar_color, _mr32_shuf(intensity, 0));
        auto* color_ptr = m_vcp3_rows + 2 + 6 * y;
        *color_ptr = _mr32_maxu_b(color, *color_ptr);
      }
    }
  }
}

void retro_t::draw_text(const int frame_no) {
  static const char SCROLL_TEXT[] =
      "                                                                                "
      "THIS DEMO IS RUNNING AT 1920*1080 AT 60FPS, WITH LOTS OF CPU TIME TO SPARE AND USING LESS "
      "THAN 110KB VRAM..."
      "                                                                                "
      "                                                                                ";

  const auto SCROLL_SPEED = 8;

  // Calculate the text position.
  const auto text_pos = frame_no * SCROLL_SPEED;
  const auto scroll_pos = text_pos % GLYPH_WIDTH;

  // Update the horizontal scroll value.
  *m_vcp4_xoffs = vcp_emit_setreg(VCR_XOFFS, static_cast<uint32_t>(scroll_pos) << 16);

  // Render & paint the next glyph.
  if (scroll_pos == (GLYPH_WIDTH - 4 * SCROLL_SPEED)) {
    // Phase 1: Select character and render the glyph.
    const auto text_idx =
        static_cast<uint32_t>(text_pos / GLYPH_WIDTH) % (sizeof(SCROLL_TEXT) - 1u);
    const auto c = SCROLL_TEXT[text_idx];
    m_glyph_renderer.draw_char(c);
    m_glyph_renderer.grow();
    m_glyph_renderer.grow();
  } else if (scroll_pos == (GLYPH_WIDTH - 3 * SCROLL_SPEED)) {
    // Phase 2: Continue rendering the glyph.
    m_glyph_renderer.grow();
    m_glyph_renderer.grow();
  } else if (scroll_pos == (GLYPH_WIDTH - 2 * SCROLL_SPEED)) {
    // Phase 3: Continue rendering the glyph.
    m_glyph_renderer.grow();
    m_glyph_renderer.grow();
  } else if (scroll_pos == (GLYPH_WIDTH - SCROLL_SPEED)) {
    // Phase 4: Continue rendering the glyph.
    m_glyph_renderer.grow();
    m_glyph_renderer.grow();
  } else if (scroll_pos == 0) {
    // Phase 5: Scroll the text pixels to the left, one glyph...
    {
      const int words_per_glyph = (GLYPH_WIDTH >> 2) / static_cast<int>(sizeof(uint32_t));
      const int words_per_row = (m_width >> 2) / static_cast<int>(sizeof(uint32_t));
      auto* dst = reinterpret_cast<uint32_t*>(m_text_pixels);
      const auto* src = dst + words_per_glyph;
      for (int y = 0; y < GLYPH_HEIGHT; ++y) {
        auto words_left = words_per_row;
        __asm__ volatile(
            "cpuid   r2, z, z\n"
            "1:\n\t"
            "min     vl, %0, r2\n\t"
            "sub     %0, %0, vl\n\t"
            "ldw     v1, [%1, #4]\n\t"
            "ldea    %1, [%1, vl*4]\n\t"
            "stw     v1, [%2, #4]\n\t"
            "ldea    %2, [%2, vl*4]\n\t"
            "bnz     %0, 1b"
            : "+r"(words_left),  // %0
              "+r"(src),         // %1
              "+r"(dst)          // %2
            :
            : "r2", "vl", "v1", "memory");
        dst += words_per_glyph;
        src += words_per_glyph;
      }
    }

    // ...and paint the glyph.
    auto* pix_ptr = m_text_pixels + m_text_pix_stride - (GLYPH_WIDTH / 4);
    m_glyph_renderer.paint_2bpp(pix_ptr, m_text_pix_stride);
  }
}

retro_t s_retro;

}  // namespace

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

extern "C" void retro_init(void) {
  s_retro.init();
}

extern "C" void retro_deinit(void) {
  s_retro.de_init();
}

extern "C" void retro(int frame_no) {
  s_retro.draw(frame_no);
}
