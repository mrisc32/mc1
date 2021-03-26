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

#include <stdbool.h>

//--------------------------------------------------------------------------------------------------
// Debug logging.
//--------------------------------------------------------------------------------------------------

#define SDCARD_ENABLE_LOGGING

#ifdef SDCARD_ENABLE_LOGGING
// This logger function is defined by sdcard_init().
static sdcard_log_func_t s_log_func;

static inline void _sdcard_log(const char* msg) {
  sdcard_log_func_t log_func = s_log_func;
  if (log_func != (sdcard_log_func_t)0) {
    log_func(msg);
  }
}

#define SDCARD_LOG(msg) _sdcard_log(msg)
#else
#define SDCARD_LOG(msg)
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
  // Sleep/delay to achieve a clock frequence close to 400 kHz.
  _sdcard_sleep(PERIOD_MICROS(1200));

  // Set the clock signal.
  _sdcard_set(SDBIT_SCK, 1, value);
}

static void _sdcard_sck_cycles(const int num_cycles) {
  for (int i = 0; i < num_cycles; ++i) {
    _sdcard_set_sck(0);
    _sdcard_set_sck(1);
  }
}

//--------------------------------------------------------------------------------------------------
// Low level SD card command interface.
//--------------------------------------------------------------------------------------------------

static void _sdcard_send_byte(const uint32_t byte) {
  for (int shift = 7; shift >= 0; --shift) {
    _sdcard_set_sck(0);
    _sdcard_set_mosi((byte >> shift) & 1);
    _sdcard_set_sck(1);
  }
}

static uint32_t _sdcard_receive_byte(void) {
  uint32_t byte = 0;
  for (int shift = 7; shift >= 0; --shift) {
    _sdcard_set_sck(0);
    byte = (byte << 1) | _sdcard_get_miso();
    _sdcard_set_sck(1);
  }
  return byte;
}

static bool _sdcard_send_cmd(const uint8_t* cmd, const int len) {
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
    SDCARD_LOG("send_cmd: Timeout\n");
    return false;
  }

  // Send the command bytes (excluding the trailing CRC-byte).
  for (int i = 0; i < len; ++i) {
    _sdcard_send_byte(cmd[i]);
  }

  // Last byte of the command is always (CRC << 1) | 1.
  _sdcard_send_byte((crc << 1) | 1);

  return true;
}

static bool _sdcard_get_response(uint8_t* response, const int len) {
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
    _sdcard_sck_cycles(1);
  }
  if (!success) {
    SDCARD_LOG("get_response: Timeout\n");
    return false;
  }

  // Read the first byte (skip first zero-bit, because we already got it).
  uint32_t value = 0;
  for (int i = 1; i < 8; ++i) {
    _sdcard_sck_cycles(1);
    const int bit = 7 - (i & 7);
    value |= _sdcard_get_miso() << bit;
  }
  response[0] = value;

  // Read the rest response bytes.
  for (int i = 1; i < len; ++i) {
    value = _sdcard_receive_byte();
    response[i] = value;
  }

	// Receive response tail (until we get a 0 MSB, or timeout).
	for (int i = 0; i < 10 && (value & 0x80) != 0; ++i) {
    value = _sdcard_receive_byte();
	}

  return true;
}

static void _sdcard_dump_r1(const uint8_t r) {
#ifdef SDCARD_ENABLE_LOGGING
  if ((r & 0x01) != 0) {
    SDCARD_LOG("response: Idle\n");
  }
  if ((r & 0x02) != 0) {
    SDCARD_LOG("response: Erase reset\n");
  }
  if ((r & 0x04) != 0) {
    SDCARD_LOG("response: Illegal command\n");
  }
  if ((r & 0x08) != 0) {
    SDCARD_LOG("response: CRC error\n");
  }
  if ((r & 0x10) != 0) {
    SDCARD_LOG("response: Erase sequence error\n");
  }
  if ((r & 0x20) != 0) {
    SDCARD_LOG("response: Address error\n");
  }
  if ((r & 0x40) != 0) {
    SDCARD_LOG("response: Permanent error\n");
  }
#else
  (void)r;
#endif
}

