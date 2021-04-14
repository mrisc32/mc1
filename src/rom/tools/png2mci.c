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

#include <lzg.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------------------------------------------
// MCI image file format
// ---------------------
//
// All integer values are stored in little endian format.
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

// Palette modes.
#define PAL_OPTIMAL   0
#define PAL_GRAYSCALE 1

// Compression methods.
#define COMP_NONE   0
#define COMP_LZG    1

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} rgba_t;

typedef struct {
  unsigned char* pixels;
  size_t pixels_size;
  unsigned width;
  unsigned height;
  unsigned pixfmt;
  unsigned comp_mode;
  rgba_t palette[256];
} image_t;

typedef struct {
  size_t first;
  size_t count;
  rgba_t min_col;
  rgba_t max_col;
  uint32_t volume;
} color_box_t;

static rgba_t get_rgba(const uint8_t* buf) {
  rgba_t col = { buf[0], buf[1], buf[2], buf[3] };
  return col;
}

static int rgba_eq(const rgba_t col1, const rgba_t col2) {
  return (col1.r == col2.r) && (col1.g == col2.g) && (col1.b == col2.b) && (col1.a == col2.a);
}

static int rgba_diff(const rgba_t col1, const rgba_t col2) {
  const int dr = (int)col1.r - (int)col2.r;
  const int dg = (int)col1.g - (int)col2.g;
  const int db = (int)col1.b - (int)col2.b;
  const int da = (int)col1.a - (int)col2.a;

  return (dr * dr) + (dg * dg) + (db * db) + (da * da);
}

static uint8_t minu8(const uint8_t a, const uint8_t b) {
  return a < b ? a : b;
}

static uint8_t maxu8(const uint8_t a, const uint8_t b) {
  return a > b ? a : b;
}

static rgba_t rgba_min(const rgba_t col1, const rgba_t col2) {
  rgba_t col = { minu8(col1.r, col2.r),
                 minu8(col1.g, col2.g),
                 minu8(col1.b, col2.b),
                 minu8(col1.a, col2.a) };
  return col;
}

static rgba_t rgba_max(const rgba_t col1, const rgba_t col2) {
  rgba_t col = { maxu8(col1.r, col2.r),
                 maxu8(col1.g, col2.g),
                 maxu8(col1.b, col2.b),
                 maxu8(col1.a, col2.a) };
  return col;
}

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

static uint32_t to_rgba8888(const rgba_t col) {
  return ((uint32_t)col.r) |
         (((uint32_t)col.g) << 8) |
         (((uint32_t)col.b) << 16) |
         (((uint32_t)col.a) << 24);
}

static uint32_t to_rgba5551(const rgba_t col) {
  const uint32_t r5 = (uint32_t)(col.r >> 3);
  const uint32_t g5 = (uint32_t)(col.g >> 3);
  const uint32_t b5 = (uint32_t)(col.b >> 3);
  const uint32_t a1 = (uint32_t)(col.a >> 7);
  return (a1 << 15) | (b5 << 10) | (g5 << 5) | r5;
}

static uint32_t find_best_palette_idx(const rgba_t col, const image_t* image, const int pal_size) {
  int best_idx = 0;
  int best_color_diff = rgba_diff(col, image->palette[0]);
  for (int i = 1; i < pal_size && best_color_diff > 0; ++i) {
    const int color_diff = rgba_diff(col, image->palette[i]);
    if (color_diff < best_color_diff) {
      best_idx = i;
      best_color_diff = color_diff;
    }
  }
  return (uint32_t)best_idx;
}

static uint32_t to_pal8(const rgba_t col, const image_t* image) {
  return find_best_palette_idx(col, image, 256);
}

static uint32_t to_pal4(const rgba_t col, const image_t* image) {
  return find_best_palette_idx(col, image, 16);
}

static uint32_t to_pal2(const rgba_t col, const image_t* image) {
  return find_best_palette_idx(col, image, 4);
}

static uint32_t to_pal1(const rgba_t col, const image_t* image) {
  return find_best_palette_idx(col, image, 2);
}

