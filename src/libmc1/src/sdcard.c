// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
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

#include <mc1/sdcard.h>

#include <mc1/crc16.h>
#include <mc1/crc7.h>
#include <mc1/mmio.h>

//--------------------------------------------------------------------------------------------------
// Debug logging.
//--------------------------------------------------------------------------------------------------

#define SDCARD_ENABLE_LOGGING
//#define SDCARD_ENABLE_DEBUGGING

#ifdef SDCARD_ENABLE_LOGGING
static inline void _sdcard_log(const sdctx_t* ctx, const char* msg) {
  const sdcard_log_func_t log_func = ctx->log_func;
  if (log_func != (sdcard_log_func_t)0) {
    log_func(msg);
  }
}

#define SDCARD_LOG(msg) _sdcard_log(ctx, msg)
#ifdef SDCARD_ENABLE_DEBUGGING
#define SDCARD_DEBUG(msg) _sdcard_log(ctx, msg)
#else
#define SDCARD_DEBUG(msg)
#endif
#else
#define SDCARD_LOG(msg)
#define SDCARD_DEBUG(msg)
#endif

//--------------------------------------------------------------------------------------------------
// Timing helpers.
//--------------------------------------------------------------------------------------------------

// This converts a microsecond value to a loop count for an MRISC32-A1 CPU running at about 100 MHz.
// I.e. it is very appoximate.
#define PERIOD_MICROS(us) (((us) + 10) / 20)

static inline void _sdcard_sleep(int period) {
#ifdef __MRISC32__
  int count = period;
  __asm__ volatile(
      "1:\n"
      "   add   %[count], %[count], #-1\n"
      "   bgt   %[count], 1b\n"
      :
      : [ count ] "r"(count));
#endif
}

//--------------------------------------------------------------------------------------------------
// Low level I/O bit manipulation helpers.
//--------------------------------------------------------------------------------------------------

#define SD_BLOCK_SIZE 512

#define DIR_IN 0
#define DIR_OUT 1

static void _sdcard_dir(const int bit, const uint32_t mask, const uint32_t dir) {
  MMIO(SDWE) = (MMIO(SDWE) & ~(mask << bit)) | ((mask * (uint32_t)dir) << bit);
}

static uint32_t _sdcard_get(const int bit, const uint32_t mask) {
  return (MMIO(SDIN) >> bit) & mask;
}

static void _sdcard_set(const int bit, const uint32_t mask, const uint32_t value) {
  MMIO(SDOUT) = (MMIO(SDOUT) & ~(mask << bit)) | (value << bit);
}

static void _sdcard_set_mosi(const uint32_t value) {
  _sdcard_set(SDBIT_MOSI, 1, value);
}

static void _sdcard_set_cs_(const uint32_t value) {
  _sdcard_set(SDBIT_CS_, 1, value);
}

static uint32_t _sdcard_get_miso(void) {
  return _sdcard_get(SDBIT_MISO, 1);
}

static void _sdcard_set_sck(const uint32_t value) {
  _sdcard_set(SDBIT_SCK, 1, value);
}

//--------------------------------------------------------------------------------------------------
// Low level SD card command interface.
//--------------------------------------------------------------------------------------------------

static void _sdcard_set_sck_slow(const uint32_t value) {
  // Sleep/delay to achieve a clock frequence close to 400 kHz.
  _sdcard_sleep(PERIOD_MICROS(1200));

  // Set the clock signal.
  _sdcard_set_sck(value);
}

static void _sdcard_sck_cycles(sdctx_t* ctx, const int num_cycles) {
  (void)ctx;  // Currently unused.
  for (int i = 0; i < num_cycles; ++i) {
    _sdcard_set_sck_slow(0);
    _sdcard_set_sck_slow(1);
  }
}

static void _sdcard_send_byte(sdctx_t* ctx, const uint32_t byte) {
  (void)ctx;  // Currently unused.
  for (int shift = 7; shift >= 0; --shift) {
    _sdcard_set_sck_slow(0);
    _sdcard_set_mosi((byte >> shift) & 1);
    _sdcard_set_sck_slow(1);
  }
}

