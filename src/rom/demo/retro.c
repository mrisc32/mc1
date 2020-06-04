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

#include <mc1/mmio.h>
#include <mc1/vcp.h>

#include <stddef.h>
#include <stdint.h>

typedef struct {
  void* base_ptr;
  uint32_t* vcp1;
  uint32_t* vcp2;
  uint8_t* pixels1;
  int32_t width;
  int32_t height;
} retro_t;

static retro_t s_retro;


//--------------------------------------------------------------------------------------------------
// Drawing routines.
//--------------------------------------------------------------------------------------------------

static void render_layer1(const int frame_no) {
  uint32_t* vcp = s_retro.vcp1 + 3;
  for (int y = 0; y < s_retro.height; ++y) {
    uint32_t color = ((uint32_t)(uint8_t)(frame_no - y)) * 0x01000201u;
    *vcp = color;
    vcp += 3;
  }
}

static void render_layer2(const int frame_no) {
  uint32_t* vcp = s_retro.vcp2 + 3;
  for (int y = 0; y < s_retro.height; ++y) {
    uint32_t alpha = ((uint32_t)(uint8_t)(y * 2 + frame_no)) * 0x01000000u;
    uint32_t color = ((uint32_t)(uint8_t)(y + frame_no * 3)) * 0x00010102u;
    *vcp = alpha | color;
    vcp += 3;
  }
}

static void render(const int frame_no) {
  render_layer1(frame_no);
  render_layer2(frame_no);
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

  // VCP for layer 1.
  const size_t vcp1_size = 4 * (1 + s_retro.height * 3 + 1);

  // VCP for layer 2.
  const size_t vcp2_size = 4 * (1 + s_retro.height * 3 + 1);

  // Pixels for layer 1.
  const size_t pix1_size = 16;

  // Calculate the required memory size.
  const size_t total_size = vcp1_size + vcp2_size + pix1_size;

  uint8_t* mem = (uint8_t*)mem_alloc(total_size, MEM_TYPE_VIDEO | MEM_CLEAR);
  if (mem == NULL) {
    return;
  }
  s_retro.base_ptr = mem;

  s_retro.vcp1 = (uint32_t*)(mem);
  s_retro.vcp2 = (uint32_t*)(mem + vcp1_size);
  s_retro.pixels1 = mem + vcp1_size + vcp2_size;

  // Create the layer 1 VCP.
  {
    uint32_t* vcp = s_retro.vcp1;

    // Prologue.
    // Set the dither mode.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);

    // Per-line commands.
    for (int y = 0; y < s_retro.height; ++y) {
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setpal(0, 1);
      ++vcp;  // Palette color 0
    }

    // Epilogue.
    *vcp = vcp_emit_waity(32767);
  }

  // Create the layer 2 VCP.
  {
    uint32_t* vcp = s_retro.vcp2;

    // Prologue.
    // Set the blend mode.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x135);

    // Per-line commands.
    for (int y = 0; y < s_retro.height; ++y) {
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setpal(0, 1);
      ++vcp;  // Palette color 0
    }

    // Epilogue.
    *vcp = vcp_emit_waity(32767);
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
  vcp_set_prg(LAYER_2, s_retro.vcp2);

  render(frame_no);
}

