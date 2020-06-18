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

#include <mc1/fast_math.h>
#include <mc1/mmio.h>
#include <mc1/vcp.h>
#include <mc1/mci_decode.h>

#include <mr32intrin.h>
#include <stddef.h>
#include <stdint.h>

#define LOG2_SINE_LUT_ENTIRES 10
#define SINE_LUT_ENTIRES      (1 << LOG2_SINE_LUT_ENTIRES)
#define PIXEL_WORDS           16

typedef struct {
  void* base_ptr;
  uint32_t* vcp1;
  uint32_t* vcp2;
  uint32_t* vcp3;
  uint32_t* vcp3_rows;
  uint32_t* pixels1;
  int16_t* sine_lut;
  uint16_t* sun_lut;
  const mci_header_t* logo_hdr;
  uint32_t* logo_pixels;
  int32_t width;
  int32_t height;
  int32_t sky_height;
  int32_t sun_radius;
  int32_t sun_max_height;
} retro_t;

static retro_t s_retro;

extern const unsigned char mrisc32_logo[];


//--------------------------------------------------------------------------------------------------
// Drawing routines.
//--------------------------------------------------------------------------------------------------

static int sin16(int x) {
  return (int)s_retro.sine_lut[((unsigned)x) & (SINE_LUT_ENTIRES - 1u)];
}

static int sun_width(int y) {
  if (y >= s_retro.sun_radius) {
    y = 2 * s_retro.sun_radius - 1 - y;
  }
  return y >= 0 ? (int)s_retro.sun_lut[y] : 0;
}

static int iabs(int x) {
  return x < 0 ? -x : x;
}

static uint8x4_t lerp8(uint8x4_t a, uint8x4_t b, int w) {
  uint8x4_t w1 = _mr32_shuf(255 - w, _MR32_SHUFCTL(0, 0, 0, 0, 0));
  uint8x4_t w2 = _mr32_shuf(w, _MR32_SHUFCTL(0, 0, 0, 0, 0));
  return _mr32_addsu_b(_mr32_mulhiu_b(a, w1), _mr32_mulhiu_b(b, w2));
}

