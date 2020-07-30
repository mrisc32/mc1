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

#include <mc1/keyboard.h>
#include <mc1/leds.h>
#include <mc1/mmio.h>

void mandelbrot_init(void);
void mandelbrot_deinit(void);
void mandelbrot(int frame_no);

void raytrace_init(void);
void raytrace_deinit(void);
void raytrace(int frame_no);

void retro_init(void);
void retro_deinit(void);
void retro(int frame_no);

void console_init(void);
void console_deinit(void);
void console(int frame_no);

int g_demo_select;

static void wait_vblank() {
  // Wait for the next vertical blanking interval. We busy lopp since we don't have interrupts yet.
  uint32_t vid_frame_no = MMIO(VIDFRAMENO);
  while (vid_frame_no == MMIO(VIDFRAMENO)) {
    kb_poll();
  }
}

static int should_pause() {
  // Check if we should pause.
  uint32_t buttons = MMIO(BUTTONS);
  return (buttons & 1) != 0u;
}

int main(void) {
  kb_init();

  int demo_select_old = -1;

  int frame_no = 0;
  while (1) {
    kb_poll();

    if (should_pause()) {
      continue;
    }

    // Select which demo to run.
    int demo_select = g_demo_select;
    const uint32_t switches = MMIO(SWITCHES);
    if (switches == 1) {
      demo_select = DEMO_MANDELBROT;
    } else if (switches == 2) {
      demo_select = DEMO_RAYTRACE;
    } else if (switches == 4) {
      demo_select = DEMO_RETRO;
    }

    // If we're moving to a new demo, deinit all.
    if (demo_select != demo_select_old) {
      console_deinit();
      mandelbrot_deinit();
      raytrace_deinit();
      retro_deinit();
      wait_vblank();
      demo_select_old = demo_select;
      frame_no = 0;
    }

    // Run a single frame of the selected demo.
    switch (demo_select) {
      case DEMO_MANDELBROT:
        mandelbrot_init();
        mandelbrot(frame_no);
        break;

      case DEMO_RAYTRACE:
        raytrace_init();
        raytrace(frame_no);
        break;

      case DEMO_RETRO:
        retro_init();
        retro(frame_no);
        wait_vblank();
        break;

      default:
        console_init();
        console(frame_no);
        wait_vblank();
    }

    ++frame_no;
  }

  return 0;
}
