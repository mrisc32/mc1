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

#ifndef MC1_MCI_DECODE_H_
#define MC1_MCI_DECODE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pixel formats.
#define MCI_PIXFMT_RGBA8888 0
#define MCI_PIXFMT_RGBA5551 1
#define MCI_PIXFMT_PAL8     2
#define MCI_PIXFMT_PAL4     3
#define MCI_PIXFMT_PAL2     4
#define MCI_PIXFMT_PAL1     5

// Compression methods.
#define MCI_COMP_NONE   0
#define MCI_COMP_LZG    1

typedef struct {
  uint32_t magic;            ///< Magic ID (must be 0x3149434d).
  uint16_t width;            ///< Image width (in pixels).
  uint16_t height;           ///< Image height (in pixels).
  uint8_t pixel_format;      ///< Pixel format.
  uint8_t compression;       ///< Compression method.
  uint16_t num_pal_colors;   ///< Number of palette colors (0 for RGBA8888 and RGBA5551).
  uint32_t pixel_data_size;  ///< Size of the (possibly compressed) pixel data.
} mci_header_t;

/// @brief Retrieve the header info from an MCI buffer.
/// @param mci_data The MCI data buffer.
/// @returns a pointer to the header, or NULL if the buffer is not a valid, aligned MCI data.
const mci_header_t* mci_get_header(const uint8_t* mci_data);

/// @brief Get the number of bytes per row.
/// @param hdr The MCI header (must be valid!).
/// @returns the byte stride for one row.
uint32_t mci_get_stride(const mci_header_t* hdr);

/// @brief Get the number of bytes required for the pixel data.
/// @param hdr The MCI header (must be valid!).
/// @returns the number of bytes that the uncompressed pixel data will occupy in memory.
uint32_t mci_get_pixels_size(const mci_header_t* hdr);

/// @brief Decode palette data from an MCI buffer.
/// @param mci_data The MCI data buffer.
/// @param[out] palette The target palette buffer.
void mci_decode_palette(const uint8_t* mci_data, uint32_t* palette);

/// @brief Decode pixel data from an MCI buffer.
/// @param mci_data The MCI data buffer.
/// @param[out] pixels The target pixel buffer.
void mci_decode_pixels(const uint8_t* mci_data, uint32_t* pixels);

#ifdef __cplusplus
}
#endif

#endif  // MC1_MCI_DECODE_H_

