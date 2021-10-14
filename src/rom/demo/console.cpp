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
#include <mc1/sdcard.h>
#include <mc1/vconsole.h>

#include <cstring>
#include <mr32intrin.h>

#ifdef ENABLE_SELFTEST
// Defined by libselftest.
extern "C" int selftest_run(void (*callback)(const int));
#endif

#ifdef ENABLE_DHRYSTONE
// Defined by dhry_1.c.
extern "C" void dhrystone(int Number_Of_Runs);
#endif

// Defined by the linker script.
extern char __rom_size;
extern char __bss_start;
extern char __bss_size;

namespace {
inline uint32_t linker_constant(const char* ptr) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr));
}

#ifdef ENABLE_DHRYSTONE
struct clkticks_t {
  uint32_t hi;
  uint32_t lo;
};

clkticks_t get_ticks() {
  clkticks_t clkticks;
  clkticks.hi = MMIO(CLKCNTHI);
  while (true) {
    clkticks.lo = MMIO(CLKCNTLO);
    const uint32_t new_cnthi = MMIO(CLKCNTHI);
    if (new_cnthi == clkticks.hi) {
      break;
    }
    clkticks.hi = new_cnthi;
  }
  return clkticks;
}

float elapsed_seconds(const clkticks_t start, const clkticks_t end) {
  // Caclulate the clock tick difference.
  const uint32_t hicorr = (start.lo > end.lo) ? 1 : 0;
  const uint32_t dhi = end.hi - start.hi + hicorr;
  const uint32_t dlo = end.lo - start.lo;
  const float delta = _mr32_utof(dhi, 32) + _mr32_utof(dlo, 0);

  // Get the CPU clock frequency (ticks per second).
  const float ticks_per_s = _mr32_utof(MMIO(CPUCLK), 0);

  // Return the time in seconds.
  return delta / ticks_per_s;
}
#endif

#ifdef ENABLE_SELFTEST
void selftest_callback(const int ok) {
  vcon_print(ok ? "*" : "!");
}
#endif

void sdcard_log_func(const char* msg) {
  vcon_print(msg);
}

template <int N>
constexpr float digit_scalef() {
  float scale = 1.0F;
  for (int i = 0; i < N; ++i) {
    scale *= 10.0F;
  }
  return scale;
}

template <int N>
constexpr int digit_scalei() {
  int scale = 1;
  for (int i = 0; i < N; ++i) {
    scale *= 10;
  }
  return scale;
}

template <int N>
void vcon_print_float(const float x) {
  auto xi = static_cast<int>(x * digit_scalef<N>());
  constexpr auto iscale = digit_scalei<N>();
  vcon_print_dec(xi / iscale);
  if (N > 0) {
    auto frac = xi % iscale;
    char buf[N + 2];
    buf[0] = '.';
    buf[N + 1] = 0;
    for (int i = N; i >= 1; --i) {
      buf[i] = '0' + (frac % 10);
      frac /= 10;
    }
    vcon_print(buf);
  }
}

void print_size(uint32_t size) {
  static const char* SIZE_SUFFIX[] = {" bytes", " KB", " MB", " GB"};
  int size_div = 0;
  while (size >= 1024u && (size & 1023u) == 0u) {
    size = size >> 10;
    ++size_div;
  }
  vcon_print_dec(static_cast<int>(size));
  vcon_print(SIZE_SUFFIX[size_div]);
}

void print_addr_and_size(const char* str, const uint32_t addr, const uint32_t size) {
  vcon_print(str);
  vcon_print("0x");
  vcon_print_hex(addr);
  vcon_print(", ");
  print_size(size);
  vcon_print("\n");
}

class console_t {
public:
  void init();
  void de_init();
  void draw(const int frame_no);

private:
  static const int MAX_COMMAND_LEN = 127;

  void* m_vcon_mem;
  char m_command[MAX_COMMAND_LEN + 1];
  int m_command_pos;
  sdctx_t m_sdctx;
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
  print_addr_and_size("ROM:      ", ROM_START, linker_constant(&__rom_size));
  print_addr_and_size("VRAM:     ", VRAM_START, MMIO(VRAMSIZE));
  print_addr_and_size("XRAM:     ", XRAM_START, MMIO(XRAMSIZE));
  print_addr_and_size("\nbss:      ", linker_constant(&__bss_start), linker_constant(&__bss_size));

  // Print CPU info.
  vcon_print("\n\nCPU Freq: ");
  vcon_print_float<2>(static_cast<float>(MMIO(CPUCLK)) * (1.0F / 1000000.0F));
  vcon_print(" MHz\n\n");

#ifdef ENABLE_SELFTEST
  // Run the selftest.
  vcon_print("Selftest: ");
  if (selftest_run(selftest_callback)) {
    vcon_print(" PASS\n\n");
  } else {
    vcon_print(" FAIL\n\n");
  }
#endif

#ifdef ENABLE_DHRYSTONE
  {
    // Run the Dhrystone benchmark.
    vcon_print("Dhrystone: ");
    const int number_of_runs = 100000;

    // Warm up caches etc (to get reproducible results).
    dhrystone(10);

    // Do the runs, and measure the time.
    const auto start_time = get_ticks();
    dhrystone(number_of_runs);
    const auto end_time = get_ticks();
    const auto user_time = elapsed_seconds(start_time, end_time);

    // Calculate metrics:
    //  1) Dhrystones per second
    //  2) DMIPS (relative to VAX 11/780)
    //  3) DMIPS/MHz (relative to CPU frequency)
    const auto dhrystones_per_second = static_cast<float>(number_of_runs) / user_time;
    const auto dmips = dhrystones_per_second * (1.0F / 1757.0F);
    const auto dmips_per_mhz = (dmips * 1000000.0F) / static_cast<float>(MMIO(CPUCLK));

    // Print results.
    vcon_print_float<1>(dhrystones_per_second);
    vcon_print(" Dhrystones/s, ");
    vcon_print_float<2>(dmips);
    vcon_print(" DMIPS, ");
    vcon_print_float<3>(dmips_per_mhz);
    vcon_print(" DMIPS/MHz\n\n");
  }
#endif

  // SD card test.
  vcon_print("Initializing SD-card... ");
  if (sdcard_init(&m_sdctx, sdcard_log_func)) {
    vcon_print("Ok!\n");

    // Read and print the first blocks of the SD card.
    char buf[1026];
    if (sdcard_read(&m_sdctx, buf, 0, 2)) {
      vcon_print("Read 1024 bytes: ");
      buf[1024] = '\n';
      buf[1025] = 0;
      vcon_print(buf);
    } else {
      vcon_print("Read error.\n");
    }
  } else {
    vcon_print("No card found.\n");
  }

  // Give instructions.
  vcon_print("\nUse switches to select demo...\n\n\n");

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
