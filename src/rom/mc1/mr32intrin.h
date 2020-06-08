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

#ifndef MR32INTRIN_H_
#define MR32INTRIN_H_

#ifdef __MRISC32__

#include <stdint.h>

// TODO(m): This should be defined by the compiler. Right now we have to assume that all MRISC32
// CPU:s support saturating/halving operations.
#define __MRISC32_SATURATING_OPS__ 1

#ifdef __MRISC32_PACKED_OPS__
// We use specific typedefs for all supported packed data types, mostly for documentation purposes.
typedef uint32_t int8x4_t;
typedef uint32_t int16x2_t;
typedef uint32_t uint8x4_t;
typedef uint32_t uint16x2_t;
typedef uint32_t float8x4_t;
typedef uint32_t float16x2_t;
#endif  // __MRISC32_PACKED_OPS__

static inline uint32_t _mr32_cpuid(uint32_t a, uint32_t b) { uint32_t r; asm ("cpuid\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }

#ifdef __MRISC32_PACKED_OPS__
static inline int8x4_t _mr32_add_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("add.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_add_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("add.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_sub_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("sub.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_sub_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("sub.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__

#ifdef __MRISC32_PACKED_OPS__
static inline uint8x4_t _mr32_seq_b(int8x4_t a, int8x4_t b) { uint8x4_t r; asm ("seq.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_seq_h(int16x2_t a, int16x2_t b) { uint16x2_t r; asm ("seq.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_sne_b(int8x4_t a, int8x4_t b) { uint8x4_t r; asm ("sne.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_sne_h(int16x2_t a, int16x2_t b) { uint16x2_t r; asm ("sne.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_slt_b(int8x4_t a, int8x4_t b) { uint8x4_t r; asm ("slt.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_slt_h(int16x2_t a, int16x2_t b) { uint16x2_t r; asm ("slt.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_sltu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("sltu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_sltu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("sltu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_sle_b(int8x4_t a, int8x4_t b) { uint8x4_t r; asm ("sle.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_sle_h(int16x2_t a, int16x2_t b) { uint16x2_t r; asm ("sle.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_sleu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("sleu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_sleu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("sleu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__

// TODO(m): Keep or drop the word-sized min/max intrinsics? They can be generated with regular C
// code. If we keep them, we should have immediate variants too.
static inline int32_t _mr32_min(int32_t a, int32_t b) { int32_t r; asm ("min\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_minu(uint32_t a, uint32_t b) { uint32_t r; asm ("minu\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_max(int32_t a, int32_t b) { int32_t r; asm ("max\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_maxu(uint32_t a, uint32_t b) { uint32_t r; asm ("maxu\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline int8x4_t _mr32_min_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("min.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_min_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("min.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_minu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("minu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_minu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("minu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_max_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("max.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_max_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("max.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_maxu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("maxu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_maxu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("maxu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__

#ifdef __MRISC32_PACKED_OPS__
static inline int8x4_t _mr32_asr_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("asr.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_asr_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("asr.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_lsl_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("lsl.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_lsl_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("lsl.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_lsr_b(uint8x4_t a, int8x4_t b) { uint8x4_t r; asm ("lsr.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_lsr_h(uint16x2_t a, int16x2_t b) { uint16x2_t r; asm ("lsr.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__

// Create a control word for use with the SHUF instruction.
//  sign_mode - 0 = zero-fill, 1 = sign-fill
//  selN      - A 3-bit selector for byte N of the result word
//              Bit 2:    Copy/fill mode (0 = copy, 1 = fill)
//              Bits 0-1: Source byte index (0 = least signficant byte, 3 = most significant byte)
#define _MR32_SHUFCTL(sign_mode, sel3, sel2, sel1, sel0) \
  (((((uint32_t)(sign_mode)) & 1u) << 12) | \
   ((((uint32_t)(sel3)) & 7u) << 9) | ((((uint32_t)(sel2)) & 7u) << 6) | \
   ((((uint32_t)(sel1)) & 7u) << 3) | (((uint32_t)(sel0)) & 7u))

// Note: The second argument (the control word) must be a numeric constant. _MR32_SHUFCTL() can be
// used for creating a valid control word.
static inline uint32_t _mr32_shuf(uint32_t a, const uint32_t b) { uint32_t r; asm ("shuf\t%0, %1, #%2" : "=r"(r) : "r"(a), "i"(b)); return r; }

static inline int32_t _mr32_mulq(int32_t a, int32_t b) { int32_t r; asm ("mulq\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline int8x4_t _mr32_mulq_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("mulq.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_mulq_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("mulq.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_mul_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("mul.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_mul_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("mul.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_mulhi_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("mulhi.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_mulhi_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("mulhi.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_mulhiu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("mulhiu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_mulhiu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("mulhiu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__

#ifdef __MRISC32_DIV__
#ifdef __MRISC32_PACKED_OPS__
static inline int8x4_t _mr32_div_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("div.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_div_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("div.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_divu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("divu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_divu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("divu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_rem_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("rem.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_rem_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("rem.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_remu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("remu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_remu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("remu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_DIV__

#ifdef __MRISC32_SATURATING_OPS__
static inline int32_t _mr32_adds(int32_t a, int32_t b) { int32_t r; asm ("adds\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addsu(uint32_t a, uint32_t b) { uint32_t r; asm ("addsu\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_addh(int32_t a, int32_t b) { int32_t r; asm ("addh\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addhu(uint32_t a, uint32_t b) { uint32_t r; asm ("addhu\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_subs(int32_t a, int32_t b) { int32_t r; asm ("subs\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subsu(uint32_t a, uint32_t b) { uint32_t r; asm ("subsu\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_subh(int32_t a, int32_t b) { int32_t r; asm ("subh\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subhu(uint32_t a, uint32_t b) { uint32_t r; asm ("subhu\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline int8x4_t _mr32_adds_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("adds.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_adds_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("adds.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_addsu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("addsu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_addsu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("addsu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_addh_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("addh.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_addh_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("addh.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_addhu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("addhu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_addhu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("addhu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_subs_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("subs.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_subs_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("subs.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_subsu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("subsu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_subsu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("subsu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_subh_b(int8x4_t a, int8x4_t b) { int8x4_t r; asm ("subh.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_subh_h(int16x2_t a, int16x2_t b) { int16x2_t r; asm ("subh.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_subhu_b(uint8x4_t a, uint8x4_t b) { uint8x4_t r; asm ("subhu.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_subhu_h(uint16x2_t a, uint16x2_t b) { uint16x2_t r; asm ("subhu.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_SATURATING_OPS__

static inline uint32_t _mr32_clz(uint32_t a) { uint32_t r; asm ("clz\t%0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint32_t _mr32_rev(uint32_t a) { uint32_t r; asm ("rev\t%0, %1" : "=r"(r) : "r"(a)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline uint8x4_t _mr32_clz_b(uint8x4_t a) { uint8x4_t r; asm ("clz.b\t%0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint16x2_t _mr32_clz_h(uint16x2_t a) { uint16x2_t r; asm ("clz.h\t%0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint8x4_t _mr32_rev_b(uint8x4_t a) { uint8x4_t r; asm ("rev.b\t%0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint16x2_t _mr32_rev_h(uint16x2_t a) { uint16x2_t r; asm ("rev.h\t%0, %1" : "=r"(r) : "r"(a)); return r; }
#endif  // __MRISC32_PACKED_OPS__

#ifdef __MRISC32_HARD_FLOAT__
static inline float _mr32_fmin(float a, float b) { float r; asm ("fmin\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float _mr32_fmax(float a, float b) { float r; asm ("fmax\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline float8x4_t _mr32_fmin_b(float8x4_t a, float8x4_t b) { float8x4_t r; asm ("fmin.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_fmin_h(float16x2_t a, float16x2_t b) { float16x2_t r; asm ("fmin.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float8x4_t _mr32_fmax_b(float8x4_t a, float8x4_t b) { float8x4_t r; asm ("fmax.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_fmax_h(float16x2_t a, float16x2_t b) { float16x2_t r; asm ("fmax.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_HARD_FLOAT__

#ifdef __MRISC32_HARD_FLOAT__
#ifdef __MRISC32_PACKED_OPS__
static inline uint8x4_t _mr32_fseq_b(float8x4_t a, float8x4_t b) { uint8x4_t r; asm ("fseq.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_fseq_h(float16x2_t a, float16x2_t b) { uint16x2_t r; asm ("fseq.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_fsne_b(float8x4_t a, float8x4_t b) { uint8x4_t r; asm ("fsne.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_fsne_h(float16x2_t a, float16x2_t b) { uint16x2_t r; asm ("fsne.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_fslt_b(float8x4_t a, float8x4_t b) { uint8x4_t r; asm ("fslt.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_fslt_h(float16x2_t a, float16x2_t b) { uint16x2_t r; asm ("fslt.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_fsle_b(float8x4_t a, float8x4_t b) { uint8x4_t r; asm ("fsle.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_fsle_h(float16x2_t a, float16x2_t b) { uint16x2_t r; asm ("fsle.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_HARD_FLOAT__

#ifdef __MRISC32_HARD_FLOAT__
static inline float _mr32_itof(int32_t a, int32_t b) { float r; asm ("itof\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float _mr32_utof(uint32_t a, int32_t b) { float r; asm ("utof\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_ftoi(float a, int32_t b) { int32_t r; asm ("ftoi\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_ftou(float a, int32_t b) { uint32_t r; asm ("ftou\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_ftoir(float a, int32_t b) { int32_t r; asm ("ftoir\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_ftour(float a, int32_t b) { uint32_t r; asm ("ftour\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline float8x4_t _mr32_itof_b(int8x4_t a, int8x4_t b) { float8x4_t r; asm ("itof.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_itof_h(int16x2_t a, int16x2_t b) { float16x2_t r; asm ("itof.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float8x4_t _mr32_utof_b(uint8x4_t a, int8x4_t b) { float8x4_t r; asm ("utof.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_utof_h(uint16x2_t a, int16x2_t b) { float16x2_t r; asm ("utof.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_ftoi_b(float8x4_t a, int8x4_t b) { int8x4_t r; asm ("ftoi.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_ftoi_h(float16x2_t a, int16x2_t b) { int16x2_t r; asm ("ftoi.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_ftou_b(float8x4_t a, int8x4_t b) { uint8x4_t r; asm ("ftou.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_ftou_h(float16x2_t a, int16x2_t b) { uint16x2_t r; asm ("ftou.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int8x4_t _mr32_ftoir_b(float8x4_t a, int8x4_t b) { int8x4_t r; asm ("ftoir.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int16x2_t _mr32_ftoir_h(float16x2_t a, int16x2_t b) { int16x2_t r; asm ("ftoir.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint8x4_t _mr32_ftour_b(float8x4_t a, int8x4_t b) { uint8x4_t r; asm ("ftour.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint16x2_t _mr32_ftour_h(float16x2_t a, int16x2_t b) { uint16x2_t r; asm ("ftour.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_HARD_FLOAT__

#ifdef __MRISC32_HARD_FLOAT__
#ifdef __MRISC32_PACKED_OPS__
static inline float8x4_t _mr32_fadd_b(float8x4_t a, float8x4_t b) { float8x4_t r; asm ("fadd.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_fadd_h(float16x2_t a, float16x2_t b) { float16x2_t r; asm ("fadd.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float8x4_t _mr32_fsub_b(float8x4_t a, float8x4_t b) { float8x4_t r; asm ("fsub.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_fsub_h(float16x2_t a, float16x2_t b) { float16x2_t r; asm ("fsub.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float8x4_t _mr32_fmul_b(float8x4_t a, float8x4_t b) { float8x4_t r; asm ("fmul.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_fmul_h(float16x2_t a, float16x2_t b) { float16x2_t r; asm ("fmul.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float8x4_t _mr32_fdiv_b(float8x4_t a, float8x4_t b) { float8x4_t r; asm ("fdiv.b\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline float16x2_t _mr32_fdiv_h(float16x2_t a, float16x2_t b) { float16x2_t r; asm ("fdiv.h\t%0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_HARD_FLOAT__

#endif  // __MRISC32__

#endif  // MR32INTRIN_H_

