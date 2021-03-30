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

#ifndef MC1_SDCARD_H_
#define MC1_SDCARD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Logging callback function for SD card functions.
typedef void (*sdcard_log_func_t)(const char* msg);

// The contents of this struct is private and subject to change. Do not access members directly!
typedef struct {
  sdcard_log_func_t log_func;  // Optional logging callback function (NULL to disable).
  size_t num_blocks;           // Card capacity (number of blocks).
  int transfer_kbit;           // Max transfer rate (kbit/s).
  uint8_t protocol_version;    // Protocol version (1 = v1.0, 2 = v2.0, 3 = v3.0, ...).
  bool is_sdhc;                // If true, data addresses are in blocks, otherwise in bytes.
  bool use_cmd1;               // If true, initialization should use CMD1 instead of ACMD41.
} sdctx_t;

/// @brief Initialize the SD card driver.
/// @param ctx An SD card context that is initialized.
/// @param log_fun An optional log printer function (pass NULL to disable).
/// @returns a non-zero value if the initialization was successful.
bool sdcard_init(sdctx_t* ctx, sdcard_log_func_t log_fun);

/// @brief Get the size of the active SD card.
/// @param ctx An SD card context that has been initialized.
/// @returns the size, in 512-byte blocks, of the SD card, or zero if no SD card is present.
size_t sdcard_get_size(sdctx_t* ctx);

/// @brief Read one or more 512 byte blocks.
/// @param ctx An SD card context that has been initialized.
/// @param first_block The first block to read.
/// @param num_blocks The number of block to read.
/// @returns a non-zero value if the initialization was successful.
bool sdcard_read(sdctx_t* ctx, void* ptr, size_t first_block, size_t num_blocks);

/// @brief Write one or more 512 byte blocks.
/// @param ctx An SD card context that has been initialized.
/// @param first_block The first block to write.
/// @param num_blocks The number of block to write.
/// @returns a non-zero value if the initialization was successful.
bool sdcard_write(sdctx_t* ctx, const void* ptr, size_t first_block, size_t num_blocks);

#ifdef __cplusplus
}
#endif

#endif  // MC1_SDCARD_H_
