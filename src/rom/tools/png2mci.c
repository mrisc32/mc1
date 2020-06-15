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

#include "lodepng/lodepng.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//--------------------------------------------------------------------------------------------------
// MCI image file format
// ---------------------
//
// All integer values are stored in little endian format.
//
//  +--------+--------+--------------------------+
//  | Offset | Size   | Description              |
//  +--------+--------+--------------------------+
//  | 0      | 4      | Magic ID ("MCI1")        |
//  | 4      | 2      | Width                    |
//  | 6      | 2      | Height                   |
//  | 8      | 1      | Pixel format:            |
//  |        |        |   0 = RGBA8888           |
//  |        |        |   1 = RGBA5551           |
//  |        |        |   2 = PAL8               |
//  |        |        |   3 = PAL4               |
//  |        |        |   4 = PAL2               |
//  |        |        |   5 = PAL1               |
//  | 9      | 1      | Compression method:      |
//  |        |        |   0 = No compression     |
//  |        |        |   1 = RLE                |
//  | 10     | 2      | Num. palette colors (N)  |
//  | 12     | 4 * N  | Palette (N colors)       |
//  | 12+4*N | 4      | Pixel data size (bytes)  |
//  | 16+4*N | -      | Pixel data               |
//  +--------+--------+--------------------------+
//
// Pixel formats
// -------------
//
// TBD
//
// Palette
// -------
//
// The number of colors in the palette section is determined by the pixel format, according to:
//
//  +----------+------------+
//  | Pix. fmt | No. colors |
//  +----------+------------+
//  | RGBA8888 | 0          |
//  | RGBA5551 | 0          |
//  | PAL8     | 256        |
//  | PAL4     | 16         |
//  | PAL2     | 4          |
//  | PAL1     | 2          |
//  +----------+------------+
//
// The colors in the palette are stored as RGBA8888, i.e. four bytes in the following order:
// red, gree, blue, alpha (a.k.a. ABGR32, when interpreted as a 32-bit little endian word).
//
// Compression methods
// -------------------
//
// TBD
//
//--------------------------------------------------------------------------------------------------

// Pixel formats.
#define PIXFMT_RGBA8888 0
#define PIXFMT_RGBA5551 1
#define PIXFMT_PAL8     2
#define PIXFMT_PAL4     3
#define PIXFMT_PAL2     4
#define PIXFMT_PAL1     5

// Compression methods.
#define COMP_NONE   0
#define COMP_RLE    1

typedef struct {
  unsigned char* pixels;
  size_t pixels_size;
  unsigned width;
  unsigned height;
  unsigned pixfmt;
  unsigned comp_mode;
  uint32_t palette[256];
} image_t;

static int palette_colors_for_pixfmt(const unsigned pixfmt) {
  switch (pixfmt) {
    case PIXFMT_PAL8:
      return 256;
    case PIXFMT_PAL4:
      return 16;
    case PIXFMT_PAL2:
      return 4;
    case PIXFMT_PAL1:
      return 2;
    default:
      return 0;
  }
}

static int bpp_for_pixfmt(const unsigned pixfmt) {
  switch (pixfmt) {
    case PIXFMT_RGBA8888:
      return 32;
    case PIXFMT_RGBA5551:
      return 16;
    case PIXFMT_PAL8:
    default:
      return 8;
    case PIXFMT_PAL4:
      return 4;
    case PIXFMT_PAL2:
      return 2;
    case PIXFMT_PAL1:
      return 1;
  }
}

static uint32_t to_rgba5551(const uint32_t abgr32) {
  const uint32_t r = abgr32 & 255u;
  const uint32_t g = (abgr32 >> 8) & 255u;
  const uint32_t b = (abgr32 >> 16) & 255u;
  const uint32_t a = (abgr32 >> 24) & 255u;
  const uint32_t r5 = r >> 3;
  const uint32_t g5 = g >> 3;
  const uint32_t b5 = b >> 3;
  const uint32_t a1 = a >> 7;
  return (a1 << 15) | (b5 << 10) | (g5 << 5) | r5;
}

static uint32_t calc_color_delta(const uint32_t col1, const uint32_t col2) {
  const int32_t r1 = col1 & 255;
  const int32_t g1 = (col1 >> 8) & 255;
  const int32_t b1 = (col1 >> 16) & 255;
  const int32_t a1 = (col1 >> 24) & 255;

  const int32_t r2 = col2 & 255;
  const int32_t g2 = (col2 >> 8) & 255;
  const int32_t b2 = (col2 >> 16) & 255;
  const int32_t a2 = (col2 >> 24) & 255;

  const int32_t dr = r1 - r2;
  const int32_t dg = g1 - g2;
  const int32_t db = b1 - b2;
  const int32_t da = a1 - a2;

  return (uint32_t)(dr * dr) + (uint32_t)(dg * dg) + (uint32_t)(db * db) + (uint32_t)(da * da);
}

