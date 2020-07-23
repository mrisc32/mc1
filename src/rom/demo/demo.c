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
#include <mc1/memory.h>
#include <mc1/mmio.h>
#include <mc1/vconsole.h>

void mandelbrot_init(void);
void mandelbrot_deinit(void);
void mandelbrot(int frame_no);

void raytrace_init(void);
void raytrace_deinit(void);
void raytrace(int frame_no);

void retro_init(void);
void retro_deinit(void);
void retro(int frame_no);

int selftest_run(void (*callback)(const int));

static void* s_vcon_mem;

static void selftest_callback(const int ok) {
  vcon_print(ok ? "*" : "!");
}

static void print_addr_and_size(const char* str, const uint32_t addr, const uint32_t size) {
  vcon_print(str);
  vcon_print("0x");
  vcon_print_hex(addr);
  vcon_print(", ");
  vcon_print_dec((int)size);
  vcon_print(" bytes\n");
}

// These are defined by the linker script.
extern char __rom_size;
extern char __bss_start;
extern char __bss_size;

static void print_mem_info(void) {
  print_addr_and_size("ROM:  ", ROM_START, (uint32_t)(&__rom_size));
  print_addr_and_size("VRAM: ", VRAM_START, MMIO(VRAMSIZE));
  print_addr_and_size(" BSS: ", (uint32_t)(&__bss_start), (uint32_t)(&__bss_size));
  print_addr_and_size("XRAM: ", XRAM_START, MMIO(XRAMSIZE));
}

static void console_init(void) {
  if (s_vcon_mem != NULL) {
    return;
  }

  // Allocate memory for the video console framebuffer.
  const unsigned size = vcon_memory_requirement();
  s_vcon_mem = mem_alloc(size, MEM_TYPE_VIDEO | MEM_CLEAR);
  if (s_vcon_mem == NULL) {
    return;
  }

  // Show the console.
  vcon_init(s_vcon_mem);
  vcon_show(LAYER_1);
  vcon_print("\n                      **** MC1 - The MRISC32 computer ****\n\n");

  // Print some memory information etc.
  print_mem_info();

  // Run the selftest.
  vcon_print("\nSelftest: ");
  if (selftest_run(selftest_callback)) {
    vcon_print(" PASS\n\n");
  } else {
    vcon_print(" FAIL\n\n");
  }

  // Give instructions.
  vcon_print("Use switches to select demo...\n");
}

static void console_deinit(void) {
  if (s_vcon_mem == NULL) {
    return;
  }
  vcp_set_prg(LAYER_1, NULL);
  mem_free(s_vcon_mem);
  s_vcon_mem = NULL;
}

static void wait_vblank() {
  // Wait for the next vertical blanking interval. We busy lopp since we don't have interrupts yet.
  uint32_t vid_frame_no = MMIO(VIDFRAMENO);
  while (vid_frame_no == MMIO(VIDFRAMENO))
    ;
}

static int should_pause() {
  // Check if we should pause.
  uint32_t buttons = MMIO(BUTTONS);
  return (buttons & 1) != 0u;
}

int main(void) {
  uint32_t switches_old = 0xffffffffu;

  int frame_no = 0;
  while (1) {
    if (should_pause()) {
      continue;
    }

    uint32_t switches = MMIO(SWITCHES);
    if (switches != switches_old) {
      console_deinit();
      mandelbrot_deinit();
      raytrace_deinit();
      retro_deinit();
      switches_old = switches;
      frame_no = 0;
    }

    // Select program with the board switches.
    if (switches == 1) {
      mandelbrot_init();
      mandelbrot(frame_no);
    } else if (switches == 2) {
      raytrace_init();
      raytrace(frame_no);
    } else if (switches == 4) {
      retro_init();
      retro(frame_no);
      wait_vblank();
    } else {
      console_init();
      sevseg_print("OLLEH");  // Print a friendly "HELLO".
      wait_vblank();
    }
    ++frame_no;
  }

  return 0;
}