static bool _sdcard_read_data(uint8_t* buf, const int len) {
  // Wait for start bit (time out after too many cycles).
  bool success = false;
  for (int i = 1000; i > 0; --i) {
    _sdcard_sck_cycles(1);
    if (_sdcard_get_miso() == 0) {
      success = true;
      break;
    }
  }
  if (!success) {
    SDCARD_LOG("read_data: Start bits timeout\n");
    return false;
  }

  // Read data.
  for (int i = 0; i < len; ++i) {
    buf[i] = _sdcard_receive_byte();
  }

  // Check CRC (16 bits).
  // TODO(m): Implement me!
  _sdcard_sck_cycles(16);

  // Check end-bit (one bit == 1).
  // TODO(m): Implement me!
  _sdcard_sck_cycles(1);

  return true;
}

static bool _sdcard_write_data(const uint8_t* buf, const int len) {
  // Calculate the CRC for the data buffer.
  uint32_t crc = crc16(buf, len);

  // Send start bits.
  _sdcard_set_sck(0);
  _sdcard_set_mosi(0);
  _sdcard_set_sck(1);

  // Write data.
  for (int i = 0; i < len; ++i) {
    _sdcard_send_byte(buf[i]);
  }

  // Send CRC.
  _sdcard_send_byte(crc >> 8);
  _sdcard_send_byte(crc & 0xff);

  // Send stop bits.
  _sdcard_set_sck(0);
  _sdcard_set_mosi(15);
  _sdcard_set_sck(1);

  // Check busy bits (time out after too many cycles).
  bool success = false;
  for (int i = 32; i > 0; --i) {
    _sdcard_sck_cycles(1);
    // MISO is zero as long as the card is busy.
    if ((_sdcard_get_miso() & 1) == 1) {
      success = true;
      break;
    }
  }
  if (!success) {
    SDCARD_LOG("read_data: Busy bits timeout\n");
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
// Internal state.
//--------------------------------------------------------------------------------------------------

static int s_protocol_version;  // Set by CMD8.
static bool s_use_cmd1;         // Set by CMD55 or ACMD41.

//--------------------------------------------------------------------------------------------------
// Implementation of specific SD card commands.
//--------------------------------------------------------------------------------------------------

bool _sdcard_cmd0(const int retries) {
  SDCARD_LOG("SD: Send CMD0\n");

  for (int i = 0; i < retries; ++i) {
    // Send command.
    uint8_t cmd[5] = {0x40, 0x00, 0x00, 0x00, 0x00};
    if (!_sdcard_send_cmd(cmd, sizeof(cmd))) {
      return false;
    }

    // Get response (R1).
    uint8_t resp[1];
    if (!_sdcard_get_response(resp, sizeof(resp))) {
      return false;
    }
    const int r = resp[0];
    if (r == 0x01) {
      return true;
    }
    _sdcard_dump_r1(r);
  }

  return false;
}

bool _sdcard_cmd8() {
  SDCARD_LOG("SD: Send CMD8\n");

  // Send command.
  uint8_t cmd[5] = {0x48, 0x00, 0x00, 0x01, 0xaa};
  if (!_sdcard_send_cmd(cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R7).
  uint8_t resp[5];
  if (!_sdcard_get_response(resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] == 0x01) {
    // Version 2+.
    s_protocol_version = 2;
    SDCARD_LOG("CMD8: Version 2.0+\n");
    if (resp[1] != cmd[1] || resp[2] != cmd[2] || resp[3] != cmd[3] || resp[4] != cmd[4]) {
      SDCARD_LOG("CMD8: Invalid response\n");
      return false;
    }
  } else {
    // Version 1.
    s_protocol_version = 1;
    SDCARD_LOG("CMD8: Version 1\n");
    _sdcard_dump_r1(resp[0]);
  }

  return true;
}

bool _sdcard_cmd55() {
  SDCARD_LOG("SD: Send CMD55\n");

  // Send command.
  uint8_t cmd[5] = {0x77, 0x00, 0x00, 0x00, 0x00};
  if (!_sdcard_send_cmd(cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] == 0x05) {
    // We must use CMD1 instead.
    s_use_cmd1 = true;
  } else if (resp[0] != 0x01) {
    SDCARD_LOG("CMD55: Unexpected response\n");
    return false;
  }

  return true;
}

bool _sdcard_acmd41() {
  SDCARD_LOG("SD: Send ACMD41\n");

  // Send command.
  uint8_t cmd[5] = {0x69, 0x40, 0x00, 0x00, 0x00};
  if (!_sdcard_send_cmd(cmd, sizeof(cmd))) {
    return false;
  }

  // Get response (R3).
  uint8_t resp[1];
  if (!_sdcard_get_response(resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] == 0x00) {
    return true;
  } else if (resp[0] == 0x01) {
    return false;
  } else if (resp[0] == 0x05) {
    // We must use CMD1 instead.
    s_use_cmd1 = true;
    return false;
  } else {
    SDCARD_LOG("ACMD41: Unexpected response\n");
    return false;
  }
}

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

int sdcard_init(sdcard_log_func_t log_func) {
  s_log_func = log_func;

  // See "Part 1 Physical Layer Specification", Figure 4-2.

  // Set up port directions for SPI mode (as seen from the host/master).
  // Note: SCK is always in output mode.
  _sdcard_dir(SDBIT_MISO, 1, DIR_IN);
  _sdcard_dir(SDBIT_CS_, 1, DIR_OUT);
  _sdcard_dir(SDBIT_MOSI, 1, DIR_OUT);

  // TODO(m): Should we do the init phase in 400kHz?

  // 1) Hold MOSI and CS* high for more than 74 cycles of "dummy-clock",
  //    and then pull CS* low.
  _sdcard_set_mosi(1);
  _sdcard_set_cs_(1);
  _sdcard_sck_cycles(100);
  _sdcard_set_cs_(0);

  // 2) Send CMD0.
  if (!_sdcard_cmd0(100)) {
    return 0;
  }

  // 3) Send CMD8 (configure voltage mode).
  if (!_sdcard_cmd8()) {
    return 0;
  }

  s_use_cmd1 = false;
  bool success = false;
  for (int i = 0; i < 100; ++i) {
    // 4a) Send CMD55 (prefix for ACMD).
    if (!_sdcard_cmd55()) {
      return 0;
    }
    if (s_use_cmd1) {
      // TODO(m): Implement me! (use CMD1 instead of CMD55+ACMD41)
      SDCARD_LOG("SD: Old SD card - not supported\n");
      return 0;
    }

    // 4b) Send ACMD41.
    if (_sdcard_acmd41()) {
      success = true;
      break;
    }

    // Delay a bit before the next try.
    _sdcard_sleep(PERIOD_MICROS(1000));
  }
  if (!success) {
    SDCARD_LOG("SD: Initialization failed\n");
    return 0;
  }

  SDCARD_LOG("SD: Initialization succeeded!\n");

  return 1;
}

int sdcard_read(void* ptr, size_t first_block, size_t num_blocks) {
  // TODO(m): Implement me!
  (void)ptr;
  (void)first_block;
  (void)num_blocks;
  _sdcard_read_data((uint8_t*)ptr, 0);
  return 0;
}

int sdcard_write(const void* ptr, size_t first_block, size_t num_blocks) {
  // TODO(m): Implement me!
  (void)ptr;
  (void)first_block;
  (void)num_blocks;
  _sdcard_write_data((const uint8_t*)ptr, 0);
  return 0;
}
