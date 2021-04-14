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

//--------------------------------------------------------------------------------------------------
// Declarations for writing MC1 boot block code in C.
//--------------------------------------------------------------------------------------------------

#ifndef MC1_BOOT_H_
#define MC1_BOOT_H_

#include <stddef.h>

/// @brief Decoration for the main _boot() function.
///
/// Use as follows:
///
/// @code
/// void MC1_BOOT_FUNCTION _boot(const void* rom_base) {
///     ...
/// }
/// @endcode
#define MC1_BOOT_FUNCTION _Noreturn __attribute__((section(".text.start")))

/// @brief Catasrophic failure.
///
/// This calls the ROM function doh().
/// @param rom_base The ROM table base address.
/// @param msg The text message to display.
/// @note This function never returns.
static inline _Noreturn void doh(const void* rom_base, const char* msg) {
  register const char* arg1_ asm("s1") = msg;
  __asm__ volatile (
    "j       %[rom_base], #0"  // doh: ROM table offset #0
    :
    : [rom_base] "r"(rom_base),
      "r"(arg1_)
  );
  // TODO(m): Figure out a way to convince the compiler that this function never returns.
}

/// @brief Read blocks from a device.
/// @param rom_base The ROM table base address.
/// @param ptr Start of the memory area to read the data to.
/// @param device The device number to read from (0 = boot device).
/// @param first_block The first block to read.
/// @param num_block Number of blocks to read.
/// @returns a non-zero value if the operation succeeded.
/// @note The size of a block is 512 bytes.
static inline int blk_read(const void* rom_base,
                           void* ptr,
                           int device,
                           size_t first_block,
                           size_t num_block) {
  register void* arg1_ asm("s1") = ptr;
  register int arg2_ asm("s2") = device;
  register size_t arg3_ asm("s3") = first_block;
  register size_t arg4_ asm("s4") = num_block;
  register int result_ asm("s1");
  __asm__ volatile (
    "jl      %[rom_base], #4"  // blk_read: ROM table offset #4
    : [result] "=r"(result_)
    : [rom_base] "r"(rom_base),
      "r"(arg1_),
      "r"(arg2_),
      "r"(arg3_),
      "r"(arg4_)
    : "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "lr"
  );
  return result_;
}

/// @brief Calculate the CRC32C checksum.
/// @param rom_base The ROM table base address.
/// @param ptr Start of the buffer in memory.
/// @param num_bytes Number of bytes in the buffer.
/// @returns the checksum value.
static inline unsigned crc32c(const void* rom_base, void* ptr, size_t num_bytes) {
  register void* arg1_ asm("s1") = ptr;
  register size_t arg2_ asm("s2") = num_bytes;
  register unsigned result_ asm("s1");
  __asm__ volatile (
    "jl      %[rom_base], #8"  // crc32c: ROM table offset #8
    : [result] "=r"(result_)
    : [rom_base] "r"(rom_base),
      "r"(arg1_),
      "r"(arg2_)
    : "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "lr"
  );
  return result_;
}

/// @brief Decode an LZG compressed buffer.
/// @param rom_base The ROM table base address.
/// @param in Start of the source (compressed) buffer in memory.
/// @param insize Number of bytes in the input buffer.
/// @param out Start of the destination (decompressed) buffer in memory.
/// @param outsize Number of bytes in the output buffer.
/// @returns the number of decompressed bytes.
static inline unsigned LZG_Decode(const void* rom_base,
                                  const void* in,
                                  unsigned insize,
                                  void* out,
                                  unsigned outsize) {
  register const void* arg1_ asm("s1") = in;
  register unsigned arg2_ asm("s2") = insize;
  register void* arg3_ asm("s3") = out;
  register unsigned arg4_ asm("s4") = outsize;
  register unsigned result_ asm("s1");
  __asm__ volatile (
    "jl      %[rom_base], #12"  // LZG_Decode: ROM table offset #12
    : [result] "=r"(result_)
    : [rom_base] "r"(rom_base),
      "r"(arg1_),
      "r"(arg2_),
      "r"(arg3_),
      "r"(arg4_)
    : "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "lr"
  );
  return result_;
}

#endif  // MC1_BOOT_H_

