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

#ifndef ROM_FP32_HPP_
#define ROM_FP32_HPP_

#include <cstdint>

// Note: Using an anonymous namespace saves a few bytes of code size.
namespace {

// A simple fixed-point class for manipulating unsigned values.
// Note: This is mostly to avoid using floating-point instructions in the ROM code.
//
// The internal fixed point format is 12.20 bits, which gives us a valid range of
// 0.000000 - 4095.999999 and 6 decimals precision. This format is suitable for representing 2D
// screen coordinates and sizes.
//
// Usage example:
//
//  uint32_t a = 3434U;
//  uint32_t b = 12U;
//  uint32_t c = static_cast<uint32_t>((0.24_fp32 * a) / b);

class fp32_t {
public:
  explicit fp32_t(uint32_t i) : m_fpbits(i << FP_SHIFT) {
  }
  constexpr explicit fp32_t(long double& d) : m_fpbits(to_fpbits(d)) {
  }

  operator uint32_t() const {
    // Rounding cast.
    return (m_fpbits + (1U << (FP_SHIFT - 1U))) >> FP_SHIFT;
  }

  fp32_t& operator+=(const fp32_t y) {
    m_fpbits += y.m_fpbits;
    return *this;
  }
  fp32_t& operator*=(const uint32_t y) {
    m_fpbits *= y;
    return *this;
  }
  fp32_t& operator/=(const uint32_t y) {
    // Rounding division.
    m_fpbits = (m_fpbits + (y >> 1U)) / y;
    return *this;
  }

private:
  static constexpr uint32_t FP_SHIFT = 20U;

  static constexpr uint32_t to_fpbits(long double& d) {
    auto i = static_cast<uint32_t>(d);
    auto f = static_cast<uint32_t>((d - static_cast<long double>(i)) *
                                   static_cast<long double>(1U << FP_SHIFT));
    return (i << FP_SHIFT) | f;
  }

  uint32_t m_fpbits;
};

constexpr fp32_t operator""_fp32(long double x) {
  return fp32_t(x);
}

fp32_t operator+(fp32_t x, const fp32_t& y) {
  x += y;
  return x;
}
fp32_t operator*(fp32_t x, const uint32_t& y) {
  x *= y;
  return x;
}
fp32_t operator/(fp32_t x, const uint32_t& y) {
  x /= y;
  return x;
}

}  // namespace

#endif  // ROM_FP32_HPP_
