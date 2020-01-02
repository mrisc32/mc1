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

#ifndef LIBC_STDIO_H_
#define LIBC_STDIO_H_

#include <system/vconsole.h>

// We don't really support files (yet), so just provide the types.
typedef struct FILE_struct_t FILE;
#define stdin ((FILE*)4)
#define stdout ((FILE*)8)
#define stderr ((FILE*)12)
#define EOF -1

static inline int putc(int character, FILE* stream) {
  if (stream == stdout || stream == stderr) {
    return vcon_putc(character);
  }
  return EOF;
}

static inline int fputc(int character, FILE* stream) {
  return putc(character, stream);
}

static inline int puts(const char* str) {
  vcon_print(str);
  vcon_putc(10);
  return 1;
}

static inline int fputs(const char* str, FILE* stream) {
  if (stream == stdout || stream == stderr) {
    return puts(str);
  }
  return EOF;
}

#endif  // LIBC_STDIO_H_