static uint32_t _sdcard_receive_byte(sdctx_t* ctx) {
  (void)ctx;  // Currently unused.
  uint32_t byte = 0;
  for (int shift = 7; shift >= 0; --shift) {
    _sdcard_set_sck_slow(0);
    byte = (byte << 1) | _sdcard_get_miso();
    _sdcard_set_sck_slow(1);
  }
  return byte;
}

static bool _sdcard_send_cmd(sdctx_t* ctx, const uint8_t* cmd, const int len) {
  // Calculate the CRC first, since we may have to wait for the card to go ready anyway.
  const uint32_t crc = crc7(cmd, len);

  // Wait for the card to be ready to receive data (time out after a while).
  bool success = false;
  for (int i = 0; i < 10000; ++i) {
    const uint32_t bit = _sdcard_get_miso();
    if (bit == 1) {
      success = true;
      break;
    }
    _sdcard_sleep(PERIOD_MICROS(100));
  }
  if (!success) {
    SDCARD_LOG("SD: Send command timeout\n");
    return false;
  }

  // Send the command bytes (excluding the trailing CRC-byte).
  for (int i = 0; i < len; ++i) {
    _sdcard_send_byte(ctx, cmd[i]);
  }

  // Last byte of the command is always (CRC << 1) | 1.
  _sdcard_send_byte(ctx, (crc << 1) | 1);

  return true;
}

static bool _sdcard_get_response(sdctx_t* ctx, uint8_t* response, const int len) {
  // Drive MOSI high.
  _sdcard_set_mosi(1);

  // Wait for the start bit, i.e. the first 0-bit after 1-bits (time out after too many cycles).
  bool got_1_bit = false;
  bool success = false;
  for (int i = 20; i > 0; --i) {
    const uint32_t bit = _sdcard_get_miso();
    if (!got_1_bit && (bit == 1)) {
      got_1_bit = true;
    } else if (got_1_bit && (bit == 0)) {
      success = true;
      break;
    }
    _sdcard_sck_cycles(ctx, 1);
  }
  if (!success) {
    SDCARD_LOG("SD: Get response timeout\n");
    return false;
  }

  // Read the first byte (skip first zero-bit, because we already got it).
  uint32_t value = 0;
  for (int i = 1; i < 8; ++i) {
    _sdcard_sck_cycles(ctx, 1);
    const int bit = 7 - (i & 7);
    value |= _sdcard_get_miso() << bit;
  }
  response[0] = value;

  // Read the rest response bytes.
  for (int i = 1; i < len; ++i) {
    value = _sdcard_receive_byte(ctx);
    response[i] = value;
  }

  // Run 8 dummy cycles to let the operation terminate.
  _sdcard_set_mosi(1);
  _sdcard_sck_cycles(ctx, 8);

  return true;
}

static void _sdcard_dump_r1(sdctx_t* ctx, const uint8_t r) {
#ifdef SDCARD_ENABLE_DEBUGGING
  if ((r & 0x01) != 0) {
    SDCARD_DEBUG("response: Idle\n");
  }
  if ((r & 0x02) != 0) {
    SDCARD_DEBUG("response: Erase reset\n");
  }
  if ((r & 0x04) != 0) {
    SDCARD_DEBUG("response: Illegal command\n");
  }
  if ((r & 0x08) != 0) {
    SDCARD_DEBUG("response: CRC error\n");
  }
  if ((r & 0x10) != 0) {
    SDCARD_DEBUG("response: Erase sequence error\n");
  }
  if ((r & 0x20) != 0) {
    SDCARD_DEBUG("response: Address error\n");
  }
  if ((r & 0x40) != 0) {
    SDCARD_DEBUG("response: Permanent error\n");
  }
#else
  (void)ctx;
  (void)r;
#endif
}

//--------------------------------------------------------------------------------------------------
// Implementation of specific SD card commands.
//--------------------------------------------------------------------------------------------------

