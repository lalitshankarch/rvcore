#include "cpu.h"
#include "debug.h"
#include "elf.h"
#include <exception>
#include <fstream>
#include <vector>

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("Usage: %s <bin>\n", argv[0]);
    return 1;
  }

  try {
    ExecSeg exec_seg = ExecSeg::get_info(argv[1]);
    std::vector<u8> memory(exec_seg.nmembytes);
    std::ifstream file(argv[1], std::ios::binary);

    if (!file) {
      std::perror("ifstream");
      return 1;
    }

    file.seekg(exec_seg.offset);
    file.read(reinterpret_cast<char *>(memory.data()), exec_seg.nbytes);

    Cpu cpu(memory, exec_seg.entry, exec_seg.nmembytes);

    while (true) {
      cpu.step();
    }
  } catch (std::exception &ex) {
    std::cerr << std::endl << RED << ex.what() << RESET << std::endl;
  }

  return 0;
}