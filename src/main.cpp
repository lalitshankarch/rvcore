#include "types.h"
#include <iostream>

constexpr u8 test() { return u8(0xff); }

int main() {
  std::cout << (i32(0x80000000) >> 20) << std::endl;
  std::cout << i32(i16(i8(test()))) << std::endl;
  std::cout << i32(u32(i32(i8(test())))) << std::endl;
  std::cout << i32(u32(i8(test()))) << std::endl;
}