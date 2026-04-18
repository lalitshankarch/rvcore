#include "cpu.h"
#include "debug.h"

void Cpu::set_reg(u32 idx, u32 val) {
  regs[idx] = val;
  regs[0] = 0;
}

u32 Cpu::reg(u32 idx) { return regs[idx]; }

void Cpu::execute_instr(u8 *memory, u32 instr) {
  u32 imm_se = u32(i32(instr) >> 20);
  u32 u_imm = (instr >> 12) << 12;
  u32 funct7 = (instr >> 30) & 1;
  u32 rs2 = (instr >> 20) & 0x1f;
  u32 rs1 = (instr >> 15) & 0x1f;
  u32 funct3 = (instr >> 12) & 0x7;
  u32 rd = (instr >> 7) & 0x1f;
  u32 opcode = instr & 0x7f;

  switch (opcode) {
  case 0b0110111: // LUI
    set_reg(rd, u_imm);
    break;
  case 0b0010111: // AUIPC
    set_reg(rd, u_imm + pc - 4);
    break;
  case 0b1100111: {
    switch (funct3) {
    case JALR: {
      u32 target_addr = (imm_se + reg(rs1)) & ~u32(1);
      set_reg(rd, pc);
      pc = target_addr;
      break;
    }
    default:
      EXCEPTION("Unhandled funct3");
    }
    break;
  }
  case 0b0000011: {
    u32 offset = imm_se;
    u32 addr = reg(rs1) + offset;
    switch (funct3) {
    case LB:
      set_reg(rd, u32(i8(memory[addr])));
      break;
    case LH: {
      u32 b0 = u32(memory[addr + 0]);
      u32 b1 = u32(memory[addr + 1]) << 8;
      set_reg(rd, u32(i16(b1 | b0)));
      break;
    }
    case LW: {
      u32 b0 = u32(memory[addr + 0]);
      u32 b1 = u32(memory[addr + 1]) << 8;
      u32 b2 = u32(memory[addr + 2]) << 16;
      u32 b3 = u32(memory[addr + 3]) << 24;
      set_reg(rd, b3 | b2 | b1 | b0);
      break;
    }
    case LBU:
      set_reg(rd, memory[addr]);
      break;
    case LHU: {
      u32 b0 = u32(memory[addr + 0]);
      u32 b1 = u32(memory[addr + 1]) << 8;
      set_reg(rd, b1 | b0);
      break;
    }
    default:
      EXCEPTION("Unhandled funct3");
    }
    break;
  }
  case 0b0100011: {
    u32 off1 = (instr >> 7) & 0x1f;
    u32 off2 = instr >> 25;
    u32 imm = off2 << 5 | off1;
    u32 offset = u32((i32(imm) << 20) >> 20);
    u32 addr = reg(rs1) + offset;
    switch (funct3) {
    case SB:
      memory[addr] = u8(reg(rs2));
      break;
    case SH: {
      u32 hword = reg(rs2);
      u8 b0 = u8(hword);
      u8 b1 = u8(hword >> 8);
      memory[addr + 0] = b0;
      memory[addr + 1] = b1;
      break;
    }
    case SW: {
      u32 word = reg(rs2);
      u8 b0 = u8(word);
      u8 b1 = u8(word >> 8);
      u8 b2 = u8(word >> 16);
      u8 b3 = u8(word >> 24);
      memory[addr + 0] = b0;
      memory[addr + 1] = b1;
      memory[addr + 2] = b2;
      memory[addr + 3] = b3;
      break;
    }
    default:
      EXCEPTION("Unhandled funct3");
    }
    break;
  }
  case 0b0010011: {
    u32 shamt = rs2;
    switch (funct3) {
    case ADDI:
      set_reg(rd, reg(rs1) + imm_se);
      break;
    case SLTI:
      set_reg(rd, i32(reg(rs1)) < i32(imm_se));
      break;
    case SLLI:
      set_reg(rd, reg(rs1) << shamt);
      break;
    case SLTIU:
      set_reg(rd, reg(rs1) < imm_se);
      break;
    case XORI:
      set_reg(rd, reg(rs1) ^ imm_se);
      break;
    case SRXI:
      if (funct7)
        set_reg(rd, u32(i32(reg(rs1)) >> shamt)); // SRAI
      else
        set_reg(rd, reg(rs1) >> shamt); // SRLI
      break;
    case ORI:
      set_reg(rd, reg(rs1) | imm_se);
      break;
    case ANDI:
      set_reg(rd, reg(rs1) & imm_se);
      break;
    }
    break;
  }
  case 0b0110011: {
    u32 shamt = reg(rs2) & 0x1f;
    switch (funct3) {
    case ADD:
      if (funct7)
        set_reg(rd, reg(rs1) - reg(rs2));
      else
        set_reg(rd, reg(rs1) + reg(rs2));
      break;
    case SLT:
      set_reg(rd, i32(reg(rs1)) < i32(reg(rs2)));
      break;
    case SLL:
      set_reg(rd, reg(rs1) << shamt);
      break;
    case SLTU:
      set_reg(rd, reg(rs1) < reg(rs2));
      break;
    case XOR:
      set_reg(rd, reg(rs1) ^ reg(rs2));
      break;
    case SRX:
      if (funct7)
        set_reg(rd, u32(i32(reg(rs1)) >> shamt)); // SRA
      else
        set_reg(rd, reg(rs1) >> shamt); // SRL
      break;
    case OR:
      set_reg(rd, reg(rs1) | reg(rs2));
      break;
    case AND:
      set_reg(rd, reg(rs1) & reg(rs2));
      break;
    }
    break;
  }
  default:
    EXCEPTION("Unhandled opcode");
  }
}
