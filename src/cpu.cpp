#include "cpu.h"
#include "debug.h"
#include <cstring>
#include <unistd.h>

Cpu::Cpu(std::vector<u8> &mem) : pc(0x218), memory(mem) {
  regs = {};
  regs[2] = MEM_SIZE;
  memory.resize(memory.size() + 4096);
  heap_ptr = u32(memory.size());
  memory.resize(MEM_SIZE);
}

void Cpu::set_reg(u32 idx, u32 val) {
  regs[idx] = val;
  regs[0] = 0;
}

u32 Cpu::reg(u32 idx) { return regs[idx]; }

u16 load16_(std::vector<u8> &memory, u32 addr) {
  u16 val;
  std::memcpy(&val, &memory[addr], sizeof(val));
  return val;
}

u32 load32_(std::vector<u8> &memory, u32 addr) {
  u32 val;
  std::memcpy(&val, &memory[addr], sizeof(val));
  return val;
}

void store16_(std::vector<u8> &memory, u32 addr, u16 hword) {
  std::memcpy(&memory[addr], &hword, sizeof(hword));
}

void store32_(std::vector<u8> &memory, u32 addr, u32 word) {
  std::memcpy(&memory[addr], &word, sizeof(word));
}

void Cpu::step() {
  u32 instr = load32_(memory, pc);
  pc += 4;
  execute_instr(instr);
}

void Cpu::execute_instr(u32 instr) {
  u32 imm_se = u32(i32(instr) >> 20);
  u32 u_imm = (instr >> 12) << 12;
  u32 funct7 = instr >> 25;
  u32 rs2 = (instr >> 20) & 0x1f;
  u32 rs1 = (instr >> 15) & 0x1f;
  u32 funct3 = (instr >> 12) & 0x7;
  u32 rd = (instr >> 7) & 0x1f;
  u32 opcode = instr & 0x7f;

  switch (opcode) {
  case OP_LUI:
    set_reg(rd, u_imm);
    break;
  case OP_AUIPC:
    set_reg(rd, u_imm + pc - 4);
    break;
  case OP_JAL: {
    u32 imm = (((instr >> 31) & 0x1) << 20) | (((instr >> 12) & 0xff) << 12) |
              (((instr >> 20) & 0x1) << 11) | (((instr >> 21) & 0x3ff) << 1);
    u32 offset = u32(i32(imm << 11) >> 11);
    set_reg(rd, pc);
    pc = (pc - 4) + offset;
    break;
  }
  case OP_JALR: {
    if (funct3 != 0)
      EXCEPTION("Unhandled funct3");
    u32 target_addr = (imm_se + reg(rs1)) & ~u32(1);
    set_reg(rd, pc);
    pc = target_addr;
    break;
  }
  case OP_BRANCH: {
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
  case OP_LOAD: {
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
  case OP_STORE: {
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
  case OP_IMM: {
    u32 shamt = rs2;
    switch (funct3) {
    case ADDI:
      set_reg(rd, reg(rs1) + imm_se);
      break;
    case SLTI:
      set_reg(rd, i32(reg(rs1)) < i32(imm_se));
      break;
    case SLLI:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) << shamt);
      else
        EXCEPTION("Unhandled funct7");
      break;
    case SLTIU:
      set_reg(rd, reg(rs1) < imm_se);
      break;
    case XORI:
      set_reg(rd, reg(rs1) ^ imm_se);
      break;
    case SRXI:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) >> shamt); // SRLI
      else if (funct7 == 0b0100000)
        set_reg(rd, u32(i32(reg(rs1)) >> shamt)); // SRAI
      else
        EXCEPTION("Unhandled funct7");
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
  case OP_REG: {
    u32 shamt = reg(rs2) & 0x1f;
    switch (funct3) {
    case ADD:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) + reg(rs2));
      else if (funct7 == 0b0100000)
        set_reg(rd, reg(rs1) - reg(rs2));
      else
        EXCEPTION("Unhandled funct7");
      break;
    case SLL:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) << shamt);
      else
        EXCEPTION("Unhandled funct7");
      break;
    case SLT:
      if (funct7 == 0b0000000)
        set_reg(rd, i32(reg(rs1)) < i32(reg(rs2)));
      else
        EXCEPTION("Unhandled funct7");
      break;
    case SLTU:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) < reg(rs2));
      else
        EXCEPTION("Unhandled funct7");
      break;
    case XOR:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) ^ reg(rs2));
      else
        EXCEPTION("Unhandled funct7");
      break;
    case SRX:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) >> shamt); // SRL
      else if (funct7 == 0b0100000)
        set_reg(rd, u32(i32(reg(rs1)) >> shamt)); // SRA
      else
        EXCEPTION("Unhandled funct7");
      break;
    case OR:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) | reg(rs2));
      else
        EXCEPTION("Unhandled funct7");
      break;
    case AND:
      if (funct7 == 0b0000000)
        set_reg(rd, reg(rs1) & reg(rs2));
      else
        EXCEPTION("Unhandled funct7");
      break;
    }
    break;
  }
  case OP_FENCE:
    WARN_PRINT("FENCE not implemented");
    break;
  case OP_SYSTEM: {
    switch (imm_se) {
    case 0: { // ECALL
      switch (reg(17)) {
      case 0: {
        i32 increment = i32(reg(10));
        u32 old_ptr = heap_ptr;
        heap_ptr = u32(i32(heap_ptr) + increment);
        set_reg(10, old_ptr);
        break;
      }
      case 4: {
        int fd = int(reg(10));
        u32 start = reg(11);
        u32 nbytes = reg(12);
        ssize_t bytes_written = write(fd, &memory[start], nbytes);
        set_reg(10, u32(bytes_written));
        break;
      }
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
