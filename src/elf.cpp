#include "elf.h"
#include "debug.h"
#include <cstdio>
#include <cstring>

bool ExecSeg::validate_elf_header(u8 elf_header[0x18]) {
  u8 valid_header[0x18] = {};
  valid_header[MAG0] = 0x7f;
  valid_header[MAG1] = 'E';
  valid_header[MAG2] = 'L';
  valid_header[MAG3] = 'F';
  valid_header[CLASS] = 0x1;
  valid_header[DATA] = 0x1;
  valid_header[VERSION] = 0x1;
  valid_header[OSABI] = 0x0;
  valid_header[ABIVERSION] = 0x0;
  valid_header[TYPE] = 0x2;
  valid_header[MACHINE] = 0xf3;
  valid_header[ELFVERSION] = 0x1;

  return memcmp(elf_header, valid_header, 0x18) == 0;
}

static u16 read16_(const u8 *memory, u32 addr) {
  u16 val;
  std::memcpy(&val, &memory[addr], sizeof(val));
  return val;
}

static u32 read32_(const u8 *memory, u32 addr) {
  u32 val;
  std::memcpy(&val, &memory[addr], sizeof(val));
  return val;
}

ExecSeg ExecSeg::get_info(const char *path) {
  u8 elf_header[64];
  FILE *fptr = fopen(path, "rb");

  if (!fptr) {
    EXCEPTION("Invalid file path");
  }

  fread(elf_header, sizeof(elf_header), 1, fptr);

  if (!validate_elf_header(elf_header)) {
    fclose(fptr);
    EXCEPTION("Invalid executable format");
  }

  u8 p_header[0x20];
  ExecSeg exec_seg;
  u16 ph_num = read16_(elf_header, PHNUM);
  u32 ph_off = read32_(elf_header, PHOFF);

  for (u32 i = 0; i < ph_num; i++) {
    fseek(fptr, ph_off + (0x20 * i), SEEK_SET);
    fread(p_header, sizeof(p_header), 1, fptr);

    if (read32_(p_header, PH_TYPE) == 0x1) {
      exec_seg.entry = read32_(elf_header, ENTRY) - read32_(p_header, PH_VADDR);
      exec_seg.offset = read32_(p_header, PH_OFFSET);
      exec_seg.nbytes = read32_(p_header, PH_FILESZ);
      exec_seg.nmembytes = read32_(p_header, PH_MEMSZ);

      return exec_seg;
    }
  }

  fclose(fptr);

  EXCEPTION("No executable segment found");
}