static uint32_t find_best_palette_idx(
    const uint32_t abgr32, const image_t* image, const int pal_size) {
  int best_idx = 0;
  uint32_t best_color_delta = calc_color_delta(abgr32, image->palette[0]);
  for (int i = 1; i < pal_size && best_color_delta > 0u; ++i) {
    const uint32_t color_delta = calc_color_delta(abgr32, image->palette[i]);
    if (color_delta < best_color_delta) {
      best_idx = i;
      best_color_delta = color_delta;
    }
  }
  return (uint32_t)best_idx;
}

static uint32_t to_pal8(const uint32_t abgr32, const image_t* image) {
  return find_best_palette_idx(abgr32, image, 256);
}

static uint32_t to_pal4(const uint32_t abgr32, const image_t* image) {
  return find_best_palette_idx(abgr32, image, 16);
}

static uint32_t to_pal2(const uint32_t abgr32, const image_t* image) {
  return find_best_palette_idx(abgr32, image, 4);
}

static uint32_t to_pal1(const uint32_t abgr32, const image_t* image) {
  return find_best_palette_idx(abgr32, image, 2);
}

static void create_optimal_palette(image_t* image, const unsigned target_pixfmt) {
  const int no_palette_colors = palette_colors_for_pixfmt(target_pixfmt);
  if (no_palette_colors == 0) {
    return;
  }

  // Hack: Just pick out the first N unique colors.
  // TODO(m): Implement me!
  int palette_idx = 0;
  for (unsigned y = 0; y < image->height; ++y) {
    const uint8_t* src_row = &image->pixels[y * image->width * 4];
    for (unsigned x = 0; x < image->height; ++x) {
      const uint32_t abgr32 = ((uint32_t)src_row[0]) |
                              (((uint32_t)src_row[1]) << 8) |
                              (((uint32_t)src_row[2]) << 16) |
                              (((uint32_t)src_row[3]) << 24);
      src_row += 4;

      int already_have_color = 0;
      for (int i = 0; i < palette_idx; ++i) {
        if (image->palette[i] == abgr32) {
          already_have_color = 1;
          break;
        }
      }
      if (!already_have_color) {
        image->palette[palette_idx++] = abgr32;
        if (already_have_color == no_palette_colors) {
          return;
        }
      }
    }
  }
}

static void convert_pixels(image_t* image, const unsigned target_pixfmt) {
  const int bpp = bpp_for_pixfmt(target_pixfmt);

  // Determine the row stride. It must be an even multiple of 32 bits.
  const unsigned words_per_row = (image->width * bpp + 31) / 32;

  // Allocate a new pixel buffer based on the target format.
  const size_t pixels_size = words_per_row * 4 * image->height;
  uint8_t* pixels = (uint8_t*)malloc(pixels_size);
  if (pixels == NULL) {
    fprintf(stderr, "Could not allocate the pixel buffer.\n");
    exit(1);
  }

  // Copy & convert the pixels.
  // TODO(m): Optimize this.
  for (unsigned y = 0; y < image->height; ++y) {
    const uint8_t* src_row = &image->pixels[y * image->width * 4];
    uint8_t* dst_row = &pixels[y * words_per_row * 4];

    uint32_t word = 0u;
    int bits_left_in_word = 32;

    for (unsigned x = 0; x < image->width; ++x) {
      // Read the ABGR32 value from the source image.
      const uint32_t abgr32 = ((uint32_t)src_row[0]) |
                              (((uint32_t)src_row[1]) << 8) |
                              (((uint32_t)src_row[2]) << 16) |
                              (((uint32_t)src_row[3]) << 24);
      src_row += 4;

      // Convert the value to the target format.
      uint32_t pix_value;
      switch (target_pixfmt) {
        case PIXFMT_RGBA8888:
        default:
          pix_value = abgr32;
          break;
        case PIXFMT_RGBA5551:
          pix_value = to_rgba5551(abgr32);
          break;
        case PIXFMT_PAL8:
          pix_value = to_pal8(abgr32, image);
          break;
        case PIXFMT_PAL4:
          pix_value = to_pal4(abgr32, image);
          break;
        case PIXFMT_PAL2:
          pix_value = to_pal2(abgr32, image);
          break;
        case PIXFMT_PAL1:
          pix_value = to_pal1(abgr32, image);
          break;
      }

      // Inject the pixel value into the 32-bit word.
      word = word | (pix_value << (bits_left_in_word - bpp));
      bits_left_in_word -= bpp;

      // Write the word to the output buffer when the word is full.
      if (bits_left_in_word == 0) {
        dst_row[0] = word & 255;
        dst_row[1] = (word >> 8) & 255;
        dst_row[2] = (word >> 16) & 255;
        dst_row[3] = (word >> 24) & 255;
        dst_row += 4;
        word = 0u;
        bits_left_in_word = 32;
      }
    }

    // Write the last word of the row (if padding is needed).
    if (bits_left_in_word != 32) {
      dst_row[0] = word & 255;
      dst_row[1] = (word >> 8) & 255;
      dst_row[2] = (word >> 16) & 255;
      dst_row[3] = (word >> 24) & 255;
    }

  }

  // Update the image with the converted data.
  free(image->pixels);
  image->pixels = pixels;
  image->pixels_size = pixels_size;
  image->pixfmt = target_pixfmt;
}

