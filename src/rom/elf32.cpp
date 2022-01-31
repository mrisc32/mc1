//--------------------------------------------------------------------------------------------------
// Copyright (c) 2022 Marcus Geelnard
// Copyright (c) 2020 Bruno Levy
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
// This a fork of lite_elf.h by Bruno Levy. Original description:
//   A minimalistic ELF loader. Probably many things are missing.
//   Disclaimer: I do not understand everything here !
//   Bruno Levy, 12/2020
//--------------------------------------------------------------------------------------------------

#include "elf32.hpp"

#include <mc1/mfat_mc1.h>

#include <cstring>

namespace elf32 {
namespace {
// Elf32_Ehdr.e_machine
#define EM_MRISC32 0xc001

// Elf32_Shdr.sh_type
#define SHT_PROGBITS 1
#define SHT_NOBITS 8
#define SHT_INIT_ARRAY 14
#define SHT_FINI_ARRAY 15

// Elf32_Shdr.sh_flags
#define SHF_ALLOC 0x2

// ELF header
struct Elf32_Ehdr {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

// Section header
struct Elf32_Shdr {
  uint32_t sh_name;
  uint32_t sh_type;
  uint32_t sh_flags;
  uint32_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
};

class elf_file_t {
public:
  elf_file_t(const char* file_name) {
    m_fd = mfat_open(file_name, MFAT_O_RDONLY);
    m_is_open = (m_fd != -1);
  }

  ~elf_file_t() {
    if (m_fd != -1) {
      mfat_close(m_fd);
    }
  }

  bool is_open() const {
    return m_is_open;
  }

  bool read(uint8_t* ptr, uint32_t bytes) {
    while (bytes > 0U) {
      auto bytes_read = mfat_read(m_fd, ptr, bytes);
      if (bytes_read == 0) {
        // EOF.
        break;
      } else if (bytes_read == -1) {
        // Error.
        return false;
      }
      ptr += bytes_read;
      bytes -= static_cast<uint32_t>(bytes_read);
    }
    return bytes == 0U;
  }

  bool seek(uint32_t offset) {
    return mfat_lseek(m_fd, offset, MFAT_SEEK_SET) != -1;
  }

private:
  int m_fd;
  bool m_is_open;
};

}  // namespace

bool load(const char* file_name, uint32_t& entry_address) {
  elf_file_t f(file_name);
  if (!f.is_open()) {
    return false;
  }

  // Read elf header.
  Elf32_Ehdr elf_header;
  if (!f.read(reinterpret_cast<uint8_t*>(&elf_header), sizeof(elf_header))) {
    return false;
  }

  // Sanity check.
  if ((elf_header.e_ehsize != sizeof(elf_header)) ||
      (elf_header.e_shentsize != sizeof(Elf32_Shdr)) || (elf_header.e_machine != EM_MRISC32)) {
    return false;
  }

  // Get the entry address.
  entry_address = elf_header.e_entry;

  for (unsigned i = 0U; i < elf_header.e_shnum; ++i) {
    // Read the section header.
    Elf32_Shdr sec_header;
    if (!f.seek(elf_header.e_shoff + i * sizeof(Elf32_Shdr))) {
      return false;
    }
    if (!f.read(reinterpret_cast<uint8_t*>(&sec_header), sizeof(sec_header))) {
      return false;
    }

    // The sections we are interested in are the ALLOC sections.
    if ((sec_header.sh_flags & SHF_ALLOC) == 0U) {
      continue;
    }

    // PROGBIT, INI_ARRAY and FINI_ARRAY need to be loaded.
    if (sec_header.sh_type == SHT_PROGBITS || sec_header.sh_type == SHT_INIT_ARRAY ||
        sec_header.sh_type == SHT_FINI_ARRAY) {
      if (!f.seek(sec_header.sh_offset)) {
        return false;
      }
      if (!f.read(reinterpret_cast<uint8_t*>(sec_header.sh_addr), sec_header.sh_size)) {
        return false;
      }
    }

    // NOBITS need to be cleared.
    if (sec_header.sh_type == SHT_NOBITS) {
      auto* ptr = reinterpret_cast<uint8_t*>(sec_header.sh_addr);
      std::memset(ptr, 0, sec_header.sh_size);
    }
  }

  return true;
}

}  // namespace elf32
