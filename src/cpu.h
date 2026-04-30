#pragma once
#include "types.h"
#include <array>
#include <vector>

const u32 MEM_SIZE = 1024 * 1024 * 8;

class Cpu {
  std::array<u32, 32> regs;
  u32 pc, heap_ptr;
  std::vector<u8>& memory;

  enum Opcode : u32 {
    OP_LUI = 0b0110111,
    OP_AUIPC = 0b0010111,
    OP_JAL = 0b1101111,
    OP_JALR = 0b1100111,
    OP_BRANCH = 0b1100011,
    OP_LOAD = 0b0000011,
    OP_STORE = 0b0100011,
    OP_IMM = 0b0010011,
    OP_REG = 0b0110011,
    OP_FENCE = 0b0001111,
    OP_SYSTEM = 0b1110011,
  };

  enum Funct3 : u32 {
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
  void execute_instr(u32 instr);

public:
  Cpu(std::vector<u8>& mem, u32 pc_start, u32 heap_start);
  void step();
};