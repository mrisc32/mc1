// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
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
#include <mc1/memory.h>
#include <mc1/mmio.h>
#include <mc1/vconsole.h>

#include <cstring>

// Defined by libselftest.
extern "C" int selftest_run(void (*callback)(const int));

// Defined by the linker script.
extern char __rom_size;
extern char __bss_start;
extern char __bss_size;

namespace {
inline uint32_t linker_constant(const char* ptr) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr));
}

class console_t {
public:
  void init();
  void de_init();
  void draw(const int frame_no);

private:
  static void selftest_callback(const int ok) {
    vcon_print(ok ? "*" : "!");
  }

  static void print_addr_and_size(const char* str, const uint32_t addr, const uint32_t size) {
    vcon_print(str);
    vcon_print("0x");
    vcon_print_hex(addr);
    vcon_print(", ");
    vcon_print_dec(static_cast<int>(size));
    vcon_print(" bytes\n");
  }

  static const int MAX_COMMAND_LEN = 127;

  void* m_vcon_mem;
  char m_command[MAX_COMMAND_LEN + 1];
  int m_command_pos;
};

void console_t::init() {
  if (m_vcon_mem != NULL) {
    return;
  }

  // Allocate memory for the video console framebuffer.
  const unsigned size = vcon_memory_requirement();
  m_vcon_mem = mem_alloc(size, MEM_TYPE_VIDEO | MEM_CLEAR);
  if (m_vcon_mem == NULL) {
    return;
  }

  // Show the console.
  vcon_init(m_vcon_mem);
  vcon_show(LAYER_1);
  vcon_print("\n                      **** MC1 - The MRISC32 computer ****\n\n");

  // Print CPU info.
  const auto cpu_mhz_times_10 = (static_cast<int>(MMIO(CPUCLK)) + 50000) / 100000;
  vcon_print("CPU Freq: ");
  vcon_print_dec(cpu_mhz_times_10 / 10);
  vcon_print(".");
  vcon_print_dec(cpu_mhz_times_10 % 10);
  vcon_print(" MHz\n\n");

  // Print some memory information etc.
  print_addr_and_size("ROM:      ", ROM_START, linker_constant(&__rom_size));
  print_addr_and_size("VRAM:     ", VRAM_START, MMIO(VRAMSIZE));
  print_addr_and_size("XRAM:     ", XRAM_START, MMIO(XRAMSIZE));
  print_addr_and_size("\nbss:      ", linker_constant(&__bss_start), linker_constant(&__bss_size));

  // Run the selftest.
  vcon_print("\nSelftest: ");
  if (selftest_run(selftest_callback)) {
    vcon_print(" PASS\n\n");
  } else {
    vcon_print(" FAIL\n\n");
  }

  // Give instructions.
  vcon_print("Use switches to select demo...\n\n\n");

  m_command_pos = 0;
}

void console_t::de_init() {
  if (m_vcon_mem == NULL) {
    return;
  }
  vcp_set_prg(LAYER_1, NULL);
  mem_free(m_vcon_mem);
  m_vcon_mem = NULL;
}

void console_t::draw(const int frame_no) {
  // TODO(m): Can we do anything interesting here?
  (void)frame_no;
  sevseg_print("OLLEH");  // Print a friendly "HELLO".

  // Print character events from the keyboard.
  while (auto event = kb_get_next_event()) {
    if (kb_event_is_press(event)) {
      const auto character = kb_event_to_char(event);
      if (character != 0) {
        const char str[2] = {static_cast<char>(character), 0};
        vcon_print(str);

        if (kb_event_scancode(event) != KB_ENTER) {
          if (m_command_pos < MAX_COMMAND_LEN) {
            m_command[m_command_pos++] = static_cast<char>(character);
          }
        } else {
          m_command[m_command_pos] = 0;
          m_command_pos = 0;

          if (std::strcmp(&m_command[0], "go mandelbrot") == 0) {
            g_demo_select = DEMO_MANDELBROT;
          } else if (std::strcmp(&m_command[0], "go raytrace") == 0) {
            g_demo_select = DEMO_RAYTRACE;
          } else if (std::strcmp(&m_command[0], "go retro") == 0) {
            g_demo_select = DEMO_RETRO;
          }
        }
      }
    }
  }
}

console_t s_console;

}  // namespace

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

extern "C" void console_init() {
  s_console.init();
}

extern "C" void console_deinit() {
  s_console.de_init();
}

extern "C" void console(const int frame_no) {
  s_console.draw(frame_no);
}
