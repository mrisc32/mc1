// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2022 Marcus Geelnard
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
#include <mc1/vcp.h>

#ifdef ENABLE_SELFTEST
#include <selftest.h>
#endif

#include <cstdint>

// Defined by the linker script.
extern char __rom_size;
extern char __bss_start;
extern char __bss_size;

// Note: Using an anonymous namespace saves a few bytes of code size.
namespace {

constexpr uint32_t linker_constant(const char* ptr) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr));
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
constexpr float digit_scalef() {
  float scale = 1.0F;
  for (int i = 0; i < N; ++i) {
    scale *= 10.0F;
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

// Console class.
class console_t {
public:
  void init(void* mem) {
    m_vcon_mem = mem;

    // Show the console.
    vcon_init(m_vcon_mem);
    vcon_set_colors(0, 0xff000000U);
    vcon_show(LAYER_2);

    // Print a welcome message.
    vcon_print("\n                      **** MC1 - The MRISC32 computer ****\n\n");
  }

  void deinit() {
    vcp_set_prg(LAYER_2, nullptr);
  }

  void run_diagnostics() {
    // Print some memory information etc.
    print_addr_and_size("ROM:      ", ROM_START, linker_constant(&__rom_size));
    print_addr_and_size("VRAM:     ", VRAM_START, MMIO(VRAMSIZE));
    print_addr_and_size("XRAM:     ", XRAM_START, MMIO(XRAMSIZE));
    print_addr_and_size(
        "\nbss:      ", linker_constant(&__bss_start), linker_constant(&__bss_size));

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

    m_diags_have_been_run = true;
  }

  bool diags_have_been_run() const {
    return m_diags_have_been_run;
  }

  static void print(const char* msg) {
    vcon_print(msg);
  }

private:
#ifdef ENABLE_SELFTEST
  static void selftest_callback(int pass, int /* test_no */) {
    vcon_print(pass ? "*" : "!");
  }
#endif

  void* m_vcon_mem;
  bool m_diags_have_been_run = false;
};

}  // namespace
