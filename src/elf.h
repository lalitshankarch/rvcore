#pragma once

#include "types.h"
#include <fstream>
#include <vector>

struct ElfHeader32 {
  u8 mag0, mag1, mag2, mag3, cls, data, version, osabi, abiversion;
  u8 padding[7];
  u16 type, machine;
  u32 elfversion, entry, phoff, shoff, flags;
  u16 ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};

struct ProgHeader32 {
  u32 type, offset, vaddr, paddr, filesz, memsz, flags, align;
};

struct ExecSeg {
  u32 entry, nmembytes;

  static ExecSeg read_into(std::vector<u8> &memory, const char *path);

private:
  u32 offset, nbytes;

  static bool validate_elf_header(const ElfHeader32 *elf_header);
  static ExecSeg get_info(std::ifstream &file);
};
