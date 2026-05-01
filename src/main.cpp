#include "constants.h"
#include "cpu.h"
#include "debug.h"
#include "elf.h"
#include <exception>
#include <vector>

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("Usage: %s <bin>\n", argv[0]);
    return 1;
  }

  try {
    std::vector<u8> memory;
    ExecSeg exec_seg = ExecSeg::read_into(memory, argv[1]);

    memory.resize(MEM_SIZE);

    Cpu cpu(memory, exec_seg.entry, exec_seg.nmembytes);

    while (true) {
      cpu.step();
    }
  } catch (std::exception &ex) {
    std::cerr << std::endl << RED << ex.what() << RESET << std::endl;
  }

  return 0;
}