static void compress_image(image_t* image, unsigned comp_mode) {
  // TODO(m): Implement me!
  if (comp_mode != COMP_NONE) {
    fprintf(stderr, "Support for compression mode %d has not yet been implemented.\n", comp_mode);
    exit(1);
  }

  image->comp_mode = comp_mode;
}

static void write_uint8(const uint32_t x) {
  uint8_t buf[1] = { x & 255u };
  (void) fwrite(&buf[0], 1, 1, stdout);
}

static void write_uint16(const uint32_t x) {
  uint8_t buf[2] = { x & 255u, (x >> 8) & 255u };
  (void) fwrite(&buf[0], 1, 2, stdout);
}

static void write_uint32(const uint32_t x) {
  uint8_t buf[4] = { x & 255u, (x >> 8) & 255u, (x >> 16) & 255u,  (x >> 24) & 255u };
  (void) fwrite(&buf[0], 1, 4, stdout);
}

static void write_image(const image_t* image) {
  const int no_palette_colors = palette_colors_for_pixfmt(image->pixfmt);

  // Write the header.
  write_uint32(0x3149434du);       // Magic ID
  write_uint16(image->width);      // Image width
  write_uint16(image->height);     // Image height
  write_uint8(image->pixfmt);      // Pixel format
  write_uint8(image->comp_mode);   // Compression method
  write_uint16(no_palette_colors); // Number of palette colors

  // Write the palette, if any.
  for (int i = 0; i < no_palette_colors; ++i) {
    write_uint32(image->palette[i]);
  }

  // Write the pixel data.
  write_uint32(image->pixels_size);
  fwrite(image->pixels, 1, image->pixels_size, stdout);
}

int main(int argc, char** argv) {
  // Parse command line arguments.
  int target_pixfmt = PIXFMT_RGBA8888;
  int comp_mode = COMP_NONE;
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s PNGFILE [FORMAT]\n\n", argv[0]);
    fprintf(stderr, "  PNGFILE - The name of the PNG file.\n");
    fprintf(stderr, "  FORMAT  - The output pixel format:\n");
    fprintf(stderr, "              0 = RGBA8888 (default)\n");
    fprintf(stderr, "              1 = RGBA5551\n");
    fprintf(stderr, "              2 = PAL8\n");
    fprintf(stderr, "              3 = PAL4\n");
    fprintf(stderr, "              4 = PAL2\n");
    fprintf(stderr, "              5 = PAL1\n");
    fprintf(stderr, "\nThe generated MCI image is written to stdout.\n");
    exit(1);
  }
  const char* filename = argv[1];
  if (argc >= 3) {
    target_pixfmt = atoi(argv[2]);
    if (target_pixfmt < 0 || target_pixfmt > 5) {
      fprintf(stderr, "Invalid pixel format: %d.\n", target_pixfmt);
      exit(1);
    }
  }

  // Load the PNG image.
  image_t image;
  {
    unsigned error = lodepng_decode32_file(&image.pixels, &image.width, &image.height, filename);
    if (error) {
      fprintf(stderr, "Decoder error %u: %s\n", error, lodepng_error_text(error));
      exit(1);
    }
    image.pixfmt = PIXFMT_RGBA8888;
    image.pixels_size = (size_t)image.width * (size_t)image.height * sizeof(uint32_t);
  }

  // Create an optimal palette.
  create_optimal_palette(&image, target_pixfmt);

  // Convert to the target bit depth.
  convert_pixels(&image, target_pixfmt);

  // Compress the image.
  compress_image(&image, comp_mode);

  // Write the MCI image.
  write_image(&image);

  // Free the memory.
  free(image.pixels);

  return 0;
}

