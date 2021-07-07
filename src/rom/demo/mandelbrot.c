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

#include "demo_select.h"

#include <mc1/fast_math.h>
#include <mc1/framebuffer.h>
#include <mc1/keyboard.h>
#include <mc1/leds.h>

#include <mr32intrin.h>

#include <stdint.h>


//--------------------------------------------------------------------------------------------------
// Configuration.
//--------------------------------------------------------------------------------------------------

typedef struct {
  int width;
  int height;
} vmode_t;

// Different video modes to try (we try lower and lower until the framebuffer fits in memory).
static const vmode_t VMODES[] = {
  {1024, 576},
  {640, 360},
  {400, 225},
  {200, 112}
};
#define NUM_VMODES (int)(sizeof(VMODES) / sizeof(VMODES[0]))

// Mandelbrot viewing area configuration.
static const float RE_CENTER = -1.25544977f;
static const float IM_CENTER = -0.38188094f;
static const float MAX_SIZE = 6.0f;
static const int MAX_ITERATIONS = 127;


//--------------------------------------------------------------------------------------------------
// Palette.
//--------------------------------------------------------------------------------------------------

// This is a fiery palette.
static const uint8_t PAL_128_RGB[127*3] = {
  0xed,0xc0,0x87, 0xee,0xc4,0x8d, 0xef,0xc8,0x95, 0xf1,0xcc,0x9d, 0xf1,0xd0,0xa5, 0xf3,0xd5,0xad,
  0xf4,0xd8,0xb5, 0xf5,0xdc,0xbc, 0xf6,0xe1,0xc4, 0xf7,0xe5,0xcc, 0xf8,0xe9,0xd4, 0xfa,0xed,0xdb,
  0xfb,0xf1,0xe3, 0xfc,0xf4,0xeb, 0xfd,0xf9,0xf2, 0xf3,0xf3,0xf3, 0xef,0xef,0xef, 0xe8,0xe9,0xe9,
  0xe2,0xe2,0xe1, 0xdb,0xdb,0xdb, 0xd4,0xd5,0xd4, 0xcd,0xcd,0xce, 0xc7,0xc7,0xc7, 0xc0,0xc0,0xc0,
  0xb9,0xb9,0xba, 0xb3,0xb3,0xb3, 0xad,0xac,0xac, 0xa5,0xa6,0xa5, 0x9f,0x9f,0x9f, 0x98,0x98,0x99,
  0x92,0x91,0x91, 0x8b,0x8b,0x8b, 0x85,0x84,0x85, 0x7e,0x7e,0x7e, 0x77,0x77,0x77, 0x70,0x70,0x70,
  0x67,0x67,0x67, 0x60,0x60,0x60, 0x5a,0x5a,0x5a, 0x54,0x54,0x54, 0x4d,0x4d,0x4e, 0x47,0x47,0x47,
  0x41,0x41,0x40, 0x3a,0x3a,0x3b, 0x34,0x34,0x34, 0x2e,0x2d,0x2d, 0x27,0x27,0x28, 0x21,0x21,0x21,
  0x13,0x13,0x12, 0x09,0x09,0x09, 0x00,0x00,0x00, 0x09,0x04,0x02, 0x12,0x07,0x04, 0x1a,0x0a,0x05,
  0x23,0x0d,0x07, 0x2b,0x10,0x08, 0x34,0x13,0x0a, 0x3d,0x16,0x0d, 0x45,0x1a,0x0e, 0x4d,0x1d,0x10,
  0x56,0x20,0x11, 0x5f,0x23,0x13, 0x66,0x26,0x15, 0x6f,0x29,0x17, 0x7a,0x2d,0x18, 0x87,0x31,0x1b,
  0x93,0x36,0x1d, 0x9f,0x3b,0x20, 0xaa,0x3f,0x22, 0xb7,0x43,0x25, 0xc3,0x48,0x27, 0xce,0x4c,0x29,
  0xdb,0x51,0x2b, 0xe4,0x59,0x2c, 0xe7,0x6a,0x27, 0xe9,0x7b,0x23, 0xec,0x8d,0x1e, 0xef,0x9e,0x1a,
  0xf3,0xb1,0x15, 0xf6,0xc6,0x0f, 0xfa,0xdc,0x09, 0xfc,0xf1,0x04, 0xfe,0xfd,0x00, 0xfe,0xf7,0x00,
  0xfc,0xf1,0x00, 0xfb,0xec,0x00, 0xfa,0xe5,0x00, 0xf8,0xe0,0x00, 0xf7,0xda,0x00, 0xf6,0xd4,0x00,
  0xf5,0xce,0x00, 0xf3,0xc8,0x00, 0xf2,0xc4,0x00, 0xf1,0xc1,0x00, 0xf1,0xbc,0x00, 0xf0,0xb8,0x00,
  0xef,0xb4,0x00, 0xee,0xb0,0x00, 0xed,0xad,0x00, 0xe9,0xa7,0x00, 0xe5,0xa2,0x00, 0xe1,0x9c,0x00,
  0xdc,0x96,0x00, 0xd9,0x90,0x01, 0xd5,0x8a,0x01, 0xd1,0x84,0x00, 0xce,0x7e,0x00, 0xca,0x79,0x00,
  0xc5,0x72,0x01, 0xc1,0x6d,0x00, 0xbe,0x67,0x01, 0xba,0x61,0x01, 0xb6,0x5b,0x01, 0xb2,0x55,0x01,
  0xae,0x4f,0x00, 0xa9,0x4a,0x00, 0xa2,0x45,0x00, 0x9c,0x3f,0x01, 0x95,0x3a,0x01, 0x8e,0x34,0x00,
  0x87,0x2d,0x00, 0x81,0x27,0x01, 0x7a,0x21,0x01, 0x73,0x1c,0x01, 0x6c,0x16,0x01, 0x66,0x10,0x01,
  0x5f,0x0a,0x01
};

