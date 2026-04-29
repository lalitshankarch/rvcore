#pragma once

#include "types.h"

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
  u32 entry;
  u32 offset;
  u32 nbytes;
  u32 nmembytes;

  static ExecSeg get_info(const char *path);

private:
  static bool validate_elf_header(const ElfHeader32 *elf_header);
};
