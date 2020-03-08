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

#include <mc1/framebuffer.h>

#include <mc1/memory.h>
#include <mc1/mmio.h>


//--------------------------------------------------------------------------------------------------
// Private.
//--------------------------------------------------------------------------------------------------

#define REG_ADDR 0
#define REG_XOFFS 1
#define REG_XINCR 2
#define REG_HSTRT 3
#define REG_HSTOP 4
#define REG_CMODE 5
#define REG_RMODE 6

static uint32_t emit_jmp(const uint32_t addr) {
  return 0x00000000u | addr;
}

static uint32_t emit_waity(const int y) {
  return 0x50000000u | (0x0000ffffu & (uint32_t)y);
}

static uint32_t emit_setpal(const uint32_t first, const uint32_t count) {
  return 0x60000000u | (first << 8u) | (count - 1u);
}

static uint32_t emit_setreg(const uint32_t reg, const uint32_t value) {
  return 0x80000000u | (reg << 24u) | value;
}

static uint32_t to_vcp_addr(const uint32_t cpu_addr) {
  return (cpu_addr - VRAM_START) / 4u;
}

static void set_current_vcp(const void* vcp_addr) {
  uint32_t* base_vcp = (uint32_t*)VRAM_START;
  *base_vcp = emit_jmp(to_vcp_addr((uint32_t)vcp_addr));
}

static size_t bits_per_pixel(color_mode_t mode) {
  switch (mode) {
    case MODE_RGBA8888:
      return 32;
    case MODE_RGBA5551:
      return 16;
    case MODE_PAL8:
      return 8;
    case MODE_PAL4:
      return 4;
    case MODE_PAL2:
      return 2;
    case MODE_PAL1:
      return 1;
    default:
      return 0;
  }
}

static size_t palette_entries(color_mode_t mode) {
  switch (mode) {
    case MODE_PAL8:
      return 256;
    case MODE_PAL4:
      return 16;
    case MODE_PAL2:
      return 4;
    case MODE_PAL1:
      return 2;
    default:
      return 0;
  }
}

static size_t calc_stride(int width, color_mode_t mode) {
  // We round up to the nearest word size.
  return 4u * ((bits_per_pixel(mode) * (size_t)width + 31u) / 32u);
}

static size_t calc_pixels_size(int width, int height, color_mode_t mode) {
  return calc_stride(width, mode) * (size_t)height;
}

static size_t calc_vcp_size(int height, color_mode_t mode) {
  size_t prologue_words = 3;

  size_t palette_words = palette_entries(mode);
  if (palette_words > 0u)
    ++prologue_words;

  size_t row_words = 1 + height * 2;

  size_t epilogue_words = 1;

  return (prologue_words + palette_words + row_words + epilogue_words) * 4;
}


//--------------------------------------------------------------------------------------------------
// Public.
//--------------------------------------------------------------------------------------------------

fb_t* fb_create(int width, int height, color_mode_t mode) {
  // Sanity check input parameters.
  size_t bpp = bits_per_pixel(mode);
  if (width < 1 || height < 1 || bpp < 1) {
    return NULL;
  }

  // Allocate memory for the framebuffer and supporting data structures.
  const size_t vcp_size = calc_vcp_size(height, mode);
  const size_t pix_size = calc_pixels_size(width, height, mode);
  const size_t total_size = sizeof(fb_t) + vcp_size + pix_size;
  fb_t* fb = (fb_t*)mem_alloc(total_size, MEM_TYPE_VIDEO | MEM_CLEAR);
  if (!fb) {
    return NULL;
  }

  // Populate the fb_t object fields.
  {
    uint8_t* ptr = (uint8_t*)fb;
    fb->vcp = (void*)&ptr[sizeof(fb_t)];
    fb->pixels = (void*)&ptr[sizeof(fb_t) + vcp_size];
  }
  fb->stride = calc_stride(width, mode);
  fb->width = width;
  fb->height = height;
  fb->mode = mode;

  // Get the native width and height of the video signal.
  const uint32_t native_width = MMIO(VIDWIDTH);
  const uint32_t native_height = MMIO(VIDHEIGHT);

  uint32_t* vcp = (uint32_t*)fb->vcp;

  // VCP prologue.
  *vcp++ = emit_setreg(REG_XINCR, (0x010000 * width) / native_width);
  *vcp++ = emit_setreg(REG_CMODE, mode);
  *vcp++ = emit_setreg(REG_RMODE, 1);

  // Palette.
  size_t pal_N = palette_entries(mode);
  if (pal_N > 0u) {
    *vcp++ = emit_setpal(0, pal_N);
    fb->palette = (void*)vcp;
    for (uint32_t k = 0; k < pal_N; ++k) {
      *vcp++ = ((k * 255u) / pal_N) * 0x01010101u;
    }
  }

  // Address pointers.
  uint32_t vcp_fb_addr = to_vcp_addr((uint32_t)fb->pixels);
  *vcp++ = emit_waity(0);
  *vcp++ = emit_setreg(REG_HSTOP, native_width);
  *vcp++ = emit_setreg(REG_ADDR, vcp_fb_addr);
  const uint32_t vcp_fb_stride = fb->stride / 4u;
  for (int k = 1; k < height; ++k) {
    uint32_t y = ((uint32_t)k * native_height) / (uint32_t)height;
    vcp_fb_addr += vcp_fb_stride;
    *vcp++ = emit_waity(y);
    *vcp++ = emit_setreg(REG_ADDR, vcp_fb_addr);
  }

  // Wait forever.
  *vcp++ = emit_waity(32767);

  return fb;
}

void fb_destroy(fb_t* fb) {
  mem_free(fb);
}

void fb_show(fb_t* fb) {
  if (fb) {
    set_current_vcp(fb->vcp);
  }
}

