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

//#define SDCARD_ENABLE_LOGGING
//#define SDCARD_ENABLE_DEBUGGING

#ifdef SDCARD_ENABLE_LOGGING
static inline void _sdcard_log(const sdctx_t* ctx, const char* msg) {
  const sdcard_log_func_t log_func = ctx->log_func;
  if (log_func != (sdcard_log_func_t)0) {
    log_func(msg);
  }
}

#ifdef SDCARD_ENABLE_DEBUGGING
static void _sdcard_log_num(const sdctx_t* ctx, int x) {
  if (ctx->log_func == (sdcard_log_func_t)0) {
    return;
  }

  // We do this in a very manual and roundabout way to avoid using the standard C library.
  char buf[16];

  bool is_neg = (x < 0);
  if (is_neg) {
    x = -x;
  }

  int k = 16;
  buf[--k] = 0;
  do {
    int d = x % 10;
    x /= 10;
    buf[--k] = d + 48;
  } while (x != 0);
  if (is_neg) {
    buf[--k] = '-';
  }

  ctx->log_func(&buf[k]);
}
#endif

#define SDCARD_LOG(msg) _sdcard_log(ctx, msg)
#ifdef SDCARD_ENABLE_DEBUGGING
#define SDCARD_DEBUG(msg) _sdcard_log(ctx, msg)
#define SDCARD_DEBUG_NUM(x) _sdcard_log_num(ctx, x)
#else
#define SDCARD_DEBUG(msg) ((void)(ctx))
#define SDCARD_DEBUG_NUM(x) ((void)(ctx))
#endif
#else
#define SDCARD_LOG(msg) ((void)(ctx))
#define SDCARD_DEBUG(msg) ((void)(ctx))
#define SDCARD_DEBUG_NUM(x) ((void)(ctx))
#endif

//--------------------------------------------------------------------------------------------------
// Timing helpers.
//--------------------------------------------------------------------------------------------------

// This converts a nanosecond value to a loop count for an MRISC32-A1 CPU running at about 100 MHz.
// I.e. it is very appoximate.
#define PERIOD_NS(ns) (((ns) + 10) / 20)

#define SD_HPERIOD_400KHZ 1200  // 1/(2*1200ns) ~= 400 kHz

#ifdef __GNUC__
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif

