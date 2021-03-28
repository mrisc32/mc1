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

#ifndef LZG_MC1_H_
#define LZG_MC1_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t LZG_Decode(const uint8_t* in,
                    const uint32_t insize,
                    uint8_t* out,
                    const uint32_t outsize);

#ifdef __cplusplus
}
#endif

#endif  // LZG_MC1_H_

