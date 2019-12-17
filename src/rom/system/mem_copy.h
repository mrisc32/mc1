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

#ifndef SYSTEM_MEM_COPY_H_
#define SYSTEM_MEM_COPY_H_

#include <system/types.h>

/// @brief Forward copy a memory block.
/// @param destination Target memory pointer (start of buffer).
/// @param source Source memory pointer (start of buffer).
/// @param num Number of bytes to copy.
/// @returns the @c destination pointer.
void* mem_copy_fwd(void* destination, const void* source, size_t num);

// TODO(m): Add mem_copy_bwd()

#endif  // SYSTEM_MEM_COPY_H_

