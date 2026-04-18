#pragma once
#include "types.h"
#include <array>

class Cpu {
  std::array<u32, 32> regs;
  u32 pc;

  enum Funct3 : u32 {
    JALR = 0b000,
    BEQ = 0b000,
    BNE = 0b001,
    BLT = 0b100,
    BGE = 0b101,
    BLTU = 0b110,
    BGEU = 0b111,
    LB = 0b000,
    LH = 0b001,
    LW = 0b010,
    LBU = 0b100,
    LHU = 0b101,
    SB = 0b000,
    SH = 0b001,
    SW = 0b010,
    ADDI = 0b000,
    SLLI = 0b001,
    SLTI = 0b010,
    SLTIU = 0b011,
    XORI = 0b100,
    SRXI = 0b101,
    ORI = 0b110,
    ANDI = 0b111,
    ADD = 0b000,
    SLL = 0b001,
    SLT = 0b010,
    SLTU = 0b011,
    XOR = 0b100,
    SRX = 0b101,
    OR = 0b110,
    AND = 0b111,
  };

  void set_reg(u32 idx, u32 val);
  u32 reg(u32 idx);

public:
  Cpu() : regs({}), pc(0) {};
  void execute_instr(u8 *memory, u32 instr);
};