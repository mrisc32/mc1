// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
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

#ifndef MC1_MEMORY_H_
#define MC1_MEMORY_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fixed memory areas.
#define ROM_START  0x00000000
#define VRAM_START 0x40000000
#define XRAM_START 0x80000000

// Memory types.
#define MEM_TYPE_VIDEO 0x00000001 ///< Memory that can be accessed by the video hardware.
#define MEM_TYPE_EXT   0x00000002 ///< External memory.
#define MEM_TYPE_ANY   0x00000003 ///< Any memory type.

// Extra flags to mem_alloc().
#define MEM_CLEAR      0x00000100 ///< Clear the allocated memory (zero fill).

/// @brief Initialize the memory allocator.
/// @param vram_start The first free VRAM address.
/// @param vram_stop The number of free VRAM bytes.
void mem_init();

/// @brief Add a new memory pool.
/// @param start The first free address.
/// @param stop The number of free bytes.
/// @param type The memory type of the pool (e.g. @c MEM_TYPE_VIDEO).
void mem_add_pool(void* start, size_t size, unsigned type);

/// @brief Allocate one continuous block of memory.
/// @param num_bytes Number of bytes to allocate.
/// @param types Memory type(s) to allocate from (must be non-zero).
/// @returns the address of the allocated block, or NULL if no memory could be
/// allocated.
/// @note The allocated block is guaranteed to be aligned on a 4-byte boundary.
void* mem_alloc(size_t num_bytes, unsigned types);

/// @brief Free one block of memory.
/// @param ptr Pointer to the start of the memory block to free.
void mem_free(void* ptr);

/// @brief Query how much memory is free.
/// @param types Memory type(s) to allocate from (must be non-zero).
/// @returns the total number of bytes that are free for allocation.
size_t mem_query_free(unsigned types);

#ifdef __cplusplus
}
#endif

#endif  // MC1_MEMORY_H_

