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

#ifndef ROM_MOSAIC_HPP_
#define ROM_MOSAIC_HPP_

#include <mc1/mmio.h>
#include <mc1/vcp.h>

#include <mr32intrin.h>

#include <cstdint>

// Note: Using an anonymous namespace saves a few bytes of code size.
namespace {

// Mosaic background class.
class mosaic_t {
public:
  void* init(void* mem) {
    // "Allocate" memory.
    auto* pixels = reinterpret_cast<uint32_t*>(mem);
    auto* vcp_start = &pixels[MOSAIC_W * MOSAIC_H];

    // Get the HW resolution.
    const auto native_width = MMIO(VIDWIDTH);
    const auto native_height = MMIO(VIDHEIGHT);

    // VCP prologue.
    auto* vcp = vcp_start;
    *vcp++ = vcp_emit_setreg(VCR_XINCR, (0x010000 * MOSAIC_W) / native_width);
    *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_RGBA8888);

    // Address pointers.
    uint32_t vcp_pixels_addr = to_vcp_addr(reinterpret_cast<uintptr_t>(pixels));
    *vcp++ = vcp_emit_waity(0);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, native_width);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
    for (int k = 1; k < MOSAIC_H; ++k) {
      auto y = (static_cast<uint32_t>(k) * native_height) / static_cast<uint32_t>(MOSAIC_H);
      vcp_pixels_addr += MOSAIC_W;
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
    }

    // VCP epilogue: Wait forever.
    *vcp++ = vcp_emit_waity(32767);

    // Set up the VCP address.
    vcp_set_prg(LAYER_1, vcp_start);

    m_pixels = pixels;

    return reinterpret_cast<void*>(vcp);
  }

  void deinit() {
    vcp_set_prg(LAYER_1, nullptr);
  }

  void update(const uint32_t t) {
    // Define the four corner colors.
    abgr32_t p11 = make_color(t);
    abgr32_t p12 = make_color(t + 3433U);
    abgr32_t p21 = make_color(1150U - t);
    abgr32_t p22 = make_color(t + 13150U);

    // Interpolate all the "pixels" (tiles) in the mosaic.
    uint32_t* pixels = m_pixels;
    for (int y = 0; y < MOSAIC_H; ++y) {
      uint32_t wy = (y << 8) / MOSAIC_H;
      abgr32_t p1 = lerp(p11, p21, wy);
      abgr32_t p2 = lerp(p12, p22, wy);
      for (int x = 0; x < MOSAIC_W; ++x) {
        uint32_t wx = (x << 8) / MOSAIC_W;
        *pixels++ = lerp(p1, p2, wx);
      }
    }
  }

private:
  // Color type.
  using abgr32_t = uint32_t;

  static const int MOSAIC_W = 16;
  static const int MOSAIC_H = (MOSAIC_W * 9) / 16;

  static abgr32_t lerp(const abgr32_t c1, const abgr32_t c2, uint32_t w2) {
    uint32_t w1 = 255U - w2;
#ifdef __MRISC32_PACKED_OPS__
    uint8x4_t w1p = _mr32_shuf(w1, _MR32_SHUFCTL(0, 0, 0, 0, 0));  // Splat
    uint8x4_t w2p = _mr32_shuf(w2, _MR32_SHUFCTL(0, 0, 0, 0, 0));
    return _mr32_mulhiu_b(w1p, c1) + _mr32_mulhiu_b(w2p, c2);
#else
    uint32_t br = ((w1 * (c1 & 0xff00ffU) + w2 * (c2 & 0xff00ffU)) >> 8) & 0xff00ffU;
    uint32_t g = ((w1 * (c1 & 0x00ff00U) + w2 * (c2 & 0x00ff00U)) >> 8) & 0x00ff00U;
    return br | g;
#endif
  }

  static uint32_t tri_wave(uint32_t t) {
    uint32_t t_mod = t & 511U;
    return t_mod <= 255U ? t_mod : 511U - t_mod;
  }

  static abgr32_t make_color(uint32_t t) {
    uint32_t tr = t;
    uint32_t tg = t + 90U;
    uint32_t tb = 160U - t;
    uint32_t r = tri_wave(tr);
    uint32_t g = tri_wave(tg);
    uint32_t b = tri_wave(tb);
    return r | (g << 8) | (b << 16);
  }

  uint32_t* m_pixels;
};

}  // namespace

#endif  // ROM_MOSAIC_HPP_
