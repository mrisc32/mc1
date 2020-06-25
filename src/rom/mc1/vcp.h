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

#ifndef MC1_VCP_H_
#define MC1_VCP_H_

#include <mc1/memory.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LAYER_1 = 1,
  LAYER_2 = 2
} layer_t;

// Video Control Registers (VCR:s).
#define VCR_ADDR 0
#define VCR_XOFFS 1
#define VCR_XINCR 2
#define VCR_HSTRT 3
#define VCR_HSTOP 4
#define VCR_CMODE 5
#define VCR_RMODE 6

// Color modes.
#define CMODE_RGBA8888 0
#define CMODE_RGBA5551 1
#define CMODE_PAL8     2
#define CMODE_PAL4     3
#define CMODE_PAL2     4
#define CMODE_PAL1     5

/// @brief Emit a JMP instruction.
/// @param addr The address to jump to (in VCP address space).
/// @returns the instruction word.
static inline uint32_t vcp_emit_jmp(const uint32_t addr) {
  return 0x00000000u | addr;
}

/// @brief Emit a JSR instruction.
/// @param addr The address to jump to (in VCP address space).
/// @returns the instruction word.
static inline uint32_t vcp_emit_jsr(const uint32_t addr) {
  return 0x10000000u | addr;
}

/// @brief Emit an RTS instruction.
/// @returns the instruction word.
static inline uint32_t vcp_emit_rts() {
  return 0x20000000u;
}

/// @brief Emit a NOP instruction.
/// @returns the instruction word.
static inline uint32_t vcp_emit_nop() {
  return 0x30000000u;
}

/// @brief Emit a WAITX instruction.
/// @param x The x coordinate to wait for (signed).
/// @returns the instruction word.
static inline uint32_t vcp_emit_waitx(const int x) {
  return 0x40000000u | (0x0000ffffu & (uint32_t)x);
}

/// @brief Emit a WAITY instruction.
/// @param y The y coordinate to wait for (signed).
/// @returns the instruction word.
static inline uint32_t vcp_emit_waity(const int y) {
  return 0x50000000u | (0x0000ffffu & (uint32_t)y);
}

/// @brief Emit a SETPAL instruction.
/// @param first The first palette entry to set (0-255).
/// @param count The number of palette entries to set (must be >= 1).
/// @returns the instruction word.
static inline uint32_t vcp_emit_setpal(const uint32_t first, const uint32_t count) {
  return 0x60000000u | (first << 8u) | (count - 1u);
}

/// @brief Emit a SETREG instruction.
/// @param reg The register to set.
/// @param value The new value of the register.
/// @returns the instruction word.
static inline uint32_t vcp_emit_setreg(const uint32_t reg, const uint32_t value) {
  return 0x80000000u | (reg << 24u) | value;
}

/// @brief Convert a CPU address to a VCP address.
/// @param cpu_addr The address in CPU address space.
/// @returns the address in VCP address space.
static inline uint32_t to_vcp_addr(const uintptr_t cpu_addr) {
  return (uint32_t)((cpu_addr - (uintptr_t)VRAM_START) / 4u);
}

/// @brief Set the VCP for the given layer.
/// @param layer The layer to set (LAYER_1 or LAYER_2).
/// @param prg The VCP to use (NULL for no program).
void vcp_set_prg(const layer_t layer, const uint32_t* prg);

#ifdef __cplusplus
}
#endif

#endif  // MC1_VCP_H_

