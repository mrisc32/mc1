// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
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

#ifndef MC1_DOH_H_
#define MC1_DOH_H_

/// @fn doh
/// @brief Cause abnormal program termination.
///
/// A program may call this function to indicate that it can no longer continue exection. After this
/// function has been called, the program will not resume execution.
///
/// @param message An optional text message that will be shown in an error message, or NULL if no
/// specific message is to be shown.

#ifdef __cplusplus
extern "C" [[noreturn]] void doh(const char* message);
#else
_Noreturn void doh(const char* message);
#endif  // __cplusplus

#endif  // MC1_DOH_H_
