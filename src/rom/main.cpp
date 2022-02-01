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

#include "elf32.hpp"
#include "mosaic.hpp"

#ifdef ENABLE_CONSOLE
#include "console.hpp"
#endif

#include <mc1/leds.h>
#include <mc1/mfat_mc1.h>
#include <mc1/sdcard.h>

#include <cstdint>

// Defined by the linker script.
extern char __vram_free_start;

namespace {
// States for the boot state machine.
enum class boot_state_t {
  INITIALIZE,
  RUN_DIAGNOSTICS,
  WAIT_FOR_SDCARD,
  MOUNT_FAT,
  LOAD_MC1BOOT,
};

// Frame sync class.
class frame_sync_t {
public:
  frame_sync_t() : m_t(0) {
    m_last_frame_no = MMIO(VIDFRAMENO);
  }

  void wait_for_next_frame() {
    uint32_t frame_no;
    do {
      frame_no = MMIO(VIDFRAMENO);
    } while (frame_no == m_last_frame_no);
    m_t += frame_no - m_last_frame_no;
    m_last_frame_no = frame_no;
  }

  uint32_t t() const {
    return m_t;
  }

private:
  uint32_t m_t;
  uint32_t m_last_frame_no;
};

// Boot function type.
using boot_fun_t = void();

int read_block_fun(char* ptr, unsigned block_no, void* custom) {
  auto* ctx = reinterpret_cast<sdctx_t*>(custom);
  sdcard_read(ctx, ptr, block_no, 1);
  return 0;
}

int write_block_fun(const char*, unsigned, void*) {
  // Not implemented.
  return -1;
}

#ifdef ENABLE_CONSOLE
void sdcard_log_fun(const char* msg) {
  console_t::print(msg);
}
#else
#define sdcard_log_fun nullptr
#endif

}  // namespace

extern "C" int main(int, char**) {
  sevseg_print("OLLEH ");  // Print a friendly "HELLO".

#ifdef ENABLE_CONSOLE
  console_t console;
#endif
  mosaic_t mosaic;
  sdctx_t sdctx;
  frame_sync_t frame_sync;

  auto state = boot_state_t::INITIALIZE;
  while (true) {
    // Update splash screen.
    if (state != boot_state_t::INITIALIZE) {
      frame_sync.wait_for_next_frame();
      mosaic.update(frame_sync.t());
    }

    // Boot state machine.
    switch (state) {
      //--------------------------------------------------------------------------------------------
      // INITIALIZE
      //--------------------------------------------------------------------------------------------
      default:
      case boot_state_t::INITIALIZE: {
        auto* mem = reinterpret_cast<void*>(&__vram_free_start);
        mem = mosaic.init(mem);
#ifdef ENABLE_CONSOLE
        console.init(mem);
#endif
        state = boot_state_t::RUN_DIAGNOSTICS;
      } break;

      //--------------------------------------------------------------------------------------------
      // RUN_DIAGNOSTICS
      //--------------------------------------------------------------------------------------------
      case boot_state_t::RUN_DIAGNOSTICS: {
#ifdef ENABLE_CONSOLE
        if (!console.diags_have_been_run()) {
          console.run_diagnostics();
        }
        console_t::print("Insert bootable SD-card... ");
#endif
        state = boot_state_t::WAIT_FOR_SDCARD;
      } break;

      //--------------------------------------------------------------------------------------------
      // WAIT_FOR_SDCARD
      //--------------------------------------------------------------------------------------------
      case boot_state_t::WAIT_FOR_SDCARD: {
        if (sdcard_init(&sdctx, sdcard_log_fun)) {
#ifdef ENABLE_CONSOLE
          console_t::print("OK!\nMounting FAT filesystem... ");
#endif
          sevseg_print("DRACDS");
          state = boot_state_t::MOUNT_FAT;
        }
      } break;

      //--------------------------------------------------------------------------------------------
      // MOUNT_FAT
      //--------------------------------------------------------------------------------------------
      case boot_state_t::MOUNT_FAT: {
        if (mfat_mount(&read_block_fun, &write_block_fun, &sdctx) == 0) {
#ifdef ENABLE_CONSOLE
          console_t::print("OK!\n");
#endif
          sevseg_print("TAF   ");
          state = boot_state_t::LOAD_MC1BOOT;
        } else {
          // Retry the SD card step until we find a valid SD card.
          state = boot_state_t::WAIT_FOR_SDCARD;
        }
      } break;

      //--------------------------------------------------------------------------------------------
      // LOAD_MC1BOOT
      //--------------------------------------------------------------------------------------------
      case boot_state_t::LOAD_MC1BOOT: {
        // TODO(m): Here we should just stat the exe file and fail back to WAIT_FOR_SDCARD if it
        // does not exist instead of falling back all the way to INITIALIZE.

        // Deinitialize video (blank it while loading the boot executable).
#ifdef ENABLE_CONSOLE
        console.deinit();
#endif
        mosaic.deinit();

        // Try to load the boot executable.
        uint32_t entry_address = 0;
        if (elf32::load("MC1BOOT.EXE", entry_address)) {
          sevseg_print("TOOB  ");

          // Call the boot function.
          auto* boot_fun = reinterpret_cast<boot_fun_t*>(entry_address);
          boot_fun();

          // TODO(m): If the boot program returns (it shouldn't!), we probably
          // need to do a soft reset in order to get back a valid stack etc.
        } else {
          sevseg_print("LOL     ");
        }

        // Since we deinitialized before, we need to go to initialize again.
        state = boot_state_t::INITIALIZE;
      } break;
    }
  }

  return 0;
}
