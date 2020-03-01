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

#include <system/framebuffer.h>
#include <system/leds.h>
#include <system/mmio.h>
#include <system/vconsole.h>

static const int FB_WIDTH = 640;
static const int FB_HEIGHT = 360;

void mandelbrot(int frame_no, void* fb_start);
void funky(int frame_no, void* fb_start);
void raytrace_init(void);
void raytrace_deinit(void);
void raytrace(int frame_no);

static void wait_vblank() {
  // Wait for the next vertical blanking interval. We busy lopp since we don't have interrupts yet.
  uint32_t vid_frame_no = MMIO(VIDFRAMENO);
  while (vid_frame_no == MMIO(VIDFRAMENO));
}

static void set_palette(fb_t* fb) {
  uint32_t* pal = fb->palette;

  // Color 0 is black.
  pal[0] = 0xff000000;

  // Generate colorful gradients for colors 1..255.
  int32_t r = 100;
  int32_t g = 50;
  int32_t b = 0;
  int32_t r_inc = 2;
  int32_t g_inc = 3;
  int32_t b_inc = 1;

  for (int k = 1; k <= 255; ++k) {
    pal[k] = 0xff000000u | (b << 16) | (g << 8) | r;

    // Update R, and if necessary adjust the increment value.
    r += r_inc;
    if (r < 0 || r > 255) {
      r_inc = -r_inc;
      r += r_inc;
    }

    // Update G, and if necessary adjust the increment value.
    g += g_inc;
    if (g < 0 || g > 255) {
      g_inc = -g_inc;
      g += g_inc;
    }

    // Update B, and if necessary adjust the increment value.
    b += b_inc;
    if (b < 0 || b > 255) {
      b_inc = -b_inc;
      b += b_inc;
    }
  }
}

int main(void) {
  fb_t* fb = fb_create(FB_WIDTH, FB_HEIGHT, MODE_PAL8);
  if (!fb) {
    return 1;
  }

  set_palette(fb);

  int frame_no = 0;
  while (1) {
    // Select program with the board switches.
    uint32_t switches = MMIO(SWITCHES);
    if (switches == 1) {
      fb_show(fb);
      mandelbrot(frame_no, fb->pixels);
    } else if (switches == 2) {
      fb_show(fb);
      funky(frame_no, fb->pixels);
    } else if (switches == 4) {
      // TODO(m): We need to call init/deinit too etc.
      raytrace(frame_no);
    } else {
      vcon_show();
    }

    // Write the raster Y position to the segment displays.
    int raster_y = MMIO(VIDY);
    sevseg_print_dec(raster_y);

    wait_vblank();
    ++frame_no;
  }

  fb_destroy(fb);

  return 0;
}

