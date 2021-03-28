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

#include <mc1/vcp.h>

void vcp_set_prg(const layer_t layer, const uint32_t* prg) {
  if (layer < LAYER_1 || layer > LAYER_2) {
    return;
  }

  uint32_t* base_vcp = (uint32_t*)(VRAM_START + 16 * layer);
  if (prg != NULL) {
    // Jump to the given VCP.
    *base_vcp = vcp_emit_jmp(to_vcp_addr((uintptr_t)prg));
  } else {
    // Create a "clean screen" VCP that sets the background color to fully transparent black and
    // waits forever.
    *base_vcp++ = vcp_emit_setpal(0, 1);
    *base_vcp++ = 0x00000000u;
    *base_vcp = vcp_emit_waity(32767);
  }
}

