// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2022 Marcus Geelnard
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

#ifndef ROM_SPLASH_HPP_
#define ROM_SPLASH_HPP_

#include "fp32.hpp"

#include <mc1/mci_decode.h>
#include <mc1/mmio.h>
#include <mc1/vcp.h>

#include <cstdint>

// The boot splash image is linked in from a separate file.
extern const unsigned char boot_splash_mci[] __attribute__((aligned(4)));

// Note: Using an anonymous namespace saves a few bytes of code size.
namespace {

// Splash display class.
class splash_t {
public:
  void* init(void* mem) {
    // Decode the MCI header.
    auto* hdr = mci_get_header(boot_splash_mci);
    const auto pixels_size = mci_get_pixels_size(hdr);
    m_num_palette_colors = hdr->num_pal_colors;
    m_img_width = hdr->width;
    m_img_height = hdr->height;
    m_img_fmt = hdr->pixel_format;
    m_img_word_stride = mci_get_stride(hdr) / 4;

    // "Allocate" memory.
    m_pixels = reinterpret_cast<uint32_t*>(mem);
    m_vcp = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(mem) + pixels_size);

    // Decode the pixels.
    mci_decode_pixels(boot_splash_mci, m_pixels);

    // Generate the VCP.
    auto* mem_end = generate_vcp(scale_for_t(0));

    // Set up the VCP address.
    vcp_set_prg(LAYER_2, m_vcp);

    return mem_end;
  }

  void deinit() {
    vcp_set_prg(LAYER_2, nullptr);
  }

  void update(const uint32_t t) {
    (void)generate_vcp(scale_for_t(t));
  }

private:
  static fp32_t scale_for_t(const uint32_t t) {
    // Scaling as a function of time: Simulate an x^2 "bouncing" motion.
    auto t_mod = t & 127U;
    if (t_mod >= 64U) {
      t_mod = 127U - t_mod;
    }
    return 0.75_fp32 + 0.000126_fp32 * ((63U * 63U) - (t_mod * t_mod));
  }

  void* generate_vcp(fp32_t scale_for_1080p) {
    // Get the HW resolution and adjust the scaling factor.
    const auto native_width = MMIO(VIDWIDTH);
    const auto native_height = MMIO(VIDHEIGHT);
    auto scale = (scale_for_1080p * native_height) / static_cast<uint32_t>(1080);

    // Calculate the screen rectangle for the splash (centered, preserve aspect ratio).
    const auto view_height = static_cast<uint32_t>(scale * m_img_height);
    const auto view_width = static_cast<uint32_t>(scale * m_img_width);
    const auto view_top = (native_height - view_height) / 2U;
    const auto view_left = (native_width - view_width) / 2U;

    auto* vcp = m_vcp;

    // We add a wait here, and add a few NOP:s (to fill up the pipeline after the WAITY instruction)
    // so that the VCP modifications that we do during the blanking interval has effect.
    *vcp++ = vcp_emit_waity(0);
    *vcp++ = vcp_emit_nop();
    *vcp++ = vcp_emit_nop();
    *vcp++ = vcp_emit_nop();

    // VCP prologue.
    // TODO(m): Use the fixed point width from the scaling and set VCR_XOFFS too for subpixel
    // accuracy.
    *vcp++ = vcp_emit_setreg(VCR_XINCR, (0x010000U * m_img_width) / view_width);
    *vcp++ = vcp_emit_setreg(VCR_CMODE, m_img_fmt);

    // Palette.
    *vcp++ = vcp_emit_setpal(0, m_num_palette_colors);
    m_palette = vcp;
    mci_decode_palette(boot_splash_mci, vcp);
    vcp += m_num_palette_colors;

    // Address pointers.
    *vcp++ = vcp_emit_waity(view_top);
    *vcp++ = vcp_emit_setreg(VCR_HSTRT, view_left);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, view_left + view_width);
    uint32_t vcp_pixels_addr = to_vcp_addr(reinterpret_cast<uintptr_t>(m_pixels));
    const auto vcp_pixels_stride = m_img_word_stride;
    auto y = fp32_t(view_top);
    const auto y_step = fp32_t(view_height) / m_img_height;
    for (uint32_t k = 0U; k < m_img_height; ++k) {
      *vcp++ = vcp_emit_waity(static_cast<uint32_t>(y));
      *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
      y += y_step;
      vcp_pixels_addr += vcp_pixels_stride;
    }
    *vcp++ = vcp_emit_waity(static_cast<uint32_t>(y));
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);

    // VCP epilogue: Wait forever.
    *vcp++ = vcp_emit_waity(32767);

    return reinterpret_cast<void*>(vcp);
  }

  uint32_t* m_pixels;
  uint32_t* m_vcp;
  uint32_t* m_palette;
  uint32_t m_num_palette_colors;
  uint32_t m_img_width;
  uint32_t m_img_height;
  uint32_t m_img_fmt;
  uint32_t m_img_word_stride;
};

}  // namespace

#endif  // ROM_SPLASH_HPP_
