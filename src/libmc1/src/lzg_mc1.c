// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2010-2020 Marcus Geelnard
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

//--------------------------------------------------------------------------------------------------
// This is a stripped down and adapted version of lzgmini.c from the liblzg project.
//--------------------------------------------------------------------------------------------------

#include <mc1/lzg_mc1.h>

#include <mr32intrin.h>

//-- PRIVATE ---------------------------------------------------------------------------------------

// Define to enable safety checks (increases code size).
//#define CONF_DO_CHECKS 1

// Internal definitions.
#define LZG_HEADER_SIZE 16
#define LZG_METHOD_COPY 0
#define LZG_METHOD_LZG1 1

// Endian and alignment independent reader for 32-bit integers.
static uint32_t _LZG_GetUINT32(const uint8_t* in, const int offs) {
  const uint32_t b3 = (uint32_t)in[offs];
  const uint32_t b2 = (uint32_t)in[offs + 1];
  const uint32_t b1 = (uint32_t)in[offs + 2];
  const uint32_t b0 = (uint32_t)in[offs + 3];
#ifdef __MRISC32_PACKED_OPS__
  return _mr32_pack_h(_mr32_pack(b3, b1), _mr32_pack(b2, b0));
#else
  return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
#endif
}

// Get the minimum integer value.
static inline uint32_t _LZG_Min(const uint32_t a, const uint32_t b) {
  return a < b ? a : b;
}

#ifdef CONF_DO_CHECKS
// Calculate the checksum.
static uint32_t _LZG_CalcChecksum(const uint8_t* data, uint32_t size) {
  uint16_t a = 1, b = 0;
  const uint8_t* end = ((const uint8_t*)data) + size;
  while (data != end) {
    a += *data++;
    b += a;
  }
  return (((uint32_t)b) << 16) | a;
}
#endif  // CONF_DO_CHECKS

// LUT for decoding the copy length parameter.
static const uint8_t _LZG_LENGTH_DECODE_LUT[32] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                                                   13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                                   24, 25, 26, 27, 28, 29, 35, 48, 72, 128};

//-- PUBLIC ----------------------------------------------------------------------------------------

uint32_t LZG_Decode(const uint8_t* in,
                    const uint32_t insize,
                    uint8_t* out,
                    const uint32_t outsize) {
#ifdef CONF_DO_CHECKS
  // Check magic ID.
  if ((insize < LZG_HEADER_SIZE) || (in[0] != 'L') || (in[1] != 'Z') || (in[2] != 'G')) {
    return 0;
  }
#endif  // CONF_DO_CHECKS

  // Get header data.
  const uint32_t decoded_size = _LZG_GetUINT32(in, 3);
#ifdef CONF_DO_CHECKS
  const uint32_t encoded_size = _LZG_GetUINT32(in, 7);
  const uint32_t checksum = _LZG_GetUINT32(in, 11);
#endif  // CONF_DO_CHECKS

#ifdef CONF_DO_CHECKS
  // Check sizes.
  if ((outsize < decoded_size) || (encoded_size != (insize - LZG_HEADER_SIZE))) {
    return 0;
  }

  // Check checksum.
  if (_LZG_CalcChecksum(&in[LZG_HEADER_SIZE], encoded_size) != checksum) {
    return 0;
  }
#endif  // CONF_DO_CHECKS

  // Initialize the byte streams.
  const uint8_t* src = ((const uint8_t*)in) + LZG_HEADER_SIZE;
  const uint8_t* in_end = ((const uint8_t*)in) + insize;
  uint8_t* dst = out;
  const uint8_t* out_end = out + outsize;

  // Check which method to use.
  const uint32_t method = (uint32_t)in[15];
  if (method == LZG_METHOD_LZG1) {
#ifdef CONF_DO_CHECKS
    if (!((src + 4) <= in_end)) {
      return 0;
    }
#endif
    const uint32_t m1 = (uint32_t)*src++;
    const uint32_t m2 = (uint32_t)*src++;
    const uint32_t m3 = (uint32_t)*src++;
    const uint32_t m4 = (uint32_t)*src++;

    // Main decompression loop.
    while (src < in_end) {
      const uint8_t symbol = *src++;

      if ((symbol != m1) && (symbol != m2) && (symbol != m3) && (symbol != m4)) {
        // Literal copy.
#ifdef CONF_DO_CHECKS
        if (!(dst < out_end)) {
          return 0;
        }
#endif
        *dst++ = symbol;
      } else {
        // Decode offset / length parameters.
#ifdef CONF_DO_CHECKS
        if (!(src < in_end)) {
          return 0;
        }
#endif
        const uint32_t b = (uint32_t)*src++;
        if (b != 0) {
          uint32_t length, offset;
          if (symbol == m1) {
            // Distant copy.
#ifdef CONF_DO_CHECKS
            if (!((src + 2) <= in_end)) {
              return 0;
            }
#endif
            length = _LZG_LENGTH_DECODE_LUT[b & 0x1f];
            const uint32_t b2 = (uint32_t)*src++;
            offset = ((b & 0xe0) << 11) | (b2 << 8) | (*src++);
            offset += 2056;
          } else if (symbol == m2) {
            // Medium copy.
#ifdef CONF_DO_CHECKS
            if (!(src < in_end)) {
              return 0;
            }
#endif
            length = _LZG_LENGTH_DECODE_LUT[b & 0x1f];
            const uint32_t b2 = (uint32_t)*src++;
            offset = ((b & 0xe0) << 3) | b2;
            offset += 8;
          } else if (symbol == m3) {
            // Short copy.
            length = (b >> 6) + 3;
            offset = (b & 0x3f) + 8;
          } else {
            // Near copy (including RLE).
            length = _LZG_LENGTH_DECODE_LUT[b & 0x1f];
            offset = (b >> 5) + 1;
          }

          // Copy the corresponding data from the history window.
          const uint8_t* copy = dst - offset;
#ifdef CONF_DO_CHECKS
          if (!((copy >= out) && ((dst + length) <= out_end))) {
            return 0;
          }
#endif
          for (uint32_t i = 0u; i < length; ++i) {
            *dst++ = *copy++;
          }
        } else {
          // Literal copy (single occurance of a marker symbol).
#ifdef CONF_DO_CHECKS
          if (!(dst < out_end)) {
            return 0;
          }
#endif
          *dst++ = symbol;
        }
      }
    }
  } else if (method == LZG_METHOD_COPY) {
    // Plain copy.
    const uint32_t count = _LZG_Min((uint32_t)(in_end - src), (uint32_t)(out_end - dst));
    for (uint32_t i = 0u; i < count; ++i) {
      *dst++ = *src++;
    }

  }

#ifdef CONF_DO_CHECKS
  // All OK?
  if ((uint32_t)(dst - out) != decoded_size) {
    return 0;
  }
#endif

  return decoded_size;
}
