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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sdcard_log_func_t)(const char* msg);

/// @brief Initialize the SD card driver.
/// @param log_fun An optional log printer function (pass NULL to disable).
/// @returns a non-zero value if the initialization was successful.
int sdcard_init(sdcard_log_func_t log_fun);

/// @brief Read one or more 512 byte blocks.
/// @param first_block The first block to read.
/// @param num_blocks The number of block to read.
/// @returns a non-zero value if the initialization was successful.
int sdcard_read(void* ptr, size_t first_block, size_t num_blocks);

/// @brief Write one or more 512 byte blocks.
/// @param first_block The first block to write.
/// @param num_blocks The number of block to write.
/// @returns a non-zero value if the initialization was successful.
int sdcard_write(const void* ptr, size_t first_block, size_t num_blocks);

#ifdef __cplusplus
}
#endif

#endif  // MC1_SDCARD_H_
