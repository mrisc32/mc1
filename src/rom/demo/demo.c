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

// Define this to interactively select which demo to run.
//#define INTERACTIVE_MODE

#include "demo_select.h"

#include <mc1/keyboard.h>
#include <mc1/leds.h>
#include <mc1/mmio.h>

void console_init(void);
void console_deinit(void);
void console(int frame_no);

void mandelbrot_init(void);
void mandelbrot_deinit(void);
void mandelbrot(int frame_no);

void raytrace_init(void);
void raytrace_deinit(void);
void raytrace(int frame_no);

void retro_init(void);
void retro_deinit(void);
void retro(int frame_no);

void stars_init(const char* text);
void stars_deinit(void);
void stars(int frame_no);

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

#define STAR_TEXT_NULL ((const char*)0)

#define STAR_TEXT_1           \
  "\002\x00\x01"              \
  "\n"                        \
  " MEET THE WORLD'S FIRST\n" \
  "    MRISC32 COMPUTER!\n"   \
  "\002\x80\x00"              \
  "\001"                      \
  "\003\x80\xff\x80"          \
  "  MACHINE: MC1\n"          \
  "  CPU:     MRISC32-A1\n"   \
  "  CLOCK:   120 MHZ\n"      \
  "  VRAM:    256 KB "        \
  "\002\x80\x00"              \
  "\001"                      \
  "\003\xa0\xff\xe0"          \
  " SINCE WE HAVE HARDWARE\n" \
  " FLOATING-POINT SUPPORT\n" \
  "    WE CAN RENDER A\n"     \
  "  MANDELBROT FRACTAL... "

#define STAR_TEXT_2        \
  "\003\xff\xff\x80"       \
  "\n"                     \
  "   ...OR HOW ABOUT A\n" \
  "       RAY TRACER? "

#define STAR_TEXT_3            \
  "\003\xff\x80\xff"           \
  " NOW LET'S UTILIZE THE\n"   \
  "GRAPHICS CAPABILITIES OF\n" \
  "  THE MC1 COMPUTER...\n"    \
  "\002\x40\x00"               \
  "  ...RETRO STYLE! "

#define STAR_TEXT_4          \
  "\003\xa0\xff\xc0"         \
  "\n"                       \
  " THANK'S FOR WATCHING!\n" \
  "\002\x40\x00"             \
  "\001"                     \
  "\003\xff\xff\xff"         \
  "  FOR MORE INFO VISIT:\n" \
  " GITHUB.COM/MRISC32/MC1 "

#ifndef INTERACTIVE_MODE
typedef struct {
  int select;
  int num_frames;
  const char* text;
} demo_part_t;

static demo_part_t DEMO_SEQUENCE[] = {
    {DEMO_STARS, 1900, STAR_TEXT_1},
    {DEMO_MANDELBROT, 30, STAR_TEXT_NULL},
    {DEMO_STARS, 400, STAR_TEXT_2},
    {DEMO_RAYTRACE, 20, STAR_TEXT_NULL},
    {DEMO_STARS, 730, STAR_TEXT_3},
    {DEMO_RETRO, 5000, STAR_TEXT_NULL},
    {DEMO_STARS, 600, STAR_TEXT_4},
};

#define DEMO_SEQUENCE_LAST (sizeof(DEMO_SEQUENCE) / sizeof(DEMO_SEQUENCE[0]) - 1)
#endif

int main(void) {
  kb_init();

  int demo_select_old = -1;

  int frame_no = 0;
#ifndef INTERACTIVE_MODE
  unsigned sequence_idx = 0;
#endif
  while (1) {
    kb_poll();

    if (should_pause()) {
      continue;
    }

    int demo_select;
    const char* star_text;

#ifdef INTERACTIVE_MODE
    // Select which demo to run.
    demo_select = g_demo_select;
    const uint32_t switches = MMIO(SWITCHES);
    if (switches == 1) {
      demo_select = DEMO_MANDELBROT;
    } else if (switches == 2) {
      demo_select = DEMO_RAYTRACE;
    } else if (switches == 4) {
      demo_select = DEMO_RETRO;
    } else if (switches == 8) {
      demo_select = DEMO_STARS;
    }
    star_text = STAR_TEXT_1;
#else
    {
      // Select the current demo part.
      const demo_part_t* part = &DEMO_SEQUENCE[sequence_idx];
      if (frame_no >= part->num_frames && sequence_idx < DEMO_SEQUENCE_LAST) {
        ++sequence_idx;
        part = &DEMO_SEQUENCE[sequence_idx];
      }

      demo_select = part->select;
      star_text = part->text;
    }
#endif  // INTERACTIVE_MODE

    // If we're moving to a new demo, deinit all.
    if (demo_select != demo_select_old) {
      console_deinit();
      mandelbrot_deinit();
      raytrace_deinit();
      retro_deinit();
      stars_deinit();
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

      case DEMO_STARS:
        stars_init(star_text);
        stars(frame_no);
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