bool _sdcard_cmd0(sdctx_t* ctx, const int retries) {
  SDCARD_DEBUG("SD: Send CMD0\n");

  for (int i = 0; i < retries; ++i) {
    // Send command.
    uint8_t cmd[5] = {0x40, 0x00, 0x00, 0x00, 0x00};
    if (!_sdcard_send_cmd(ctx, cmd, sizeof(cmd))) {
      return false;
    }

    // Get response (R1).
    uint8_t resp[1];
    if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
      return false;
    }
    const int r = resp[0];
    if (r == 0x01) {
      return true;
    }
    _sdcard_dump_r1(ctx, r);
  }

  return false;
}

bool _sdcard_cmd8(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD8\n");

  // Send command.
  uint8_t cmd[5] = {0x48, 0x00, 0x00, 0x01, 0xaa};
  if (!_sdcard_send_cmd(ctx, cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R7).
  uint8_t resp[5];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] == 0x01) {
    // Version 2+.
    ctx->protocol_version = 2;
    SDCARD_DEBUG("CMD8: Version 2.0+\n");
    if (resp[1] != cmd[1] || resp[2] != cmd[2] || resp[3] != cmd[3] || resp[4] != cmd[4]) {
      SDCARD_LOG("CMD8: Invalid response\n");
      return false;
    }
  } else {
    // Version 1.
    ctx->protocol_version = 1;
    SDCARD_DEBUG("CMD8: Version 1\n");
    _sdcard_dump_r1(ctx, resp[0]);
  }

  return true;
}

bool _sdcard_cmd55(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD55\n");

  // Send command.
  uint8_t cmd[5] = {0x77, 0x00, 0x00, 0x00, 0x00};
  if (!_sdcard_send_cmd(ctx, cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] == 0x05) {
    // We must use CMD1 instead.
    ctx->use_cmd1 = true;
  } else if (resp[0] != 0x01) {
    SDCARD_LOG("CMD55: Unexpected response\n");
    return false;
  }

  return true;
}

bool _sdcard_acmd41(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send ACMD41\n");

  // Send command.
  uint8_t cmd[5] = {0x69, 0x40, 0x00, 0x00, 0x00};
  if (!_sdcard_send_cmd(ctx, cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] == 0x00) {
    return true;
  } else if (resp[0] == 0x01) {
    return false;
  } else if (resp[0] == 0x05) {
    // We must use CMD1 instead.
    ctx->use_cmd1 = true;
    return false;
  } else {
    SDCARD_LOG("ACMD41: Unexpected response\n");
    return false;
  }
}

bool _sdcard_cmd58(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD58\n");

  // Send command.
  uint8_t cmd[5] = {0x7a, 0x00, 0x00, 0x00, 0x00};
  if (!_sdcard_send_cmd(ctx, cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R3).
  uint8_t resp[5];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] != 0x00) {
    SDCARD_LOG("CMD58: Unexpected response\n");
    return false;
  }

  // Check if the card is using high capacity addressing.
  if ((resp[1] & 0x40) != 0) {
    SDCARD_DEBUG("SD: The card type is SDHC\n");
    ctx->is_sdhc = true;
  } else {
    ctx->is_sdhc = false;
  }

  return true;
}


bool _sdcard_cmd16(sdctx_t* ctx, const uint32_t block_size) {
  SDCARD_DEBUG("SD: Send CMD16\n");

  // Send command.
  uint8_t cmd[5] = {0x50, block_size >> 24, block_size >> 16, block_size >> 8, block_size};
  if (!_sdcard_send_cmd(ctx, cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] != 0x00) {
    SDCARD_LOG("CMD16: Unexpected response\n");
    return false;
  }

  return true;
}

