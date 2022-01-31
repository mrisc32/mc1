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

#ifndef MC1_ELF32_H_
#define MC1_ELF32_H_

#include <cstdint>

namespace elf32 {

/// @brief Load an ELF32 executable into memory.
/// @param file_name The path to the executable file.
/// @param[out] entry_address The start address of the program.
/// @returns true on success, or false on failure.
bool load(const char* file_name, uint32_t& entry_address);

}  // namespace elf32

#endif  // MC1_ELF32_H_