static void set_palette(fb_t* fb) {
  const uint8_t* pal = &PAL_128_RGB[0];
  for (int k = 0; k < MAX_ITERATIONS; ++k) {
    const uint32_t r = (uint32_t)pal[0];
    const uint32_t g = (uint32_t)pal[1];
    const uint32_t b = (uint32_t)pal[2];
    fb->palette[k] = _mr32_pack_h(_mr32_pack(0xff, g), _mr32_pack(b, r));
    pal += 3;
  }
  fb->palette[MAX_ITERATIONS] = 0x00000000;
}


//--------------------------------------------------------------------------------------------------
// Mandelbrot implementation
//--------------------------------------------------------------------------------------------------

static float get_zoom(int frame_no) {
  frame_no &= 127;
  if (frame_no >= 64) {
    frame_no = 128 - frame_no;
  }
  return fast_pow(0.90f, (float)frame_no);
}

static void mandel_row(const float re_c,
                       const float im_c,
                       const float dre_dx,
                       const float dim_dx,
                       const int width,
                       uint32_t* row_pixels) {
  const float limit_sqr = 4.0f;
  const float dre_dx_x4 = dre_dx * 4.0f;
  const float dim_dx_x4 = dim_dx * 4.0f;

  int pixels_left;
  uint32_t* pix;
  uint32_t count;
  uint32_t tmp1;
  uint32_t tmp2;
  uint32_t tmp_vec[1];
  __asm__ volatile(
      "ldi     vl, #4\n\t"

      "mov     %[pix], %[row_pixels]\n\t"
      "mov     %[pixels_left], %[width]\n\t"

      // [v1, v2] = C
      "ldea    v2, [z, #1]\n\t"
      "itof    v2, v2, z\n\t"
      "fmul    v1, v2, %[dre_dx]\n\t"
      "fadd    v1, v1, %[re_c]\n\t"    // v1 = re_c + dre_dx * [0.0f, 1.0f, 2.0f, 3.0f]
      "fmul    v2, v2, %[dim_dx]\n\t"
      "fadd    v2, v2, %[im_c]\n\t"    // v2 = im_c + dim_dx * [0.0f, 1.0f, 2.0f, 3.0f]

      "\n1:\n\t"
      // v10 = result
      "mov     v10, vz\n\t"

      // [v3, v4] = z   (z_1 = C)
      "mov     v3, v1\n\t"
      "mov     v4, v2\n\t"

      "ldi     %[count], #1\n\t"

      // Check if all vector points are inside M1 or M2, and skip the iterations if so. See:
      // - http://iquilezles.org/www/articles/mset_1bulb/mset1bulb.htm
      // - http://iquilezles.org/www/articles/mset_2bulb/mset2bulb.htm
      "fmul    v5, v1, v1\n\t"
      "fmul    v6, v2, v2\n\t"
      "fadd    v5, v5, v6\n\t"  // v5 = |C|^2

      // M1: (16 * |C|^2)^2 - 96 * |C|^2 + 32 * Re(C) - 3 < 0
      "ldi     %[tmp2], #0x41800000\n\t"  // 16.0f
      "fmul    v6, v5, %[tmp2]\n\t"
      "fmul    v6, v6, v6\n\t"
      "ldi     %[tmp1], #0x42c00000\n\t"  // 96.0f
      "fmul    v7, v5, %[tmp1]\n\t"
      "fsub    v6, v6, v7\n\t"
      "ldi     %[tmp1], #0x42000000\n\t"  // 32.0f
      "fmul    v7, v1, %[tmp1]\n\t"
      "fadd    v6, v6, v7\n\t"
      "ldi     %[tmp1], #0xc0400000\n\t"  // -3.0f
      "fadd    v6, v6, %[tmp1]\n\t"
      "fslt    v6, v6, z\n\t"             // v6 = Inside M1?

      // M2: 16 * (|C|^2 + 2 * Re(C)) + 15 < 0
      "ldi     %[tmp1], #0x40000000\n\t"  // 2.0f
      "fmul    v7, v1, %[tmp1]\n\t"
      "fadd    v7, v5, v7\n\t"
      "fmul    v7, v7, %[tmp2]\n\t"
      "ldi     %[tmp1], #0x41700000\n\t"  // 15.0f
      "fadd    v7, v7, %[tmp1]\n\t"
      "fslt    v7, v7, z\n\t"             // v7 = Inside M2?

      // Inside either M1 or M2?
      "or      v6, v6, v7\n\t"

      // ...for all four pixels?
      "and/f   v6, v6, v6\n\t"
      "ldi     vl, #2\n\t"
      "and/f   v6, v6, v6\n\t"
      "ldi     vl, #1\n\t"
      "stw     v6, [%[tmp_vec]]\n\t"
      "ldw     %[tmp1], [%[tmp_vec]]\n\t"
      "ldi     vl, #4\n\t"
      "bns     %[tmp1], 2f\n\t"

      // Early-out for these four pixels.
      "ldi     %[tmp1], #0x7f7f7f7f\n\t"
      "stw     %[tmp1], [%[pix]]\n\t"
      "b       4f\n\t"

      "\n2:\n\t"

      // [v5, v6] = [z.re^2, z.im^2]
      "fmul    v5, v3, v3\n\t"
      "fmul    v6, v4, v4\n\t"

      // v7 = |z|^2
      "fadd    v7, v5, v6\n\t"

      // |z|^2 < limit_sqr?
      "fslt    v8, v7, %[limit_sqr]\n\t"

      // Update per-pixel results.
      "and     v9, v8, %[count]\n\t"
      "max     v10, v9, v10\n\t"

      // Done?
      // TODO(m): We need better vector -> scalar instructions (see mrisc32-#38).
      "or/f    v9, v8, v8\n\t"
      "ldi     vl, #2\n\t"
      "or/f    v9, v9, v9\n\t"
      "ldi     vl, #1\n\t"
      "stw     v9, [%[tmp_vec]]\n\t"
      "ldw     %[tmp1], [%[tmp_vec]]\n\t"
      "ldi     vl, #4\n\t"
      "bz      %[tmp1], 3f\n\t"

      // z.im = sqr(z.re + z.im) - (z.re^2 + z.im^2) + c.im;
      "fadd    v4, v3, v4\n\t"
      "fmul    v4, v4, v4\n\t"
      "fsub    v4, v4, v7\n\t"
      "fadd    v4, v4, v2\n\t"

      // z.re = z.re^2 - z.im^2 + c.re;
      "fsub    v3, v5, v6\n\t"
      "fadd    v3, v3, v1\n\t"

      "add     %[count], %[count], #1\n\t"
      "sle     %[tmp1], %[count], #%[MAX_ITERATIONS]\n\t"
      "bs      %[tmp1], 2b\n\t"

      "\n3:\n\t"
      "pack/f  v10, v10, v10\n\t"
      "ldi     vl, #2\n\t"
      "pack.h/f v10, v10, v10\n\t"
      "ldi     vl, #1\n\t"
      "stw     v10, [%[pix]]\n\t"
      "ldi     vl, #4\n\t"

      // Next four pixels...
      "\n4:\n\t"
      "fadd    v1, v1, %[dre_dx_x4]\n\t"
      "fadd    v2, v2, %[dim_dx_x4]\n\t"
      "add     %[pix], %[pix], #4\n\t"
      "add     %[pixels_left], %[pixels_left], #-4\n\t"
      "bgt     %[pixels_left], 1b\n\t"

      : [count] "=&r"(count),
        [pixels_left] "=&r"(pixels_left),
        [pix] "=&r"(pix),
        [tmp1] "=&r"(tmp1),
        [tmp2] "=&r"(tmp2)
      : [re_c] "r"(re_c),
        [im_c] "r"(im_c),
        [dre_dx] "r"(dre_dx),
        [dim_dx] "r"(dim_dx),
        [dre_dx_x4] "r"(dre_dx_x4),
        [dim_dx_x4] "r"(dim_dx_x4),
        [limit_sqr] "r"(limit_sqr),
        [MAX_ITERATIONS] "n"(MAX_ITERATIONS),
        [width] "r"(width),
        [row_pixels] "r"(row_pixels),
        [tmp_vec] "r"(tmp_vec)
      : "vl", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "memory");
}

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