int compare_rgba_r(const void* item1, const void* item2) {
  return (int)(((const rgba_t*)item1)->r) - (int)(((const rgba_t*)item2)->r);
}

int compare_rgba_g(const void* item1, const void* item2) {
  return (int)(((const rgba_t*)item1)->g) - (int)(((const rgba_t*)item2)->g);
}

int compare_rgba_b(const void* item1, const void* item2) {
  return (int)(((const rgba_t*)item1)->b) - (int)(((const rgba_t*)item2)->b);
}

int compare_rgba_a(const void* item1, const void* item2) {
  return (int)(((const rgba_t*)item1)->a) - (int)(((const rgba_t*)item2)->a);
}

static rgba_t calc_representative_color(rgba_t* col_array, const size_t count) {
  // Simple algorithm: Just calculate the mean color.
  uint32_t r = 0;
  uint32_t g = 0;
  uint32_t b = 0;
  uint32_t a = 0;
  for (size_t i = 0; i < count; ++i) {
    r += (uint32_t)col_array[i].r;
    g += (uint32_t)col_array[i].g;
    b += (uint32_t)col_array[i].b;
    a += (uint32_t)col_array[i].a;
  }
  const uint32_t round = (uint32_t)(count / 2u);
  r = (r + round) / (uint32_t)count;
  g = (g + round) / (uint32_t)count;
  b = (b + round) / (uint32_t)count;
  a = (a + round) / (uint32_t)count;
  rgba_t col = { r, g, b, a };
  return col;
}

static void update_box_bounds(const rgba_t* col_array, color_box_t* box) {
  // Find the min and max bounds along all axes.
  box->min_col = box->max_col = col_array[box->first];
  for (size_t i = 1; i < box->count; ++i) {
    box->min_col = rgba_min(box->min_col, col_array[box->first + i]);
    box->max_col = rgba_max(box->max_col, col_array[box->first + i]);
  }

  // Calculate the box volume.
  const uint32_t dr = 1 + (uint32_t)(box->max_col.r - box->min_col.r);
  const uint32_t dg = 1 + (uint32_t)(box->max_col.g - box->min_col.g);
  const uint32_t db = 1 + (uint32_t)(box->max_col.b - box->min_col.b);
  const uint32_t da = 1 + (uint32_t)(box->max_col.a - box->min_col.a);
  box->volume = dr * dr + dg * dg + db * db + da * da;
}

static void median_cut(rgba_t* col_array,
                       const size_t count,
                       rgba_t* palette,
                       const int num_palette_colors) {
  if (count == 0) {
    return;
  }
  if (num_palette_colors > 256) {
    fprintf(stderr, "Woot!\n");
    exit(1);
  }

  color_box_t boxes[256];

  // Initiate the first box.
  boxes[0].first = 0;
  boxes[0].count = count;
  update_box_bounds(col_array, &boxes[0]);
  int num_boxes = 1;

  // Keep splitting those boxes until we have enough.
  while (num_boxes < num_palette_colors) {
    // Find the largest box.
    int largest_box_idx = -1;
    for (int i = 0; i < num_boxes; ++i) {
      if (boxes[i].count > 1 &&
          (largest_box_idx == -1 || boxes[i].volume > boxes[largest_box_idx].volume)) {
        largest_box_idx = i;
      }
    }
    if (largest_box_idx == -1) {
      // No suitable box was found, so we're done.
      break;
    }
    color_box_t* largest_box = &boxes[largest_box_idx];

    // Which dimension has the largest span?
    const int dr = largest_box->max_col.r - largest_box->min_col.r;
    const int dg = largest_box->max_col.g - largest_box->min_col.g;
    const int db = largest_box->max_col.b - largest_box->min_col.b;
    const int da = largest_box->max_col.a - largest_box->min_col.a;
    int (*cmp_fun)(const void*,const void*) = compare_rgba_r;
    int max_delta = dr;
    if (dg > max_delta) {
      cmp_fun = compare_rgba_g;
      max_delta = dg;
    }
    if (db > max_delta) {
      cmp_fun = compare_rgba_b;
      max_delta = db;
    }
    if (da > max_delta) {
      cmp_fun = compare_rgba_a;
    }

    // Sort colors along the given dimension.
    qsort(&col_array[largest_box->first], largest_box->count, sizeof(rgba_t), cmp_fun);

    // Do the split.
    color_box_t* box1 = largest_box;
    color_box_t* box2 = &boxes[num_boxes++];
    box2->count = box1->count / 2;
    box1->count = box1->count - box2->count;
    box2->first = box1->first + box1->count;
    update_box_bounds(col_array, box1);
    update_box_bounds(col_array, box2);
  }

  // Use the boxes to create a palette.
  for (int i = 0; i < num_boxes; ++i) {
    const color_box_t* box = &boxes[i];
    palette[i] = calc_representative_color(&col_array[box->first], box->count);
  }
}