bool _sdcard_cmd17(sdctx_t* ctx, const uint32_t block_addr) {
  SDCARD_DEBUG("SD: Send CMD17\n");

  // Send command.
  uint8_t cmd[5] = {0x51, block_addr >> 24, block_addr >> 16, block_addr >> 8, block_addr};
  if (!_sdcard_send_cmd(ctx, cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] != 0x00) {
    SDCARD_LOG("CMD17: Unexpected response\n");
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
// Reset/initialization routine.
//--------------------------------------------------------------------------------------------------

static bool _sdcard_reset(sdctx_t* ctx) {
  bool success = false;

  ctx->is_sdhc = false;

  // 1) Hold MOSI and CS* high for more than 74 cycles of "dummy-clock",
  //    and then pull CS* low.
  _sdcard_set_mosi(1);
  _sdcard_set_cs_(1);
  _sdcard_sck_cycles(ctx, 100);
  _sdcard_set_cs_(0);

  // 2) Send CMD0.
  if (!_sdcard_cmd0(ctx, 100)) {
    goto done;
  }

  // 3) Send CMD8 (configure voltage mode).
  if (!_sdcard_cmd8(ctx)) {
    goto done;
  }

  ctx->use_cmd1 = false;
  for (int i = 0; i < 10000; ++i) {
    // 4a) Send CMD55 (prefix for ACMD).
    if (!_sdcard_cmd55(ctx)) {
      goto done;
    }
    if (ctx->use_cmd1) {
      // TODO(m): Implement me! (use CMD1 instead of CMD55+ACMD41)
      SDCARD_LOG("SD: Old SD card - not supported\n");
      return 0;
    }

    // 4b) Send ACMD41.
    if (_sdcard_acmd41(ctx)) {
      success = true;
      break;
    }

    // Delay a bit before the next try.
    _sdcard_sleep(PERIOD_MICROS(1000));
  }

  if (success) {
    // 5) Check the card type (read the OCR).
    success = _sdcard_cmd58(ctx);
  }

done:
  // Pull CS* high again.
  _sdcard_set_cs_(1);

  if (success) {
    SDCARD_DEBUG("SD: Initialization succeeded!\n");
  } else {
    SDCARD_DEBUG("SD: Initialization failed\n");
  }

  return success;
}

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

bool sdcard_init(sdctx_t* ctx, sdcard_log_func_t log_func) {
  ctx->log_func = log_func;

  // Set up port directions for SPI mode (as seen from the host/master).
  // Note: SCK is always in output mode.
  _sdcard_dir(SDBIT_MISO, 1, DIR_IN);
  _sdcard_dir(SDBIT_CS_, 1, DIR_OUT);
  _sdcard_dir(SDBIT_MOSI, 1, DIR_OUT);

  // Try to reset the card (if any is connected).
  return _sdcard_reset(ctx);
}

bool sdcard_read(sdctx_t* ctx, void* ptr, size_t first_block, size_t num_blocks) {
  bool success = false;

  // Pull CS* low.
  _sdcard_set_cs_(0);

  // Set block size.
  if (!_sdcard_cmd16(ctx, SD_BLOCK_SIZE)) {
    // TODO(m): We should probably try _sdcard_reset() here and retry.
    goto done;
  }

  uint8_t* buf = (uint8_t*)ptr;

  const size_t block_end = first_block + num_blocks;
  for (size_t block_no = first_block; block_no < block_end; ++block_no) {
    // Set the block address, depending on if we're using HC or standard capacity.
    const uint32_t block_addr = ctx->is_sdhc ? block_no : block_no * SD_BLOCK_SIZE;
    if (!_sdcard_cmd17(ctx, block_addr)) {
      goto done;
    }

    // Wait for the response token.
    uint32_t token = 0xff;
    for (int i = 0; i < 1000; ++i) {
      token = _sdcard_receive_byte(ctx);
      if (token != 0xff) {
        break;
      }
    }
    if (token != 0xfe) {
      // The card returned an error response.
      SDCARD_LOG("SD: Read error\n");
      _sdcard_send_byte(ctx, 0xff);
      goto done;
    }

    // Read one block.
    for (int i = 0; i < SD_BLOCK_SIZE; ++i) {
      buf[i] = _sdcard_receive_byte(ctx);
    }

    // Send a couple of dummy bytes to terminate the transfer.
    for (int i = 0; i < 2; ++i) {
      _sdcard_send_byte(ctx, 0xff);
    }

    buf += SD_BLOCK_SIZE;
  }

  success = true;

done:
  // Pull CS* high again.
  _sdcard_set_cs_(1);

  return success;
}

bool sdcard_write(sdctx_t* ctx, const void* ptr, size_t first_block, size_t num_blocks) {
  // TODO(m): Implement me!
  (void)ctx;
  (void)ptr;
  (void)first_block;
  (void)num_blocks;
  return false;
}
