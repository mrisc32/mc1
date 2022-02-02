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

#ifdef ENABLE_SPLASH
#include "splash.hpp"
#endif
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
// Name of the boot executable file.
const char* BOOT_EXE = "MC1BOOT.EXE";

// States for the boot state machine.
enum class boot_state_t {
  INITIALIZE,
  RUN_DIAGNOSTICS,
  WAIT_FOR_SDCARD,
  MOUNT_FAT,
  LOAD_MC1BOOT,
};

// Status of the boot process.
enum class boot_status_t {
  NONE,
  NO_SDCARD,
  NO_FAT,
  NO_BOOTEXE,
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

  mosaic_t mosaic;
#ifdef ENABLE_SPLASH
  splash_t splash;
#endif
#ifdef ENABLE_CONSOLE
  console_t console;
#endif
  sdctx_t sdctx;
  frame_sync_t frame_sync;

  auto status = boot_status_t::NONE;
  auto previous_status = boot_status_t::NONE;
  auto state = boot_state_t::INITIALIZE;
  while (true) {
    // Update splash screen.
    if (state != boot_state_t::INITIALIZE) {
      frame_sync.wait_for_next_frame();
      mosaic.update(frame_sync.t());

      if (status != previous_status) {
#ifdef ENABLE_CONSOLE
        const char* msg;
        switch (status) {
          case boot_status_t::NO_SDCARD:
            msg = "Insert bootable SD card\n";
            break;
          case boot_status_t::NO_FAT:
            msg = "Not a FAT formatted SD card\n";
            break;
          case boot_status_t::NO_BOOTEXE:
            msg = "No boot executable found\n";
            break;
          default:
            msg = nullptr;
            break;
        }
        if (msg != nullptr) {
          console_t::print(msg);
        }
#endif
        previous_status = status;
      }
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
#ifdef ENABLE_SPLASH
        mem = splash.init(mem);
#endif
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
#endif
        state = boot_state_t::WAIT_FOR_SDCARD;
      } break;

      //--------------------------------------------------------------------------------------------
      // WAIT_FOR_SDCARD
      //--------------------------------------------------------------------------------------------
      case boot_state_t::WAIT_FOR_SDCARD: {
        if (sdcard_init(&sdctx, sdcard_log_fun)) {
          state = boot_state_t::MOUNT_FAT;
        } else {
          status = boot_status_t::NO_SDCARD;
        }
      } break;

      //--------------------------------------------------------------------------------------------
      // MOUNT_FAT
      //--------------------------------------------------------------------------------------------
      case boot_state_t::MOUNT_FAT: {
        if (mfat_mount(&read_block_fun, &write_block_fun, &sdctx) == 0) {
          state = boot_state_t::LOAD_MC1BOOT;
        } else {
          // Retry the SD card step until we find a valid FAT formatted SD card.
          status = boot_status_t::NO_FAT;
          state = boot_state_t::WAIT_FOR_SDCARD;
        }
      } break;

      //--------------------------------------------------------------------------------------------
      // LOAD_MC1BOOT
      //--------------------------------------------------------------------------------------------
      case boot_state_t::LOAD_MC1BOOT: {
        // Stat the boot exe file to see if it exists.
        mfat_stat_t stat;
        if (mfat_stat(BOOT_EXE, &stat) == 0) {
          // Deinitialize video (blank it while loading the boot executable).
#ifdef ENABLE_CONSOLE
          console.deinit();
#endif
#ifdef ENABLE_SPLASH
          splash.deinit();
#endif
          mosaic.deinit();

          // Try to load the boot executable.
          uint32_t entry_address = 0;
          if (elf32::load(BOOT_EXE, entry_address)) {
            // Call the boot function.
            auto* boot_fun = reinterpret_cast<boot_fun_t*>(entry_address);
            boot_fun();
          }

          // If we got this far we either could not load the EXE file, or the EXE file has finished
          // executing and returned. In either case we can not trust the contents of RAM (e.g. the
          // stack), so we need to soft reset.
          __asm__ volatile("\tj\tz, #0x00000200");
        }

        // Retry the SD card step until we find a bootable SD card.
        status = boot_status_t::NO_BOOTEXE;
        state = boot_state_t::WAIT_FOR_SDCARD;
      } break;
    }
  }

  return 0;
}