static FORCE_INLINE void _sdio_sleep(int period) {
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

static void _sdio_dir_in(const uint32_t bits) {
  MMIO(SDWE) &= ~bits;
}

static void _sdio_dir_out(const uint32_t bits) {
  MMIO(SDWE) |= bits;
}

static void _sdio_set_mosi(const uint32_t value) {
  const uint32_t sdout = MMIO(SDOUT);
  if (value) {
    MMIO(SDOUT) = sdout | SD_MOSI_BIT;
  } else {
    MMIO(SDOUT) = sdout & ~SD_MOSI_BIT;
  }
}

static uint32_t _sdio_get_miso(void) {
  return (MMIO(SDIN) >> SD_MISO_BIT_NO) & 1;
}

static void _sdio_set_cs_0(void) {
  MMIO(SDOUT) &= ~SD_CS_BIT;
}

static void _sdio_set_cs_1(void) {
  MMIO(SDOUT) |= SD_CS_BIT;
}

static void _sdio_set_sck_0(void) {
  MMIO(SDOUT) &= ~SD_SCK_BIT;
}

static void _sdio_set_sck_1(void) {
  MMIO(SDOUT) |= SD_SCK_BIT;
}

//--------------------------------------------------------------------------------------------------
// Low level SD card command interface.
//--------------------------------------------------------------------------------------------------

static void _sdcard_sck_cycles_slow(const int num_cycles) {
  for (int i = 0; i < num_cycles; ++i) {
    _sdio_sleep(PERIOD_NS(SD_HPERIOD_400KHZ));
    _sdio_set_sck_0();
    _sdio_sleep(PERIOD_NS(SD_HPERIOD_400KHZ));
    _sdio_set_sck_1();
  }
}

static void _sdcard_send_byte(const uint32_t byte) {
  for (int shift = 7; shift >= 0; --shift) {
    _sdio_set_mosi((byte >> shift) & 1);
    _sdio_set_sck_0();
    _sdio_sleep(PERIOD_NS(SD_HPERIOD_400KHZ));
    _sdio_set_sck_1();
    _sdio_sleep(PERIOD_NS(SD_HPERIOD_400KHZ));
  }
}

static uint32_t _sdcard_receive_byte(void) {
  uint32_t byte = 0;
  for (int i = 8; i > 0; --i) {
    _sdio_set_sck_0();
    _sdio_sleep(PERIOD_NS(SD_HPERIOD_400KHZ));
    _sdio_set_sck_1();
    byte = (byte << 1) | _sdio_get_miso();
    _sdio_sleep(PERIOD_NS(SD_HPERIOD_400KHZ));
  }
  return byte;
}

static uint32_t _sdcard_receive_byte_fast(void) {
  uint32_t sck_hi = SD_MOSI_BIT | SD_SCK_BIT;  // MOSI high, CS* low, SCK high
  uint32_t sck_lo = SD_MOSI_BIT;               // MOSI high, CS* low, SCK low
#ifdef __MRISC32__
  // This assembler optimized routine takes five CPU clock cycles per bit, with a 40:60 SCK cycle.
  uint32_t mmio = 0xc0000000;
  uint32_t byte;
  uint32_t tmp;
  __asm__ volatile(
      "   stw   %[sck_lo], %[mmio], #100\n"  // MMIO(SDOUT) = sck_lo
      "   nop\n"
      "   nop\n"
      "   stw   %[sck_hi], %[mmio], #100\n"  // MMIO(SDOUT) = sck_hi
      "   ldw   %[byte], %[mmio], #60\n"     // byte = MMIO(SDIN) (bit #0 is MISO)

      "   stw   %[sck_lo], %[mmio], #100\n"
      "   mkbf  %[byte], %[byte], #(1<<5)|7\n"
      "   nop\n"
      "   stw   %[sck_hi], %[mmio], #100\n"
      "   ldw   %[tmp], %[mmio], #60\n"

      "   stw   %[sck_lo], %[mmio], #100\n"
      "   mkbf  %[tmp], %[tmp], #(1<<5)|6\n"
      "   or    %[byte], %[byte], %[tmp]\n"
      "   stw   %[sck_hi], %[mmio], #100\n"
      "   ldw   %[tmp], %[mmio], #60\n"

      "   stw   %[sck_lo], %[mmio], #100\n"
      "   mkbf  %[tmp], %[tmp], #(1<<5)|5\n"
      "   or    %[byte], %[byte], %[tmp]\n"
      "   stw   %[sck_hi], %[mmio], #100\n"
      "   ldw   %[tmp], %[mmio], #60\n"

      "   stw   %[sck_lo], %[mmio], #100\n"
      "   mkbf  %[tmp], %[tmp], #(1<<5)|4\n"
      "   or    %[byte], %[byte], %[tmp]\n"
      "   stw   %[sck_hi], %[mmio], #100\n"
      "   ldw   %[tmp], %[mmio], #60\n"

      "   stw   %[sck_lo], %[mmio], #100\n"
      "   mkbf  %[tmp], %[tmp], #(1<<5)|3\n"
      "   or    %[byte], %[byte], %[tmp]\n"
      "   stw   %[sck_hi], %[mmio], #100\n"
      "   ldw   %[tmp], %[mmio], #60\n"

      "   stw   %[sck_lo], %[mmio], #100\n"
      "   mkbf  %[tmp], %[tmp], #(1<<5)|2\n"
      "   or    %[byte], %[byte], %[tmp]\n"
      "   stw   %[sck_hi], %[mmio], #100\n"
      "   ldw   %[tmp], %[mmio], #60\n"

      "   stw   %[sck_lo], %[mmio], #100\n"
      "   mkbf  %[tmp], %[tmp], #(1<<5)|1\n"
      "   or    %[byte], %[byte], %[tmp]\n"
      "   stw   %[sck_hi], %[mmio], #100\n"
      "   ldw   %[tmp], %[mmio], #60\n"

      "   and   %[tmp], %[tmp], #1\n"
      "   or    %[byte], %[byte], %[tmp]\n"
      : [ byte ] "=r"(byte), [ tmp ] "=&r"(tmp)
      : [ sck_hi ] "r"(sck_hi), [ sck_lo ] "r"(sck_lo), [ mmio ] "r"(mmio));
#else
  uint32_t byte = 0;
  for (int i = 8; i > 0; --i) {
    MMIO(SDOUT) = sck_lo;
    byte = byte << 1;
    // We insert a scheduling barrier here to better balance the SCK hi/lo periods.
    __asm__ volatile("" : : : "memory");
    MMIO(SDOUT) = sck_hi;
    byte |= _sdio_get_miso();
  }
#endif
  return byte;
}

static void _sdcard_terminate_operation(void) {
  // Send 8 dummy cycles to terminate.
  _sdio_set_mosi(1);
  _sdcard_sck_cycles_slow(8);
}

static void _sdcard_select_card(void) {
  _sdio_sleep(PERIOD_NS(1000));
  _sdio_set_cs_0();
}

static void _sdcard_deselect_card(void) {
  _sdio_set_cs_1();
  _sdcard_terminate_operation();
}

static bool _sdcard_send_cmd(const uint32_t cmd, const uint32_t value) {
  // Prepare the command buffer.
  uint8_t buf[6];
  buf[0] = 0x40 | cmd;
  buf[1] = value >> 24;
  buf[2] = value >> 16;
  buf[3] = value >> 8;
  buf[4] = value;
  buf[5] = (crc7(buf, 5) << 1) | 1;

  // Make the card ready to receive data. (?)
  (void)_sdcard_receive_byte();

  // Send the command bytes, including the trailing CRC-byte.
  for (int i = 0; i < 6; ++i) {
    _sdcard_send_byte(buf[i]);
  }

  return true;
}

static bool _sdcard_get_response(sdctx_t* ctx, uint8_t* response, const int len) {
  // Wait for the start bit, i.e. the first 0-bit after 1-bits (time out after too many cycles).
  bool got_1_bit = false;
  bool success = false;
  for (int i = 200; i > 0; --i) {
    const uint32_t bit = _sdio_get_miso();
    if (!got_1_bit && (bit == 1)) {
      got_1_bit = true;
    } else if (got_1_bit && (bit == 0)) {
      success = true;
      break;
    }
    _sdcard_sck_cycles_slow(1);
  }
  if (!success) {
    SDCARD_LOG("SD: Get response timeout\n");
    return false;
  }

  // Read the first byte (skip first zero-bit, because we already got it).
  uint32_t value = 0;
  for (int i = 1; i < 8; ++i) {
    _sdcard_sck_cycles_slow(1);
    const int bit = 7 - (i & 7);
    value |= _sdio_get_miso() << bit;
  }
  response[0] = value;

  // Read the rest response bytes.
  for (int i = 1; i < len; ++i) {
    value = _sdcard_receive_byte();
    response[i] = value;
  }

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

static bool _sdcard_wait_for_token(const uint32_t token) {
  uint32_t byte = 0xff;
  for (int i = 0; i < 1000; ++i) {
    byte = _sdcard_receive_byte();
    if (byte == token) {
      return true;
    }
  }
  return false;
}

static bool _sdcard_read_data_block(sdctx_t* ctx, uint8_t* buf, size_t num_bytes) {
  // Wait for the response token.
  if (!_sdcard_wait_for_token(0xfe)) {
    SDCARD_LOG("SD: Read data token timeout\n");
    return false;
  }

  // Read a single block of data.
  // Use fast transfer if the SD card can do 10+ Mbit/s, otherwise slow transfer.
  if (ctx->transfer_kbit >= 10000) {
    for (size_t i = 0; i < num_bytes; ++i) {
      buf[i] = _sdcard_receive_byte_fast();
    }
  } else {
    for (size_t i = 0; i < num_bytes; ++i) {
      buf[i] = _sdcard_receive_byte();
    }
  }

  // Skip the trailing CRC16 (2 bytes).
  for (int i = 0; i < 2; ++i) {
    (void)_sdcard_receive_byte();
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
// Implementation of specific SD card commands.
//--------------------------------------------------------------------------------------------------

static bool _sdcard_cmd0(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD0\n");

  // Send command.
  if (!_sdcard_send_cmd(0, 0)) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  // Check that the card is in idle state.
  if (resp[0] != 0x01) {
    return false;
  }

  return true;
}

static bool _sdcard_cmd8(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD8\n");

  // Send command.
  if (!_sdcard_send_cmd(8, 0x000001aa)) {
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
    if (resp[1] != 0 || resp[2] != 0 || resp[3] != 0x01 || resp[4] != 0xaa) {
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

static bool _sdcard_cmd9(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD9\n");

  // Send command.
  if (!_sdcard_send_cmd(9, 0)) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if ((resp[0] & 0xfe) != 0) {
    SDCARD_LOG("CMD9: Unexpected response\n");
    _sdcard_dump_r1(ctx, resp[0]);
    return false;
  }

  return true;
}

static bool _sdcard_cmd55(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD55\n");

  // Send command.
  if (!_sdcard_send_cmd(55, 0)) {
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

static bool _sdcard_acmd41(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send ACMD41\n");

  // Send command (request HCS=1, i.e. set bit 30).
  if (!_sdcard_send_cmd(41, 0x40000000)) {
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

static bool _sdcard_cmd58(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD58\n");

  // Send command.
  if (!_sdcard_send_cmd(58, 0)) {
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

static bool _sdcard_cmd16(sdctx_t* ctx, const uint32_t block_size) {
  SDCARD_DEBUG("SD: Send CMD16\n");

  // Send command.
  if (!_sdcard_send_cmd(16, block_size)) {
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

static bool _sdcard_cmd17(sdctx_t* ctx, const uint32_t block_addr) {
  SDCARD_DEBUG("SD: Send CMD17\n");

  // Send command.
  if (!_sdcard_send_cmd(17, block_addr)) {
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

static bool _sdcard_cmd18(sdctx_t* ctx, const uint32_t block_addr) {
  SDCARD_DEBUG("SD: Send CMD18\n");

  // Send command.
  if (!_sdcard_send_cmd(18, block_addr)) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] != 0x00) {
    SDCARD_LOG("CMD18: Unexpected response\n");
    return false;
  }

  return true;
}

static bool _sdcard_cmd12(sdctx_t* ctx) {
  SDCARD_DEBUG("SD: Send CMD12\n");

  // Send command.
  if (!_sdcard_send_cmd(12, 0)) {
    return false;
  }

  // Get response (R1).
  uint8_t resp[1];
  if (!_sdcard_get_response(ctx, resp, sizeof(resp))) {
    return false;
  }

  if (resp[0] != 0x00) {
    SDCARD_LOG("CMD12: Unexpected response\n");
    return false;
  }

  return true;
}

// Constant LUT:s used for decoding TRAN_SPEED values.
static const uint16_t TRAN_SPEED_UNIT[8] = {10, 100, 1000, 10000, 0, 0, 0, 0};
static const uint8_t TRAN_SPEED_SCALE[16] =
    {0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};

static bool _sdcard_read_csd(sdctx_t* ctx) {
  // As a response to CMD9, the CSD register is sent as a single block read operation, with a data
  // start token, a 16-byte data block (the 128-bit register) and a trailing CRC16.
  uint8_t csd[16];
  if (!_sdcard_read_data_block(ctx, &csd[0], sizeof(csd))) {
    return false;
  }

  // CSD register bit mappings:
  //   csd[0]:  127:120
  //   csd[1]:  119:112
  //   csd[2]:  111:104
  //   csd[3]:  103:96
  //   csd[4]:  95:88
  //   csd[5]:  87:80
  //   csd[6]:  79:72
  //   csd[7]:  71:64
  //   csd[8]:  63:56
  //   csd[9]:  55:48
  //   csd[10]: 47:40
  //   csd[11]: 39:32
  //   csd[12]: 31:24
  //   csd[13]: 23:16
  //   csd[14]: 15:8
  //   csd[15]: 7:0

  // Decode CSD_STRUCTURE version (bits 127:126, byte 0).
  int csd_structure = csd[0] >> 6;
  SDCARD_DEBUG("SD: CSD_STRUCTURE=");
  SDCARD_DEBUG_NUM(csd_structure);
  SDCARD_DEBUG("\n");

  // Get card capacity parameters. These are dependent on the CSD_STRUCTURE version.
  size_t c_size;
  int c_size_mult;
  if (csd_structure == 0) {
    // (According to Physical Layer Simplified Specification Version 8.00, p. 191).
    // C_SIZE, bits 73:62 (in bytes 6, 7 and 8).
    c_size = ((((size_t)csd[6]) & 0x03) << 10) | (((size_t)csd[7]) << 2) | (((size_t)csd[8]) >> 6);

    // C_SIZE_MULT, bits 49:47 (in bytes 9 and 10).
    c_size_mult = (int)(((csd[9] & 0x03) << 1) | (csd[10] >> 7));
  } else if (csd_structure == 1) {
    // (According to Physical Layer Simplified Specification Version 8.00, p. 196).
    // C_SIZE, bits 69:48 (in bytes 7, 8 and 9).
    c_size = ((((size_t)csd[7]) & 0x3f) << 16) | (((size_t)csd[8]) << 8) | (size_t)csd[9];

    // C_SIZE_MULT=8 corresponds to 1024 blocks (512 KiB).
    c_size_mult = 8;
  } else if (csd_structure == 2) {
    // (According to Physical Layer Simplified Specification Version 8.00, p. 199).
    // C_SIZE, bits 75:48 (in bytes 6, 7, 8 and 9).
    c_size = ((((size_t)csd[6]) & 0x0f) << 24) | (((size_t)csd[7]) << 16) |
             (((size_t)csd[8]) << 8) | (size_t)csd[9];

    // C_SIZE_MULT=8 corresponds to 1024 blocks (512 KiB).
    c_size_mult = 8;
  } else {
    SDCARD_DEBUG("SD: Unsupported CSD_STRUCTURE version\n");
    return false;
  }

  // READ_BL_LEN, bits 83:80 (in byte 5).
  // Note: This is guaranteed to be 9 (block size = 512 bytes) when CSD_STRUCTURE=1 or 2.
  const int read_bl_len = csd[5] & 0x0f;

  // Decode transfer speed.
  // TRAN_SPEED (bits 103:96, byte 3).
  //   Bits 2:0: Unit (0=100kbit/s, 1=1Mbit/s, 2=10Mbit/s, 3=100Mbit/s, 4-7=reserved).
  //   Bits 6:3: Scale (0=reserved, 1=1.0, 2=1.2, 3=1.3, 4=1.5, ... see LUT).
  const size_t tran_speed = csd[3];
  ctx->transfer_kbit =
      (size_t)TRAN_SPEED_UNIT[tran_speed & 0x07] * (size_t)TRAN_SPEED_SCALE[tran_speed >> 3];
  SDCARD_DEBUG("SD: ");
  SDCARD_DEBUG_NUM(ctx->transfer_kbit);
  SDCARD_DEBUG(" kbit/s\n");

  // Calculate the total capacity, counted as number of 512-byte blocks.
  ctx->num_blocks = (c_size + 1) << (c_size_mult + read_bl_len - 9 + 2);

  SDCARD_DEBUG("SD: C_SIZE=");
  SDCARD_DEBUG_NUM(c_size);
  SDCARD_DEBUG(", C_SIZE_MULT=");
  SDCARD_DEBUG_NUM(c_size_mult);
  SDCARD_DEBUG(", READ_BL_LEN=");
  SDCARD_DEBUG_NUM(read_bl_len);
  SDCARD_DEBUG(", num_blocks=");
  SDCARD_DEBUG_NUM(ctx->num_blocks);
  SDCARD_DEBUG("\n");

  return true;
}

//--------------------------------------------------------------------------------------------------
// Reset/initialization routine.
//--------------------------------------------------------------------------------------------------

static bool _sdcard_reset(sdctx_t* ctx) {
  bool success = false;
  int retry;

  // Initialize the SD context with default values.
  ctx->num_blocks = 0;
  ctx->transfer_kbit = 100;
  ctx->protocol_version = 1;
  ctx->use_cmd1 = false;
  ctx->is_sdhc = false;

  // 1) Hold MOSI and CS* high for more than 74 cycles of "dummy-clock",
  //    and then pull CS* low.
  _sdio_set_mosi(1);
  _sdio_set_cs_1();
  _sdcard_sck_cycles_slow(100);
  _sdio_set_cs_0();

  // 2) Send CMD0. We try many times until we get success, or time out.
  for (retry = 1000; retry > 0; --retry) {
    if (_sdcard_cmd0(ctx)) {
      break;
    }
    _sdio_sleep(PERIOD_NS(10000));
  }
  if (retry <= 0) {
    goto done;
  }

  // 3) Send CMD8 (configure voltage mode).
  if (!_sdcard_cmd8(ctx)) {
    goto done;
  }

  ctx->use_cmd1 = false;
  for (retry = 10000; retry > 0; --retry) {
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
      break;
    }

    // Delay a bit before the next try.
    _sdio_sleep(PERIOD_NS(1000));
  }
  if (retry <= 0) {
    goto done;
  }

  // 5) Send CMD58 (READ_OCR) to determine CCS value (block addressing mode).
  if (!_sdcard_cmd58(ctx)) {
    goto done;
  }

  // 6) Send CMD9 (SEND_CSD) to determine the card size and speed.
  if (!_sdcard_cmd9(ctx)) {
    goto done;
  }
  if (!_sdcard_read_csd(ctx)) {
    goto done;
  }

  // 7) Set block size.
  if (!_sdcard_cmd16(ctx, SD_BLOCK_SIZE)) {
    goto done;
  }

  success = true;

done:
  // Deselect card again.
  _sdcard_deselect_card();

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
  _sdio_dir_in(SD_MISO_BIT);
  _sdio_dir_out(SD_CS_BIT | SD_MOSI_BIT);

  // Try to reset the card (if any is connected).
  return _sdcard_reset(ctx);
}

size_t sdcard_get_size(sdctx_t* ctx) {
  return ctx->num_blocks;
}

bool sdcard_read(sdctx_t* ctx, void* ptr, size_t first_block, size_t num_blocks) {
  if (num_blocks == 0) {
    return true;
  }

  bool success = false;

  // Select card.
  _sdcard_select_card();

  // Set the start block address and initiate read (retry with a reset if necessary).
  const uint32_t block_addr = ctx->is_sdhc ? first_block : first_block * SD_BLOCK_SIZE;
  int retry;
  for (retry = 2; retry > 0; --retry) {
    if (num_blocks > 1) {
      // Use CMD18 (READ_MULTIPLE_BLOCK) when num_blocks > 1.
      if (_sdcard_cmd18(ctx, block_addr)) {
        break;
      }
    } else {
      // Use CMD17 (READ_SINGLE_BLOCK) when num_blocks == 1.
      if (_sdcard_cmd17(ctx, block_addr)) {
        break;
      }
    }

    // Try to reset the card and retry the command once more.
    _sdcard_reset(ctx);
  }
  if (retry <= 0) {
    goto done;
  }

  uint8_t* buf = (uint8_t*)ptr;
  for (size_t blk = 0; blk < num_blocks; ++blk) {
    if (!_sdcard_read_data_block(ctx, buf, SD_BLOCK_SIZE)) {
      goto terminate;
    }

    buf += SD_BLOCK_SIZE;
  }

  if (num_blocks > 1) {
    // We must wait for a read token before we can send CMD12.
    if (!_sdcard_wait_for_token(0xfe)) {
      SDCARD_LOG("SD: Read token timeout (CMD 12)\n");
      goto done;
    }

    // Terminate READ_MULTIPLE_BLOCK.
    if (!_sdcard_cmd12(ctx)) {
      goto done;
    }
  }

  success = true;

terminate:
  _sdcard_terminate_operation();

done:
  _sdcard_deselect_card();

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
