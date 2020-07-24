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

#include <mc1/leds.h>
#include <mc1/memory.h>
#include <mc1/mmio.h>
#include <mc1/vconsole.h>

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

  void* m_vcon_mem;
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

  // Print some memory information etc.
  print_addr_and_size("ROM:  ", ROM_START, linker_constant(&__rom_size));
  print_addr_and_size("VRAM: ", VRAM_START, MMIO(VRAMSIZE));
  print_addr_and_size("XRAM: ", XRAM_START, MMIO(XRAMSIZE));
  print_addr_and_size("\nbss:  ", linker_constant(&__bss_start), linker_constant(&__bss_size));

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
