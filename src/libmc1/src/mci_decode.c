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

#include <mc1/mci_decode.h>

#include <mc1/lzg_mc1.h>

#include <string.h>

//--------------------------------------------------------------------------------------------------
// MCI image file format:
//
//  +---------------------------------------------+
//  | Header (16 bytes)                           |
//  +---------+--------+--------------------------+
//  | Offset  | Size   | Description              |
//  +---------+--------+--------------------------+
//  | 0       | 4      | Magic ID ("MCI1")        |
//  | 4       | 2      | Width                    |
//  | 6       | 2      | Height                   |
//  | 8       | 1      | Pixel format             |
//  | 9       | 1      | Compression method       |
//  | 10      | 2      | Num. palette colors (Nc) |
//  | 12      | 4      | Pixel data bytes (Nb)    |
//  +---------+--------+--------------------------+
//
//  +---------------------------------------------+
//  | Data                                        |
//  +---------+--------+--------------------------+
//  | Offset  | Size   | Description              |
//  +---------+--------+--------------------------+
//  | 16      | 4 * Nc | Palette (Nc colors)      |
//  | 16+4*Nc | Nb     | Pixel data (Nb bytes)    |
//  +---------+--------+--------------------------+
//
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Private.
//--------------------------------------------------------------------------------------------------

static int is_word_aligned(const uint8_t* ptr) {
  return (((uint32_t)ptr) & 3u) == 0u;
}

static int has_magic_id(const uint8_t* ptr) {
  // The magic ID is "MCI1", or 0x3149434d in hex (little endian).
  return *((const uint32_t*)ptr) == 0x3149434du;
}

static const uint32_t* get_palette_data(const mci_header_t* hdr) {
  const uint8_t* base = (const uint8_t*)hdr;
  return (const uint32_t*)(base + sizeof(mci_header_t));
}

static const uint8_t* get_pixel_data(const mci_header_t* hdr) {
  const uint8_t* base = (const uint8_t*)hdr;
  return base + sizeof(mci_header_t) + 4 * hdr->num_pal_colors;
}


//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

const mci_header_t* mci_get_header(const uint8_t* mci_data) {
  // Sanity check.
  if (mci_data == NULL || !is_word_aligned(mci_data) || !has_magic_id(mci_data)) {
    return NULL;
  }

  return (const mci_header_t*)mci_data;
}

uint32_t mci_get_stride(const mci_header_t* hdr) {
  const uint32_t width = hdr->width;
  const uint32_t bpp = 32u >> hdr->pixel_format;
  return ((width * bpp + 31u) / 32u) * 4u;
}

uint32_t mci_get_pixels_size(const mci_header_t* hdr) {
  return mci_get_stride(hdr) * hdr->height;
}

void mci_decode_palette(const uint8_t* mci_data, uint32_t* palette) {
  const mci_header_t* hdr = mci_get_header(mci_data);
  if (hdr == NULL) {
    return;
  }

  // Copy the palette data.
  memcpy(palette, get_palette_data(hdr), 4u * (uint32_t)hdr->num_pal_colors);
}

void mci_decode_pixels(const uint8_t* mci_data, uint32_t* pixels) {
  const mci_header_t* hdr = mci_get_header(mci_data);
  if (hdr == NULL) {
    return;
  }

  // Get pixel data info.
  const uint8_t* pixel_data = get_pixel_data(hdr);
  const uint32_t unpacked_size = mci_get_pixels_size(hdr);

  // Uncompress the pixel data.
  if (hdr->compression == MCI_COMP_NONE) {
    memcpy(pixels, pixel_data, unpacked_size);
  } else if (hdr->compression == MCI_COMP_LZG) {
    LZG_Decode(pixel_data, hdr->pixel_data_size, (uint8_t*)pixels, unpacked_size);
  }
}

