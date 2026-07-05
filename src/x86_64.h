#include "types.h"
#include <algorithm>

#define B0(x) u8((x) & 0xff)
#define B1(x) u8((x >> 8) & 0xff)
#define B2(x) u8((x >> 16) & 0xff)
#define B3(x) u8((x >> 24) & 0xff)
#define B4(x) u8((x >> 32) & 0xff)
#define B5(x) u8((x >> 40) & 0xff)
#define B6(x) u8((x >> 48) & 0xff)
#define B7(x) u8((x >> 56) & 0xff)

enum Reg8 { AL = 0, AH = 4, CL = 1, CH = 5, DL = 2, DH = 6 };
enum Reg32 { EAX = 0, ECX = 1, EDX = 2 };
enum Reg64 { RAX = 0, RCX = 1, RDX = 2 };

class x86_64 {
public:
  inline static void *ptr = nullptr;

  static void emit(std::initializer_list<u8> bytes) {
    std::copy(bytes.begin(), bytes.end(), static_cast<u8 *>(ptr));
    ptr = static_cast<u8 *>(ptr) + bytes.size();
  }

  // Move R64 to R64
  static void mov(Reg64 reg1, Reg64 reg2) {
    emit({0x48, u8(0xb8 + (u8(reg2 << 3) | reg1))});
  }

  // Move QWORD to R64
  static void mov(Reg64 reg, u64 imm) {
    emit({0x48, u8(0xb8 + reg), B0(imm), B1(imm), B2(imm), B3(imm), B4(imm),
          B5(imm), B6(imm), B7(imm)});
  }

  // Move DWORD to R32
  static void mov(Reg32 reg, u32 imm) {
    emit({u8(0xb8 + reg), B0(imm), B1(imm), B2(imm), B3(imm)});
  }

  // Move DWORD from memory [R64 + offset] to R32
  static void mov(Reg32 reg32, Reg64 reg64, i32 offset) {
    emit({0x8b, u8(0x80 + ((reg32 << 3) | reg64)), B0(offset), B1(offset),
          B2(offset), B3(offset)});
  }

  // Move DWORD from R32 to memory [R64 + offset]
  static void mov(Reg64 reg64, i32 offset, Reg32 reg32) {
    emit({0x89, u8(0x80 + ((reg32 << 3) | reg64)), B0(offset), B1(offset),
          B2(offset), B3(offset)});
  }

  // Move BYTE from memory [R64 + offset] to R8
  static void mov_r8_mem(Reg8 reg8, Reg64 reg64, i32 offset) {
    emit({0x8a, u8(0x80 + ((reg8 << 3) | reg64)), B0(offset), B1(offset),
          B2(offset), B3(offset)});
  }

  // Move BYTE from R8 to memory [R64 + offset]
  static void mov_mem_r8(Reg64 reg64, i32 offset, Reg8 reg8) {
    emit({0x88, u8(0x80 + ((reg8 << 3) | reg64)), B0(offset), B1(offset),
          B2(offset), B3(offset)});
  }

  // Move DWORD to memory [R64 + offset]
  static void mov(Reg64 reg64, i32 offset, u32 imm) {
    emit({0xc7, u8(0x80 + reg64), B0(offset), B1(offset), B2(offset),
          B3(offset), B0(imm), B1(imm), B2(imm), B3(imm)});
  }

  // Add DWORD to R32
  static void add(Reg32 reg, u32 imm) {
    emit({0x81, u8(0xc0 + reg), B0(imm), B1(imm), B2(imm), B3(imm)});
  }

  // Compare DWORD with R32
  static void cmp(Reg32 reg, u32 imm) {
    emit({0x81, u8(0xf8 + reg), B0(imm), B1(imm), B2(imm), B3(imm)});
  }

  // Compare R32 with R32
  static void cmp(Reg32 reg1, Reg32 reg2) {
    emit({0x39, u8(0xc0 + (u8(reg2 << 3) | reg1))});
  }

  // Set byte if less (SF<>OF)
  static void setl(Reg8 reg) { emit({0x0f, 0x9c, u8(0xc0 + u8(reg))}); }

  // Set byte if below (CF=1)
  static void setb(Reg8 reg) { emit({0x0f, 0x92, u8(0xc0 + u8(reg))}); }

  // Move with zero-extend
  static void movzx(Reg32 reg32, Reg8 reg8) {
    emit({0x0f, 0xb6, u8(0xc0 + (u8(reg32 << 3) | reg8))});
  }

  // Multiply R32 by 2, IMM8 times
  static void sal(Reg32 reg, u8 imm) { emit({0xc1, u8(0xf0 + u8(reg)), imm}); }

  // Multiply R32 by 2, CL times
  static void sal(Reg32 reg) { emit({0xd3, u8(0xe0 + u8(reg))}); }

  // Signed divide R32 by 2, IMM8 times
  static void sar(Reg32 reg, u8 imm) { emit({0xc1, u8(0xf8 + u8(reg)), imm}); }

  // Signed divide R32 by 2, CL times
  static void sar(Reg32 reg) { emit({0xd3, u8(0xf8 + u8(reg))}); }

  // Unigned divide R32 by 2, IMM8 times
  static void shr(Reg32 reg, u8 imm) { emit({0xc1, u8(0xe8 + u8(reg)), imm}); }

  // Unsigned divide R32 by 2, CL times
  static void shr(Reg32 reg) { emit({0xd3, u8(0xe8 + u8(reg))}); }

  // XOR DWORD with R32
  static void xori(Reg32 reg, u32 imm) {
    emit({0x81, u8(0xf0 + reg), B0(imm), B1(imm), B2(imm), B3(imm)});
  }

  // XOR R32 with R32
  static void xorr(Reg32 reg1, Reg32 reg2) {
    emit({0x31, u8(0xc0 + (u8(reg2 << 3) | reg1))});
  }

  // OR DWORD with R32
  static void ori(Reg32 reg, u32 imm) {
    emit({0x81, u8(0xc8 + reg), B0(imm), B1(imm), B2(imm), B3(imm)});
  }

  // OR R32 with R32
  static void orr(Reg32 reg1, Reg32 reg2) {
    emit({0x09, u8(0xc0 + (u8(reg2 << 3) | reg1))});
  }

  // AND DWORD with R32
  static void andi(Reg32 reg, u32 imm) {
    emit({0x81, u8(0xe0 + reg), B0(imm), B1(imm), B2(imm), B3(imm)});
  }

  // AND R32 with R32
  static void andr(Reg32 reg1, Reg32 reg2) {
    emit({0x21, u8(0xc0 + (u8(reg2 << 3) | reg1))});
  }

  // Add R32 to R32
  static void addr(Reg32 reg1, Reg32 reg2) {
    emit({0x01, u8(0xc0 + (u8(reg2 << 3) | reg1))});
  }

  // Add R32 from R32
  static void subr(Reg32 reg1, Reg32 reg2) {
    emit({0x29, u8(0xc0 + (u8(reg2 << 3) | reg1))});
  }

  // Push QWORD onto the stack
  static void push(Reg64 reg) { emit({u8(0x50 + u8(reg))}); }

  // Push QWORD from the stack
  static void pop(Reg64 reg) { emit({u8(0x58 + u8(reg))}); }

  // Call function at address stored in R64
  static void call(Reg64 reg) { emit({0xff, u8(0xd0 + reg)}); }

  static void ret() { emit({0xC3}); }
};
