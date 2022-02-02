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
    auto* mem_end = generate_vcp(DEFAULT_TARGET_VIEW_HEIGHT);

    // Set up the VCP address.
    vcp_set_prg(LAYER_2, m_vcp);

    return mem_end;
  }

  void deinit() {
    vcp_set_prg(LAYER_2, nullptr);
  }

private:
  // Splash view height, on a 1080p screen.
  static const uint32_t DEFAULT_TARGET_VIEW_HEIGHT = 436;

  void* generate_vcp(uint32_t target_view_height) {
    // Get the HW resolution.
    const auto native_width = MMIO(VIDWIDTH);
    const auto native_height = MMIO(VIDHEIGHT);

    // Calculate the screen rectangle for the splash (centered, preserve aspect ratio).
    const auto view_height = (target_view_height * native_height) / 1080U;
    const auto view_width = (m_img_width * view_height) / m_img_height;
    const auto view_top = (native_height - view_height) / 2;
    const auto view_left = (native_width - view_width) / 2;

    // VCP prologue.
    auto* vcp = m_vcp;
    *vcp++ = vcp_emit_setreg(VCR_XINCR, (0x010000 * m_img_width) / view_width);
    *vcp++ = vcp_emit_setreg(VCR_CMODE, m_img_fmt);

    // Palette.
    *vcp++ = vcp_emit_setpal(0, m_num_palette_colors);
    m_palette = vcp;
    mci_decode_palette(boot_splash_mci, vcp);
    vcp += m_num_palette_colors;

    // Address pointers.
    uint32_t vcp_pixels_addr = to_vcp_addr(reinterpret_cast<uintptr_t>(m_pixels));
    *vcp++ = vcp_emit_waity(view_top);
    *vcp++ = vcp_emit_setreg(VCR_HSTRT, view_left);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, view_left + view_width);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
    const auto vcp_stride = m_img_word_stride;
    for (uint32_t k = 1U; k < m_img_height; ++k) {
      auto y = view_top + (k * view_height) / m_img_height;
      vcp_pixels_addr += vcp_stride;
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
    }
    *vcp++ = vcp_emit_waity(view_top + view_height);
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
