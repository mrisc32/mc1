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

#ifndef MC1_FAST_MATH_H_
#define MC1_FAST_MATH_H_

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
// Type punning functions. These are essentialy no-ops (zero cycles) on MRISC32.
//--------------------------------------------------------------------------------------------------

static inline int32_t bitcast_float_to_int(const float x) {
  int32_t y;
  memcpy(&y, &x, sizeof(y));
  return y;
}

static inline float bitcast_int_to_float(const int32_t x) {
  float y;
  memcpy(&y, &x, sizeof(y));
  return y;
}


//--------------------------------------------------------------------------------------------------
// Fast approximate implementations of useful math functions.
//--------------------------------------------------------------------------------------------------

static inline float fast_rsqrt(const float x) {
  // See: https://en.wikipedia.org/wiki/Fast_inverse_square_root
  float x2 = x * 0.5f;
  int32_t i = bitcast_float_to_int(x);
  i = 0x5f3759df - (i >> 1);
  float y = bitcast_int_to_float(i);
  y = y * (1.5f - (x2 * y * y));
  //y = y * (1.5f - (x2 * y * y));
  return y;
}

static inline float fast_sqrt(const float x) {
  return x * fast_rsqrt(x);
}

float fast_sin(float x);

static inline float fast_cos(const float x) {
  // cos(x) = sin(x + PI/2)
  return fast_sin(x + 1.570796327f);
}

static inline float fast_pow2(const float p) {
  float clipp = (p < -126.0f) ? -126.0f : p;
  return bitcast_int_to_float((int32_t)((1 << 23) * (clipp + 126.94269504f)));
}

static inline float fast_log2(const float x) {
  int32_t xi = bitcast_float_to_int(x);
  float mx = bitcast_int_to_float((xi & 0x007FFFFF) | 0x3f000000);
  float y = 1.1920928955078125e-7f * (float)xi;
  return y - 124.22551499f - 1.498030302f * mx - 1.72587999f / (0.3520887068f + mx);
}

static inline float fast_pow(const float x, const float p) {
  return fast_pow2(p * fast_log2(x));
}

#ifdef __cplusplus
}
#endif

#endif  // MC1_FAST_MATH_H_

