#include "elf.h"
#include "debug.h"
#include <cstring>
#include <fstream>

bool ExecSeg::validate_elf_header(const ElfHeader32 *elf_header) {
  ElfHeader32 valid_header{};
  valid_header.mag0 = 0x7f;
  valid_header.mag1 = 'E';
  valid_header.mag2 = 'L';
  valid_header.mag3 = 'F';
  valid_header.cls = 0x01;
  valid_header.data = 0x01;
  valid_header.version = 0x01;
  valid_header.osabi = 0x00;
  valid_header.abiversion = 0x00;
  valid_header.type = 0x02;
  valid_header.machine = 0xf3;
  valid_header.elfversion = 0x01;

  return std::memcmp(elf_header, &valid_header, 0x18) == 0;
}

ExecSeg ExecSeg::get_info(const char *path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    EXCEPTION("Invalid file path");
  }

  ElfHeader32 elf_header{};
  file.read(reinterpret_cast<char *>(&elf_header), sizeof(elf_header));

  if (!validate_elf_header(&elf_header)) {
    EXCEPTION("Invalid executable format");
  }

  ProgHeader32 prog_header{};
  for (u32 i = 0; i < elf_header.phnum; i++) {
    file.seekg(elf_header.phoff + (u32(sizeof(ProgHeader32)) * i));
    file.read(reinterpret_cast<char *>(&prog_header), sizeof(prog_header));

    if (prog_header.type == 0x1) {
      ExecSeg exec_seg{};
      exec_seg.entry = elf_header.entry - prog_header.vaddr;
      exec_seg.offset = prog_header.offset;
      exec_seg.nbytes = prog_header.filesz;
      exec_seg.nmembytes = prog_header.memsz;

      return exec_seg;
    }
  }

  EXCEPTION("No executable segment found");
}