#pragma once

#include "types.h"

enum ElfHeader32 {
  MAG0,
  MAG1,
  MAG2,
  MAG3,
  CLASS,
  DATA,
  VERSION,
  OSABI,
  ABIVERSION,
  PAD,
  TYPE = 0x10,
  MACHINE = 0x12,
  ELFVERSION = 0x14,
  ENTRY = 0x18,
  PHOFF = 0x1C,
  SHOFF = 0x20,
  FLAGS = 0x24,
  HSIZE = 0x28,
  PHENTSIZE = 0x2A,
  PHNUM = 0x2C,
  SHENTSIZE = 0x2E,
  SHNUM = 0x30,
  SHSTRNDX = 0x32,
};

enum PHeader32 {
  PH_TYPE,
  PH_OFFSET = 0x04,
  PH_VADDR = 0x08,
  PH_PADDR = 0x0C,
  PH_FILESZ = 0x10,
  PH_MEMSZ = 0x14,
  PH_FLAGS = 0x18,
  PH_ALIGN = 0x1C,
};

struct ExecSeg {
  u32 entry;
  u32 offset;
  u32 nbytes;
  u32 nmembytes;

  static ExecSeg get_info(const char* path);

private:
    static bool validate_elf_header(u8 elf_header[0x18]);
};
