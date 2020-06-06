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

static inline uint32_t _mr32_cpuid(uint32_t a, uint32_t b) { uint32_t r; asm ("cpuid %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }

// TODO(m): The immediate version of SHUF is much more useful. Make a macro?
static inline uint32_t _mr32_shuf(uint32_t a, uint32_t b) { uint32_t r; asm ("shuf %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }

static inline uint32_t _mr32_clz(uint32_t a) { uint32_t r; asm ("clz %0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint32_t _mr32_rev(uint32_t a) { uint32_t r; asm ("rev %0, %1" : "=r"(r) : "r"(a)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline uint32_t _mr32_clz_b(uint32_t a) { uint32_t r; asm ("clz.b %0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint32_t _mr32_clz_h(uint32_t a) { uint32_t r; asm ("clz.h %0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint32_t _mr32_rev_b(uint32_t a) { uint32_t r; asm ("rev.b %0, %1" : "=r"(r) : "r"(a)); return r; }
static inline uint32_t _mr32_rev_h(uint32_t a) { uint32_t r; asm ("rev.h %0, %1" : "=r"(r) : "r"(a)); return r; }
#endif  // __MRISC32_PACKED_OPS__

static inline int32_t _mr32_min(int32_t a, int32_t b) { int32_t r; asm ("min %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_minu(uint32_t a, uint32_t b) { uint32_t r; asm ("minu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_max(int32_t a, int32_t b) { int32_t r; asm ("max %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_maxu(uint32_t a, uint32_t b) { uint32_t r; asm ("maxu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline uint32_t _mr32_min_b(uint32_t a, uint32_t b) { uint32_t r; asm ("min.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_min_h(uint32_t a, uint32_t b) { uint32_t r; asm ("min.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_minu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("minu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_minu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("minu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_max_b(uint32_t a, uint32_t b) { uint32_t r; asm ("max.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_max_h(uint32_t a, uint32_t b) { uint32_t r; asm ("max.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_maxu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("maxu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_maxu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("maxu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__

static inline int32_t _mr32_mulq(int32_t a, int32_t b) { int32_t r; asm ("mulq %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline uint32_t _mr32_mulq_b(uint32_t a, uint32_t b) { uint32_t r; asm ("mulq.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_mulq_h(uint32_t a, uint32_t b) { uint32_t r; asm ("mulq.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_mul_b(uint32_t a, uint32_t b) { uint32_t r; asm ("mul.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_mul_h(uint32_t a, uint32_t b) { uint32_t r; asm ("mul.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_mulhi_b(uint32_t a, uint32_t b) { uint32_t r; asm ("mulhi.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_mulhi_h(uint32_t a, uint32_t b) { uint32_t r; asm ("mulhi.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_mulhiu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("mulhiu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_mulhiu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("mulhiu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__

#ifdef __MRISC32_DIV__
#ifdef __MRISC32_PACKED_OPS__
static inline uint32_t _mr32_div_b(uint32_t a, uint32_t b) { uint32_t r; asm ("div.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_div_h(uint32_t a, uint32_t b) { uint32_t r; asm ("div.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_divu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("divu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_divu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("divu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_rem_b(uint32_t a, uint32_t b) { uint32_t r; asm ("rem.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_rem_h(uint32_t a, uint32_t b) { uint32_t r; asm ("rem.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_remu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("remu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_remu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("remu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_DIV__

#ifdef __MRISC32_SATURATING_OPS__
static inline int32_t _mr32_adds(int32_t a, int32_t b) { int32_t r; asm ("adds %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addsu(uint32_t a, uint32_t b) { uint32_t r; asm ("addsu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_addh(int32_t a, int32_t b) { int32_t r; asm ("addh %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addhu(uint32_t a, uint32_t b) { uint32_t r; asm ("addhu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_subs(int32_t a, int32_t b) { int32_t r; asm ("subs %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subsu(uint32_t a, uint32_t b) { uint32_t r; asm ("subsu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline int32_t _mr32_subh(int32_t a, int32_t b) { int32_t r; asm ("subh %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subhu(uint32_t a, uint32_t b) { uint32_t r; asm ("subhu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#ifdef __MRISC32_PACKED_OPS__
static inline uint32_t _mr32_adds_b(uint32_t a, uint32_t b) { uint32_t r; asm ("adds.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_adds_h(uint32_t a, uint32_t b) { uint32_t r; asm ("adds.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addsu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("addsu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addsu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("addsu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addh_b(uint32_t a, uint32_t b) { uint32_t r; asm ("addh.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addh_h(uint32_t a, uint32_t b) { uint32_t r; asm ("addh.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addhu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("addhu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_addhu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("addhu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subs_b(uint32_t a, uint32_t b) { uint32_t r; asm ("subs.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subs_h(uint32_t a, uint32_t b) { uint32_t r; asm ("subs.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subsu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("subsu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subsu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("subsu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subh_b(uint32_t a, uint32_t b) { uint32_t r; asm ("subh.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subh_h(uint32_t a, uint32_t b) { uint32_t r; asm ("subh.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subhu_b(uint32_t a, uint32_t b) { uint32_t r; asm ("subhu.b %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
static inline uint32_t _mr32_subhu_h(uint32_t a, uint32_t b) { uint32_t r; asm ("subhu.h %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); return r; }
#endif  // __MRISC32_PACKED_OPS__
#endif  // __MRISC32_SATURATING_OPS__

#endif  // __MRISC32__

#endif  // MR32INTRIN_H_

