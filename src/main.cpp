#include "cpu.h"
#include "debug.h"
#include <exception>
#include <fstream>
#include <vector>

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("Usage: %s <bin>\n", argv[0]);
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::perror("ifstream");
    return 1;
  }

  std::vector<u8> memory((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  memory.resize(1024 * 1024); // 1 MiB

  Cpu cpu(memory.data());

  try {
    while (true) {
      cpu.step();
    }
  } catch (std::exception &ex) {
    std::cerr << std::endl << RED << ex.what() << RESET << std::endl;
  }

  return 0;
}