static fb_t* s_fb;

void mandelbrot_init(void) {
  if (s_fb == NULL) {
    for (int i = 0; i < NUM_VMODES; ++i) {
      const vmode_t* vm = &VMODES[i];
      s_fb = fb_create(vm->width, vm->height, CMODE_PAL8);
      if (s_fb != NULL) {
        fb_show(s_fb, LAYER_1);
        set_palette(s_fb);
        break;
      }
    }
  }
}

void mandelbrot_deinit(void) {
  if (s_fb != NULL) {
    vcp_set_prg(LAYER_1, NULL);
    fb_destroy(s_fb);
    s_fb = NULL;
  }
}

void mandelbrot(int frame_no) {
  if (s_fb == NULL) {
    return;
  }

  sevseg_print_dec(frame_no);

  const int width = s_fb->width;
  const int height = s_fb->height;
  const size_t stride = s_fb->stride;

  // Get the scaling and rotation for this frame.
  const float step = get_zoom(frame_no) * MAX_SIZE / (float)width;
  const float dre_dx = step * fast_cos((-0.03125f) * (float)frame_no);
  const float dim_dx = step * fast_sin((-0.03125f) * (float)frame_no);
  const float dre_dy = -dim_dx;
  const float dim_dy = dre_dx;

  for (int k = 0; k < height; ++k) {
    // We start at the middle and alternatingly expand up and down.
    int dy = (k + 1) >> 1;
    if ((k & 1) == 1) {
      dy = -dy;
    }
    const int y = (height >> 1) + dy;

    // Calculate the C value for the first pixel of this row.
    const float x0 = (float)(-width / 2);
    const float y0 = (float)dy;
    float re_c = RE_CENTER + dre_dx * x0 + dre_dy * y0;
    float im_c = IM_CENTER + dim_dx * x0 + dim_dy * y0;

    // Draw one row (left to right).
    uint32_t* pixels_row = (uint32_t*)&((uint8_t*)s_fb->pixels)[(size_t)y * stride];
    mandel_row(re_c, im_c, dre_dx, dim_dx, width, pixels_row);

    // Check for keyboard ESC press.
    {
      kb_poll();
      int stop = 0;
      uint32_t event;
      while ((event = kb_get_next_event()) != 0u) {
        if (kb_event_is_press(event) && kb_event_scancode(event) == KB_ESC) {
          stop = 1;
        }
      }
      if (stop) {
        g_demo_select = DEMO_NONE;
        break;
      }
    }
  }
}

