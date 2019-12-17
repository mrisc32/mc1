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

#ifndef SYSTEM_STDC_H_
#define SYSTEM_STDC_H_

#include <system/types.h>

// stdlib.h
void* malloc(size_t size);
void* calloc(size_t size);
void free(void* ptr);

// string.h
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* destination, const void* source, size_t num);
// TODO(m): Add memmove.

#endif  // SYSTEM_STDC_H_