static void draw_sky(const int frame_no) {
  static const uint8x4_t SKY_COLS[] = {
    0x00000000u, 0x00080002u, 0x00200010u, 0x00601020u, 0x00802060u, 0x00802880u, 0x00c030f0u
  };

  // TODO(m): Make the sun stop smoothly.
  const float w_scale = ((float)(sizeof(SKY_COLS)/sizeof(SKY_COLS[0]) - 1)) /
                        (float)s_retro.sky_height;
  int sun_rise = _mr32_min(frame_no, SINE_LUT_ENTIRES / 2);
  sun_rise = (s_retro.sun_max_height * sin16(sun_rise >> 1)) >> 15;
  const uint32_t horiz_mid = (uint32_t)(s_retro.width >> 1);
  uint32_t* vcp = s_retro.vcp1 + 4;
  for (int y = 0; y < s_retro.sky_height; ++y) {
    // Modulate the sky with a slow blue sine.
    // TODO(m): Make this resolution-independent.
    const int s = 128 + (sin16(frame_no * 2 + y * 3) >> 8);
    const uint8x4_t sin_mod =
        _mr32_mulhiu_b(0x0040160eu, _mr32_shuf(s, _MR32_SHUFCTL(0, 0, 0, 0, 0)));

    // Generate a sweet sky gradient.
    const int w = _mr32_ftoi(w_scale * (float)y, 8);
    const int idx = w >> 8;
    const uint8x4_t sky_col = lerp8(SKY_COLS[idx], SKY_COLS[idx+1], w & 255);
    vcp[2] = _mr32_addsu_b(sky_col, sin_mod);

    // Draw the sun.
    const int sun_y = y - (s_retro.sky_height - sun_rise);
    uint32_t sun_w = (uint32_t)sun_width(sun_y);

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

static void draw_checkerboard(const int frame_no) {
  // Create the checker board at the bottom of the screen.
  const int checker_height = s_retro.height - s_retro.sky_height;
  const float width_div2 = _mr32_itof(s_retro.width, 1);
  const float scale_step = 10.0f / (float)checker_height;
  float scale_div = 1.0f;
  const int offs_base = 0x10000000 + (sin16(frame_no) * 32);
  const float check_fade_scale = 255.0f / (float)checker_height;
  uint32_t* vcp = s_retro.vcp2 + 3;
  for (int y = 0; y < checker_height; ++y) {
    // Calculate and set the XOFFS and XINCR registers.
    const float scale = (1.0f / 8.0f) / scale_div;
    scale_div += scale_step;
    const float offs = scale * width_div2;
    const int32_t xoffs = offs_base - _mr32_ftoir(offs, 16);
    const int32_t xincr = _mr32_ftoir(scale, 16);
    vcp[1] = vcp_emit_setreg(VCR_XOFFS, (uint32_t)xoffs & 0x000fffffu);
    vcp[2] = vcp_emit_setreg(VCR_XINCR, (uint32_t)xincr & 0x00ffffffu);

    // Calculate and set the palette colors.
    // 1) Use alternating colors to achieve the checker effect.
    uint32_t color0 = 0x00ffc0d0u;
    uint32_t color1 = 0x00302010u;
    if (((_mr32_ftoi(scale, 13) + frame_no) & 32) != 0) {
      uint32_t tmp = color1;
      color1 = color0;
      color0 = tmp;
    }
    // 2) Fade towards a common color at the horizon.
    const int w = (int)(check_fade_scale * (float)y);
    vcp[4] = lerp8(0x006060a0u, color0, w);
    vcp[5] = lerp8(0x006060a0u, color1, w);

    vcp += 6;
  }
}

static void draw_logo_and_raster_bars(const int frame_no) {
  // Clear the raster colors.
  {
    // TODO(m): Do this in a vector loop instead.
    uint32_t* vcp = s_retro.vcp3_rows;
    for (int y = 0; y < s_retro.height; ++y) {
      vcp[2] = 0u;
      vcp[4] = vcp_emit_setreg(VCR_HSTRT, 0);
      vcp[5] = vcp_emit_setreg(VCR_HSTOP, 0);
      vcp += 6;
    }
  }

  // Update the image.
  {
    // Calculate the image position and properties.
    const int width_div2 = s_retro.width >> 1;
    const int height_div2 = s_retro.height >> 1;
    const int img_x = width_div2 + ((width_div2 * sin16(frame_no * 2)) >> 16);
    const int img_y = height_div2 + ((height_div2 * sin16(frame_no * 3)) >> 16);
    const int img_w = (int)s_retro.logo_hdr->width;
    const int img_h = (int)s_retro.logo_hdr->height;

    const int BANNER_H = 16;
    const int y0 = img_y - (img_h >> 1) - BANNER_H;
    uint32_t* vcp = s_retro.vcp3_rows + 6 * y0;

    // The banner top.
    for (int y = 0; y < BANNER_H; ++y) {
      vcp[2] = lerp8(0x08ffffffu, 0x80ffffffu, y << 4);
      vcp += 6;
    }

    const int hstop_0 = img_x + (img_w >> 1);
    uint32_t img_adr = to_vcp_addr((uint32_t)s_retro.logo_pixels);
    const uint32_t img_stride = mci_get_stride(s_retro.logo_hdr) / 4;

    // The image.
    for (int y = 0; y < img_h; ++y) {
      // Apply some horizontal wiggle.
      int wiggle_x = frame_no * 8 + y * 3;
      if ((wiggle_x & (3 << LOG2_SINE_LUT_ENTIRES)) != 0) {
        wiggle_x = 0;
      }
      wiggle_x = sin16(wiggle_x + (SINE_LUT_ENTIRES / 4)) >> 10;

      const uint32_t hstop = (uint32_t)(hstop_0 + wiggle_x);
      const uint32_t hstrt = hstop - (uint32_t)img_w;

      vcp[2] = 0x80ffffffu;
      vcp[3] = vcp_emit_setreg(VCR_ADDR, img_adr);
      vcp[4] = vcp_emit_setreg(VCR_HSTRT, hstrt);
      vcp[5] = vcp_emit_setreg(VCR_HSTOP, hstop);
      img_adr += img_stride;
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
    const uint8x4_t bar_color_1 = 0xff44ffc7u;
    const uint8x4_t bar_color_2 = 0xffff43ffu;

    // Calculate the bar alpha.
    int alpha = (sin16((frame_no - 800) >> 1) >> 7) + 100;
    alpha = (alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha));

    for (int k = 0; k < NUM_BARS; ++k) {
      // Calculate the bar position.
      int pos = sin16((frame_no + 4 * k) * (SINE_LUT_ENTIRES / 256));
      pos = (s_retro.height >> 1) + (((s_retro.height * 3) * pos) >> 18);

      // Calculate the bar color.
      const uint32_t w1 = (uint32_t)(k * (255 / (NUM_BARS - 1)));
      const uint32_t w2 = 255u - w1;
      const uint8x4_t bar_color = _mr32_addsu_b(
          _mr32_mulhiu_b(bar_color_1, _mr32_shuf(w1, _MR32_SHUFCTL(0, 0, 0, 0, 0))),
          _mr32_mulhiu_b(bar_color_2, _mr32_shuf(w2, _MR32_SHUFCTL(0, 0, 0, 0, 0))));

      // Draw the bar.
      for (int i = -32; i <= 32; ++i) {
        const int y = pos + i;
        const uint32_t intensity = ((uint32_t)(alpha * (32 - iabs(i)))) >> 5;
        const uint8x4_t color = _mr32_mulhiu_b(bar_color, _mr32_shuf(intensity, 0));
        uint8x4_t* color_ptr = s_retro.vcp3_rows + 2 + 6 * y;
        *color_ptr = _mr32_maxu_b(color, *color_ptr);
      }
    }
  }
}


//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

void retro_init(void) {
  if (s_retro.base_ptr != NULL) {
    return;
  }

  // Get the native video resolution.
  s_retro.width = (int)MMIO(VIDWIDTH);
  s_retro.height = (int)MMIO(VIDHEIGHT);
  s_retro.sky_height = (s_retro.height * 5) >> 3;
  s_retro.sun_radius = (s_retro.width * 3) >> 4;
  s_retro.sun_max_height = (s_retro.sun_radius * 3) >> 1;

  // Get information about MCI images.
  s_retro.logo_hdr = mci_get_header(mrisc32_logo);

  // VCP 1 (sky - top of layer 1).
  const int32_t vcp1_height = s_retro.sky_height;
  const size_t vcp1_size = sizeof(uint32_t) * (4 + vcp1_height * 6);

  // VCP 2 (checker board - bottom of layer 1).
  const int32_t vcp2_height = s_retro.height - s_retro.sky_height;
  const size_t vcp2_size = sizeof(uint32_t) * (3 + vcp2_height * 6 + 1);

  // VCP 3 (layer 2).
  const size_t vcp3_size =
      sizeof(uint32_t) * (4 + s_retro.logo_hdr->num_pal_colors + s_retro.height * 6 + 1);

  // Pixels for layer 1.
  const size_t pix1_size = sizeof(uint32_t) * PIXEL_WORDS;

  // Sine LUT.
  const size_t sine_size = sizeof(int16_t) * SINE_LUT_ENTIRES;

  // Sun outline LUT.
  const size_t sun_size = sizeof(uint16_t) * s_retro.sun_radius;

  // Logo memory requirement.
  const size_t logo_size = mci_get_pixels_size(s_retro.logo_hdr);

  // Calculate the required memory size.
  const size_t total_size = vcp1_size +
                            vcp2_size +
                            vcp3_size +
                            pix1_size +
                            sine_size +
                            sun_size +
                            logo_size;

  uint8_t* mem = (uint8_t*)mem_alloc(total_size, MEM_TYPE_VIDEO | MEM_CLEAR);
  if (mem == NULL) {
    return;
  }
  s_retro.base_ptr = mem;

  s_retro.vcp1 = (uint32_t*)(mem);
  s_retro.vcp2 = (uint32_t*)(mem + vcp1_size);
  s_retro.vcp3 = (uint32_t*)(mem + vcp1_size + vcp2_size);
  s_retro.pixels1 = (uint32_t*)(mem + vcp1_size + vcp2_size + vcp3_size);
  s_retro.sine_lut = (int16_t*)(mem + vcp1_size + vcp2_size + vcp3_size + pix1_size);
  s_retro.sun_lut = (uint16_t*)(mem + vcp1_size + vcp2_size + vcp3_size + pix1_size + sine_size);
  s_retro.logo_pixels =
      (uint32_t*)(mem + vcp1_size + vcp2_size + vcp3_size + pix1_size + sine_size + sun_size);

  // Create the VCP for layer 1 (VCP 1 + VCP 2).
  {
    uint32_t* vcp = s_retro.vcp1;

    // Prologue.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);  // Set the dither mode
    *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_PAL1);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, to_vcp_addr((uint32_t)s_retro.pixels1));
    *vcp++ = vcp_emit_setreg(VCR_XINCR, 0x000000);

    // The sky.
    int y = 0;
    {
      const int sun_top_y = s_retro.sky_height - s_retro.sun_max_height;
      for (; y < s_retro.sky_height; ++y) {
        const int w = (255 * (y - sun_top_y)) / s_retro.sun_max_height;
        const uint8x4_t sun_col = lerp8(0x0019ffff, 0x009c09fd, w);

        *vcp++ = vcp_emit_waity(y);
        *vcp++ = vcp_emit_setpal(0, 2);
        ++vcp;             // Palette color 0
        *vcp++ = sun_col;  // Palette color 1
        *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0);
        *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);
      }
    }

    // Checkerboard prologue.
    // Note: This is where s_retro.vcp2 points.
    *vcp++ = vcp_emit_waity(y);
    *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, s_retro.width);

    // The checker board.
    {
      for (; y < s_retro.height; ++y) {
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

  // Create the VCP for layer 2.
  {
    uint32_t* vcp = s_retro.vcp3;

    // Prologue.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);  // Set the blend mode.
    *vcp++ = vcp_emit_setreg(VCR_CMODE, s_retro.logo_hdr->pixel_format);
    *vcp++ = vcp_emit_setreg(VCR_XINCR, 0x010000u);

    // Define the palette.
    if (s_retro.logo_hdr->num_pal_colors > 0u) {
      *vcp++ = vcp_emit_setpal(0, s_retro.logo_hdr->num_pal_colors);
      mci_decode_palette(mrisc32_logo, vcp);
      vcp += s_retro.logo_hdr->num_pal_colors;
    }

    // Per-line commands.
    s_retro.vcp3_rows = vcp;
    for (int y = 0; y < s_retro.height; ++y) {
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setpal(0, 1);
      ++vcp;  // Palette color 0
      *vcp++ = vcp_emit_setreg(VCR_ADDR, 0);
      *vcp++ = vcp_emit_setreg(VCR_HSTRT, 0);
      *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);
    }

    // Epilogue.
    *vcp = vcp_emit_waity(32767);
  }

  // Create the pixels for the checker board.
  for (int k = 0; k < PIXEL_WORDS; ++k) {
    s_retro.pixels1[k] = 0x0f0f0f0fu;
  }

  // Create the sine LUT.
  for (int k = 0; k < SINE_LUT_ENTIRES; ++k) {
    const float y = fast_sin(k * (6.283185307f / (float)SINE_LUT_ENTIRES));
    s_retro.sine_lut[k] = (int16_t)(32767.0f * y);
  }

  // Create the sun outline LUT.
  {
    const float sun_width = (float)s_retro.sun_radius;
    for (int k = 0; k < s_retro.sun_radius; ++k) {
      const float y = (1.0f / sun_width) * (float)(s_retro.sun_radius - k);
      const float x = fast_sqrt(1.0f - y * y);
      s_retro.sun_lut[k] = (uint16_t)(sun_width * x);
    }
  }

  // Decode images.
  {
    mci_decode_pixels(mrisc32_logo, s_retro.logo_pixels);
  }
}

void retro_deinit(void) {
  if (s_retro.base_ptr != NULL) {
    mem_free(s_retro.base_ptr);
    s_retro.base_ptr = NULL;

    vcp_set_prg(LAYER_1, NULL);
    vcp_set_prg(LAYER_2, NULL);
  }
}

void retro(int frame_no) {
  if (s_retro.base_ptr == NULL) {
    return;
  }

  vcp_set_prg(LAYER_1, s_retro.vcp1);
  vcp_set_prg(LAYER_2, s_retro.vcp3);

  // We draw top-to-bottom (roughly) in order to be done with the work before the raster beam
  // catches up (to avoid tearing).
  draw_sky(frame_no);
  draw_logo_and_raster_bars(frame_no);
  draw_checkerboard(frame_no);
}

