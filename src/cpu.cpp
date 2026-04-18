#include "cpu.h"
#include "debug.h"

void Cpu::set_reg(u32 idx, u32 val) {
  regs[idx] = val;
  regs[0] = 0;
}

u32 Cpu::reg(u32 idx) { return regs[idx]; }

u16 load16_(const u8 *memory, u32 addr) {
  u16 b0 = u16(memory[addr + 0]);
  u16 b1 = u16(memory[addr + 1] << 8);
  return b1 | b0;
}

u32 load32_(const u8 *memory, u32 addr) {
  u32 b0 = u32(memory[addr + 0]);
  u32 b1 = u32(memory[addr + 1]) << 8;
  u32 b2 = u32(memory[addr + 2]) << 16;
  u32 b3 = u32(memory[addr + 3]) << 24;
  return b3 | b2 | b1 | b0;
}

void store16_(u8 *memory, u32 addr, u16 hword) {
  memory[addr + 0] = u8(hword);
  memory[addr + 1] = u8(hword >> 8);
}

void store32_(u8 *memory, u32 addr, u32 word) {
  memory[addr + 0] = u8(word);
  memory[addr + 1] = u8(word >> 8);
  memory[addr + 2] = u8(word >> 16);
  memory[addr + 3] = u8(word >> 24);
}

void Cpu::step(u8 *memory) {
  u32 instr = load32_(memory, pc);
  pc += 4;
  execute_instr(memory, instr);
}

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
  case 0b1101111: { // JAL
    u32 imm = (((instr >> 31) & 0x1) << 20) | (((instr >> 12) & 0xff) << 12) |
              (((instr >> 20) & 0x1) << 11) | (((instr >> 21) & 0x3ff) << 1);
    u32 offset = u32(i32(imm << 11) >> 11);
    set_reg(rd, pc);
    pc = (pc - 4) + offset;
    break;
  }
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
  case 0b1100011: {
    u32 imm = (((instr >> 31) & 0x1) << 12) | (((instr >> 7) & 0x1) << 11) |
              (((instr >> 25) & 0x3f) << 5) | (((instr >> 8) & 0xf) << 1);
    u32 offset = u32(i32(imm << 19) >> 19);
    u32 addr = (pc - 4) + offset;
    switch (funct3) {
    case BEQ:
      if (reg(rs1) == reg(rs2))
        pc = addr;
      break;
    case BNE:
      if (reg(rs1) != reg(rs2))
        pc = addr;
      break;
    case BLT:
      if (i32(reg(rs1)) < i32(reg(rs2)))
        pc = addr;
      break;
    case BGE:
      if (i32(reg(rs1)) >= i32(reg(rs2)))
        pc = addr;
      break;
    case BLTU:
      if (reg(rs1) < reg(rs2))
        pc = addr;
      break;
    case BGEU:
      if (reg(rs1) >= reg(rs2))
        pc = addr;
      break;
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
    case LH:
      set_reg(rd, u32(i16(load16_(memory, addr))));
      break;
    case LW:
      set_reg(rd, load32_(memory, addr));
      break;
    case LBU:
      set_reg(rd, memory[addr]);
      break;
    case LHU:
      set_reg(rd, load16_(memory, addr));
      break;
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
    case SH:
      store16_(memory, addr, u16(reg(rs2)));
      break;
    case SW:
      store32_(memory, addr, reg(rs2));
      break;
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
  case 0b0001111: // FENCE
    WARN_PRINT("FENCE not implemented");
    break;
  case 0b1110011: {
    switch (imm_se) {
    case 0: { // ECALL
      switch (reg(17)) {
      case 4:
        printf("%s", &memory[reg(10)]);
        break;
      case 10:
        EXCEPTION("EXIT");
        break;
      default:
        EXCEPTION("Unknown syscall");
      }
      break;
    }
    case 1:
      EXCEPTION("EBREAK");
      break;
    default:
      EXCEPTION("Unhandled SYSTEM");
    }
    break;
  }
  default:
    EXCEPTION("Unhandled opcode");
  }
}
