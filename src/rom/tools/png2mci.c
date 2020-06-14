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
//  | 4      | 2      | Pixel format:            |
//  |        |        |   0 = RGBA8888           |
//  |        |        |   1 = RGBA5551           |
//  |        |        |   2 = PAL8               |
//  |        |        |   3 = PAL4               |
//  |        |        |   4 = PAL2               |
//  |        |        |   5 = PAL1               |
//  | 6      | 2      | Compression method:      |
//  |        |        |   0 = No compression     |
//  |        |        |   1 = RLE                |
//  | 8      | 4      | Width                    |
//  | 12     | 4      | Height                   |
//  | 16     | 4 * N  | Palette (N colors)       |
//  | 16+4*N | 4      | Pixel data size (bytes)  |
//  | 20+4*N | -      | Pixel data               |
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

int palette_colors_for_pixfmt(unsigned pixfmt) {
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

void convert_pixels(image_t* image, unsigned target_pixfmt) {
  // TODO(m): Implement me!
  if (target_pixfmt != PIXFMT_RGBA8888) {
    fprintf(stderr, "Support for color format %d has not yet been implemented.\n", target_pixfmt);
    exit(1);
  }

  image->pixfmt = target_pixfmt;
}

void compress_image(image_t* image, unsigned comp_mode) {
  // TODO(m): Implement me!
  if (comp_mode != COMP_NONE) {
    fprintf(stderr, "Support for compression mode %d has not yet been implemented.\n", comp_mode);
    exit(1);
  }

  image->comp_mode = comp_mode;
}

void write_uint16(const uint32_t x) {
  uint8_t buf[2] = { x & 255u, (x >> 8) & 255u };
  (void) fwrite(&buf[0], 1, 2, stdout);
}

void write_uint32(const uint32_t x) {
  uint8_t buf[4] = { x & 255u, (x >> 8) & 255u, (x >> 16) & 255u,  (x >> 24) & 255u };
  (void) fwrite(&buf[0], 1, 4, stdout);
}

void write_image(const image_t* image) {
  // Write the header.
  write_uint32(0x3149434du);       // Magic ID
  write_uint16(image->pixfmt);     // Pixel format
  write_uint16(image->comp_mode);  // Compression method
  write_uint32(image->width);      // Image width
  write_uint32(image->height);     // Image height

  // Write the palette, if any.
  const int no_palette_colors = palette_colors_for_pixfmt(image->pixfmt);
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

