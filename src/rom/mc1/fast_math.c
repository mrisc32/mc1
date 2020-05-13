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

#include <mc1/fast_math.h>

float fast_sin(float x) {
  // 1) Reduce periods of sin(x) to the range -PI/2 to PI/2.
  const int period = (int)(x * (1.0f / 3.141592654f) + 0.5f);
  const int negate = period & 1;
  x -= 3.141592654f * ((float)period - 0.5f);

  // 2) Use a Tailor series approximation in the range -PI/2 to PI/2.
  // See: https://en.wikipedia.org/wiki/Taylor_series#Approximation_error_and_convergence
  // sin(x) ≃ x - x³/3! + x⁵/5! - x⁷/7!
  // Note: 3! = 6, 5! = 120, 7! = 5040
  const float x2 = x * x;
  const float x3 = x2 * x;
  const float x5 = x3 * x2;
  const float x7 = x5 * x2;
  float y = x - (1.0f/6.0f) * x3 + (1.0f/120.0f) * x5 - (1.0f/5040.0f) * x7;

  return negate ? -y : y;
}

