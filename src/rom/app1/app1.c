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

#include <mc1/leds.h>
#include <mc1/mmio.h>
#include <mc1/vconsole.h>

void mandelbrot_init(void);
void mandelbrot_deinit(void);
void mandelbrot(int frame_no);

void funky_init(void);
void funky_deinit(void);
void funky(int frame_no);

void raytrace_init(void);
void raytrace_deinit(void);
void raytrace(int frame_no);

static void wait_vblank() {
  // Wait for the next vertical blanking interval. We busy lopp since we don't have interrupts yet.
  uint32_t vid_frame_no = MMIO(VIDFRAMENO);
  while (vid_frame_no == MMIO(VIDFRAMENO));
}

int main(void) {
  uint32_t switches_old = 0xffffffffu;

  for (int frame_no = 0; ; ++frame_no) {
    // Write the frame number to the segment displays.
    sevseg_print_dec(frame_no);

    uint32_t switches = MMIO(SWITCHES);
    if (switches != switches_old) {
      mandelbrot_deinit();
      funky_deinit();
      raytrace_deinit();
      switches_old = switches;
    }

    // Select program with the board switches.
    if (switches == 1) {
      mandelbrot_init();
      mandelbrot(frame_no);
    } else if (switches == 2) {
      funky_init();
      funky(frame_no);
      wait_vblank();
    } else if (switches == 4) {
      raytrace_init();
      raytrace(frame_no);
    } else {
      vcon_show();
    }
  }

  return 0;
}