static int compare_rgba(const void* item1, const void* item2) {
  const rgba_t col1 = *(const rgba_t*)item1;
  const rgba_t col2 = *(const rgba_t*)item2;
  if (col1.a != col2.a) {
    return col1.a - col2.a;
  }
  // We approximate brightness (~= 0.3*R + 0.6*G + 0.1*B) scaled by the alpha channel.
  const int intensity1 = ((int)col1.a) * (3 * (int)col1.r + 6 * (int)col1.g + 1 * (int)col1.b);
  const int intensity2 = ((int)col2.a) * (3 * (int)col2.r + 6 * (int)col2.g + 1 * (int)col2.b);
  return intensity1 - intensity2;
}

static int is_fully_transparent(const rgba_t col, const uint8_t threshold) {
  return col.a <= threshold;
}

static int is_opaque_black(const rgba_t col, const uint8_t threshold) {
  return col.r <= threshold && col.g <= threshold && col.b <= threshold &&
         col.a >= (255u - threshold);
}

static int is_opaque_white(const rgba_t col, const uint8_t threshold) {
  return col.r >= (255u - threshold) && col.g >= (255u - threshold) &&
         col.b >= (255u - threshold) && col.a >= (255u - threshold);
}

static void create_palette(image_t* image, const unsigned target_pixfmt, const int palette_mode) {
  const int no_palette_colors = palette_colors_for_pixfmt(target_pixfmt);
  if (no_palette_colors == 0) {
    return;
  }

  // Grayscale?
  if (palette_mode == PAL_GRAYSCALE) {
    // Generate a plain grayscale palette.
    for (int i = 0; i < no_palette_colors; ++i) {
      uint32_t g = (unsigned)(i * 255) / (unsigned)(no_palette_colors - 1);
      rgba_t rgba = {g, g, g, 255};
      image->palette[i] = rgba;
    }

    // Done!
    return;
  }

  // Extract all image colors into an array of colors.
  size_t num_pixels = (size_t)image->width * (size_t)image->height;
  rgba_t* col_array = (rgba_t*)malloc(sizeof(rgba_t) * num_pixels);
  if (col_array == NULL) {
    fprintf(stderr, "Unable to allocate work memory for color quantization.\n");
    exit(1);
  }
  int has_fully_transparent = 0;
  int has_opaque_black = 0;
  int has_opaque_white = 0;
  {
    const uint8_t* src = image->pixels;
    rgba_t* dst = col_array;
    for (unsigned y = 0; y < image->height; ++y) {
      for (unsigned x = 0; x < image->width; ++x) {
        const rgba_t col = get_rgba(src);
        has_fully_transparent = has_fully_transparent || is_fully_transparent(col, 1u);
        has_opaque_black = has_opaque_black || is_opaque_black(col, 1u);
        has_opaque_white = has_opaque_white || is_opaque_white(col, 1u);
        *dst++ = col;
        src += 4;
      }
    }
  }

  // Remove duplicate colors from the image.
  // TODO(m): Keep counts as weights?
  qsort(col_array, num_pixels, sizeof(rgba_t), compare_rgba);
  {
    size_t current_idx = 0;
    for (size_t i = 1; i < num_pixels; ++i) {
      if (!rgba_eq(col_array[current_idx], col_array[i])) {
        col_array[++current_idx] = col_array[i];
      }
    }
    num_pixels = current_idx + 1;
  }

  // Apply median cut to produce a palette.
  median_cut(col_array, num_pixels, image->palette, no_palette_colors);

  // Free the color array (we're done with it).
  free(col_array);

  // Sort the palette (darkest most transparent colors first).
  qsort(image->palette, no_palette_colors, sizeof(rgba_t), compare_rgba);

  // Remove duplicate colors in the palette.
  // Note: Since the palette is now sorted, duplicates are next to each other.
  int actual_palette_colors;
  {
    int current_idx = 0;
    for (int i = 1; i < no_palette_colors; ++i) {
      if (!rgba_eq(image->palette[current_idx], image->palette[i])) {
        image->palette[++current_idx] = image->palette[i];
      }
    }
    actual_palette_colors = current_idx + 1;
    const rgba_t white = { 255, 255, 255, 255 };
    for (int i = actual_palette_colors; i < no_palette_colors; ++i) {
      image->palette[++current_idx] = white;
    }
  }

  // Re-insert special colors if necessary.
  if (no_palette_colors > 2) {
    int lacks_fully_transparent = has_fully_transparent;
    int lacks_opaque_black = has_opaque_black;
    int lacks_opaque_white = has_opaque_white;
    for (int i = 0; i < actual_palette_colors; ++i) {
      const rgba_t col = image->palette[i];
      lacks_fully_transparent = lacks_fully_transparent && !is_fully_transparent(col, 0u);
      lacks_opaque_black = lacks_opaque_black && !is_opaque_black(col, 0u);
      lacks_opaque_white = lacks_opaque_white && !is_opaque_white(col, 0u);
    }
    if (lacks_fully_transparent) {
      const rgba_t col = { 0, 0, 0, 0 };
      image->palette[0] = col;
    }
    if (lacks_opaque_white) {
      const rgba_t col = { 255, 255, 255, 255 };
      if (actual_palette_colors < no_palette_colors) {
        image->palette[actual_palette_colors++] = col;
      } else {
        image->palette[actual_palette_colors - 1] = col;
      }
    }
    if (lacks_opaque_black) {
      const rgba_t col = { 0, 0, 0, 255 };
      if (actual_palette_colors < no_palette_colors) {
        image->palette[actual_palette_colors++] = col;
      } else {
        const int idx = find_best_palette_idx(col, image, actual_palette_colors);
        image->palette[idx] = col;
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
  for (unsigned y = 0; y < image->height; ++y) {
    const uint8_t* src = &image->pixels[y * image->width * 4];
    uint8_t* dst = &pixels[y * words_per_row * 4];

    uint32_t word = 0u;
    int bits_left_in_word = 32;
    int shift = 0;

    for (unsigned x = 0; x < image->width; ++x) {
      // Read the color value from the source image.
      const rgba_t col = get_rgba(src);
      src += 4;

      // Convert the value to the target format.
      uint32_t pix_value;
      switch (target_pixfmt) {
        case PIXFMT_RGBA8888:
        default:
          pix_value = to_rgba8888(col);
          break;
        case PIXFMT_RGBA5551:
          pix_value = to_rgba5551(col);
          break;
        case PIXFMT_PAL8:
          pix_value = to_pal8(col, image);
          break;
        case PIXFMT_PAL4:
          pix_value = to_pal4(col, image);
          break;
        case PIXFMT_PAL2:
          pix_value = to_pal2(col, image);
          break;
        case PIXFMT_PAL1:
          pix_value = to_pal1(col, image);
          break;
      }

      // Inject the pixel value into the 32-bit word.
      word = word | (pix_value << shift);
      bits_left_in_word -= bpp;
      shift += bpp;

      // Write the word to the output buffer when the word is full.
      if (bits_left_in_word == 0) {
        dst[0] = word;
        dst[1] = word >> 8;
        dst[2] = word >> 16;
        dst[3] = word >> 24;
        dst += 4;
        word = 0u;
        bits_left_in_word = 32;
        shift = 0;
      }
    }

    // Write the last word of the row (if padding is needed).
    if (bits_left_in_word != 32) {
      dst[0] = word;
      dst[1] = word >> 8;
      dst[2] = word >> 16;
      dst[3] = word >> 24;
    }

  }

  // Update the image with the converted data.
  free(image->pixels);
  image->pixels = pixels;
  image->pixels_size = pixels_size;
  image->pixfmt = target_pixfmt;
}

static void compress_image(image_t* image, unsigned comp_mode) {
  if (comp_mode == COMP_NONE) {
    // Nothing to do!
  } else if (comp_mode == COMP_LZG) {
    // Allocate memory for the compressed data.
    lzg_uint32_t max_enc_size = LZG_MaxEncodedSize(image->pixels_size);
    unsigned char* enc_buf = (unsigned char*)malloc(max_enc_size);
    if (enc_buf == NULL) {
      fprintf(stderr, "liblzg: Out of memory!\n");
      exit(1);
    }

    // Compress the data.
    lzg_encoder_config_t config;
    LZG_InitEncoderConfig(&config);
    config.level = LZG_LEVEL_9;
    lzg_uint32_t enc_size = LZG_Encode(image->pixels,
                                       image->pixels_size,
                                       enc_buf,
                                       max_enc_size,
                                       &config);
    if (enc_size == 0u) {
      fprintf(stderr, "liblzg: Compression failed!\n");
      exit(1);
    }

    // Replace the raw pixel data with the compressed pixel data.
    free(image->pixels);
    image->pixels = enc_buf;
    image->pixels_size = (size_t)enc_size;
  } else {
    fprintf(stderr, "Unsupportd compression mode: %d.\n", comp_mode);
    exit(1);
  }

  image->comp_mode = comp_mode;
}

static void write_uint8(const uint32_t x, FILE* f) {
  uint8_t buf[1] = { x & 255u };
  (void) fwrite(&buf[0], 1, 1, f);
}

static void write_uint16(const uint32_t x, FILE* f) {
  uint8_t buf[2] = { x & 255u, (x >> 8) & 255u };
  (void) fwrite(&buf[0], 1, 2, f);
}

static void write_uint32(const uint32_t x, FILE* f) {
  uint8_t buf[4] = { x & 255u, (x >> 8) & 255u, (x >> 16) & 255u,  (x >> 24) & 255u };
  (void) fwrite(&buf[0], 1, 4, f);
}

static void write_image(const image_t* image, FILE* f) {
  const int no_palette_colors = palette_colors_for_pixfmt(image->pixfmt);

  // Write the header.
  write_uint32(0x3149434du, f);         // Magic ID
  write_uint16(image->width, f);        // Image width
  write_uint16(image->height, f);       // Image height
  write_uint8(image->pixfmt, f);        // Pixel format
  write_uint8(image->comp_mode, f);     // Compression method
  write_uint16(no_palette_colors, f);   // Number of palette colors
  write_uint32(image->pixels_size, f);  // Pixel data size (in bytes)

  // Write the palette, if any.
  for (int i = 0; i < no_palette_colors; ++i) {
    write_uint32(to_rgba8888(image->palette[i]), f);
  }

  // Write the pixel data.
  fwrite(image->pixels, 1, image->pixels_size, f);
}

static void print_usage(const char* prg_name) {
  fprintf(stderr, "Usage: %s [options] PNGFILE [MCIFILE]\n\n", prg_name);
  fprintf(stderr, "  PNGFILE     - The name of the PNG file\n");
  fprintf(stderr, "  MCIFILE     - The name of the MCI file (optional)\n");
  fprintf(stderr, "\nPixel format options:\n");
  fprintf(stderr, "  --rgba8888  - Pixel format = RGBA8888 (default)\n");
  fprintf(stderr, "  --rgba5551  - Pixel format = RGBA5551\n");
  fprintf(stderr, "  --pal8      - Pixel format = PAL8 (8 bpp palette)\n");
  fprintf(stderr, "  --pal4      - Pixel format = PAL4 (4 bpp palette)\n");
  fprintf(stderr, "  --pal2      - Pixel format = PAL2 (2 bpp palette)\n");
  fprintf(stderr, "  --pal1      - Pixel format = PAL1 (1 bpp palette)\n");
  fprintf(stderr, "\nPalette options (only for PAL formats):\n");
  fprintf(stderr, "  --optimal   - Use optimal palette (default)\n");
  fprintf(stderr, "  --grayscale - Use a grayscale palette\n");
  fprintf(stderr, "\nCompression options:\n");
  fprintf(stderr, "  --nocomp    - Use no compression (default)\n");
  fprintf(stderr, "  --lzg       - Use LZG compression\n");
  fprintf(stderr, "\nGeneral options:\n");
  fprintf(stderr, "  --help      - Show this help text\n");
  fprintf(stderr, "\nIf MCIFILE is not given, the image is written to stdout.\n");
}

int main(int argc, char** argv) {
  // Parse command line arguments.
  if (argc < 2) {
    print_usage(argv[0]);
    exit(1);
  }
  int target_pixfmt = PIXFMT_RGBA8888;
  int palette_mode = PAL_OPTIMAL;
  int comp_mode = COMP_NONE;
  const char* png_file_name = NULL;
  const char* mci_file_name = NULL;
  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if (strcmp(arg, "--help") == 0) {
      print_usage(argv[0]);
      exit(0);
    } else if (strcmp(arg, "--rgba8888") == 0) {
      target_pixfmt = PIXFMT_RGBA8888;
    } else if (strcmp(arg, "--rgba5551") == 0) {
      target_pixfmt = PIXFMT_RGBA5551;
    } else if (strcmp(arg, "--pal8") == 0) {
      target_pixfmt = PIXFMT_PAL8;
    } else if (strcmp(arg, "--pal4") == 0) {
      target_pixfmt = PIXFMT_PAL4;
    } else if (strcmp(arg, "--pal2") == 0) {
      target_pixfmt = PIXFMT_PAL2;
    } else if (strcmp(arg, "--pal1") == 0) {
      target_pixfmt = PIXFMT_PAL1;
    } else if (strcmp(arg, "--optimal") == 0) {
      palette_mode = PAL_OPTIMAL;
    } else if (strcmp(arg, "--grayscale") == 0) {
      palette_mode = PAL_GRAYSCALE;
    } else if (strcmp(arg, "--nocomp") == 0) {
      comp_mode = COMP_NONE;
    } else if (strcmp(arg, "--lzg") == 0) {
      comp_mode = COMP_LZG;
    } else if (arg[0] == '-') {
      fprintf(stderr, "Unrecognized option: %s\n", arg);
      print_usage(argv[0]);
      exit(1);
    } else if (png_file_name == NULL) {
      png_file_name = arg;
    } else if (mci_file_name == NULL) {
      mci_file_name = arg;
    } else {
      fprintf(stderr, "Unrecognized argument: %s\n", arg);
      print_usage(argv[0]);
      exit(1);
    }
  }

  // Load the PNG image.
  image_t image;
  {
    unsigned error = lodepng_decode32_file(&image.pixels,
                                           &image.width,
                                           &image.height,
                                           png_file_name);
    if (error) {
      fprintf(stderr, "Decoder error %u: %s\n", error, lodepng_error_text(error));
      exit(1);
    }
    image.pixfmt = PIXFMT_RGBA8888;
    image.pixels_size = (size_t)image.width * (size_t)image.height * sizeof(uint32_t);
  }

  // Create an optimal palette.
  create_palette(&image, target_pixfmt, palette_mode);

  // Convert to the target bit depth.
  convert_pixels(&image, target_pixfmt);

  // Compress the image.
  compress_image(&image, comp_mode);

  // Write the MCI image.
  FILE* out_file = stdout;
  if (mci_file_name != NULL) {
    out_file = fopen(mci_file_name, "wb");
    if (out_file == NULL) {
      fprintf(stderr, "Error: Unable to open %s for writing.\n", mci_file_name);
      exit(1);
    }
  }
  write_image(&image, out_file);
  if (mci_file_name != NULL) {
    fclose(out_file);
  }

  // Free the memory.
  free(image.pixels);

  return 0;
}

