#include "cpu.h"
#include "constants.h"
#include "debug.h"
#include <SDL3/SDL_timer.h>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#endif

#define NEXT                                                                   \
  instr = load32_(memory, pc);                                                 \
  pc += 4;                                                                     \
  goto *targets[instr & 0x7f];

#define FUNCT3(instr) ((instr >> 12) & 0x7)

#define BRANCH_COMMON                                                          \
  u32 rs1 = (instr >> 15) & 0x1f;                                              \
  u32 rs2 = (instr >> 20) & 0x1f;                                              \
  u32 imm = (((instr >> 31) & 0x1) << 12) | (((instr >> 7) & 0x1) << 11) |     \
            (((instr >> 25) & 0x3f) << 5) | (((instr >> 8) & 0xf) << 1);       \
  u32 offset = u32(i32(imm << 19) >> 19);                                      \
  u32 addr = (pc - 4) + offset;

#define LOAD_COMMON                                                            \
  u32 imm_se = u32(i32(instr) >> 20);                                          \
  u32 rs1 = (instr >> 15) & 0x1f;                                              \
  u32 rd = (instr >> 7) & 0x1f;                                                \
  u32 offset = imm_se;                                                         \
  u32 addr = reg(rs1) + offset;

#define STORE_COMMON                                                           \
  u32 rs1 = (instr >> 15) & 0x1f;                                              \
  u32 rs2 = (instr >> 20) & 0x1f;                                              \
  u32 off1 = (instr >> 7) & 0x1f;                                              \
  u32 off2 = instr >> 25;                                                      \
  u32 imm = off2 << 5 | off1;                                                  \
  u32 offset = u32((i32(imm) << 20) >> 20);                                    \
  u32 addr = reg(rs1) + offset;

#define IMM_COMMON                                                             \
  u32 rs1 = (instr >> 15) & 0x1f;                                              \
  u32 rd = (instr >> 7) & 0x1f;

#define REG_COMMON                                                             \
  u32 rs1 = (instr >> 15) & 0x1f;                                              \
  u32 rs2 = (instr >> 20) & 0x1f;                                              \
  u32 funct7 = instr >> 25;                                                    \
  u32 rd = (instr >> 7) & 0x1f;

Cpu::Cpu(std::vector<u8> &mem, u32 pc_start, u32 heap_start)
    : pc(pc_start), heap_ptr(heap_start), memory(mem), should_render(false) {
  regs = {};
  regs[2] = STACK_START; // Set stack pointer
}

void Cpu::set_reg(u32 idx, u32 val) {
  if (idx != 0)
    regs[idx] = val;
}

u32 Cpu::reg(u32 idx) { return regs[idx]; }

static u16 load16_(std::vector<u8> &memory, u32 addr) {
  u16 val;
  std::memcpy(&val, &memory[addr], sizeof(val));
  return val;
}

static u32 load32_(std::vector<u8> &memory, u32 addr) {
  u32 val;
  std::memcpy(&val, &memory[addr], sizeof(val));
  return val;
}

static void store16_(std::vector<u8> &memory, u32 addr, u16 hword) {
  std::memcpy(&memory[addr], &hword, sizeof(hword));
}

static void store32_(std::vector<u8> &memory, u32 addr, u32 word) {
  std::memcpy(&memory[addr], &word, sizeof(word));
}

static const char *host_char_ptr(std::vector<u8> &memory, u32 addr) {
  return reinterpret_cast<const char *>(&memory[addr]);
}

static int translate_open_flags(int flag) {
  int h = 0;

  switch (flag & 0x3) {
  case 0:
    h |= O_RDONLY;
    break;
  case 1:
    h |= O_WRONLY;
    break;
  case 2:
    h |= O_RDWR;
    break;
  }

  if (flag & 0x0200)
    h |= O_CREAT;
  if (flag & 0x0400)
    h |= O_TRUNC;
  if (flag & 0x0800)
    h |= O_EXCL;
  if (flag & 0x8)
    h |= O_APPEND;

  return h;
}

void Cpu::execute() {
  static void *targets[128]{};
  static bool initialized = false;

  if (!initialized) {
    targets[OP_LUI] = &&op_lui;
    targets[OP_AUIPC] = &&op_auipc;
    targets[OP_JAL] = &&op_jal;
    targets[OP_JALR] = &&op_jalr;
    targets[OP_BRANCH] = &&op_branch;
    targets[OP_LOAD] = &&op_load;
    targets[OP_STORE] = &&op_store;
    targets[OP_IMM] = &&op_imm;
    targets[OP_REG] = &&op_reg;
    targets[OP_FENCE] = &&op_fence;
    targets[OP_SYSTEM] = &&op_system;

    initialized = true;
  }

  NEXT;

op_lui: {
  u32 rd = (instr >> 7) & 0x1f;
  u32 u_imm = (instr >> 12) << 12;
  set_reg(rd, u_imm);
  NEXT;
}

op_auipc: {
  u32 rd = (instr >> 7) & 0x1f;
  u32 u_imm = (instr >> 12) << 12;
  set_reg(rd, u_imm + pc - 4);
  NEXT;
}

op_jal: {
  u32 rd = (instr >> 7) & 0x1f;
  u32 imm = (((instr >> 31) & 0x1) << 20) | (((instr >> 12) & 0xff) << 12) |
            (((instr >> 20) & 0x1) << 11) | (((instr >> 21) & 0x3ff) << 1);
  u32 offset = u32(i32(imm << 11) >> 11);
  set_reg(rd, pc);
  pc = (pc - 4) + offset;
  NEXT;
}

op_jalr: {
  u32 imm_se = u32(i32(instr) >> 20);
  u32 rs1 = (instr >> 15) & 0x1f;
  u32 rd = (instr >> 7) & 0x1f;
  if (FUNCT3(instr) != 0)
    EXCEPTION("Unhandled funct3");
  u32 target_addr = (imm_se + reg(rs1)) & ~u32(1);
  set_reg(rd, pc);
  pc = target_addr;
  NEXT;
}

op_branch: {
  static void *branch_targets[8] = {
      &&beq, &&bne, nullptr, nullptr, &&blt, &&bge, &&bltu, &&bgeu,
  };

  goto *branch_targets[FUNCT3(instr)];

beq: {
  BRANCH_COMMON;
  if (reg(rs1) == reg(rs2))
    pc = addr;
  NEXT;
}

bne: {
  BRANCH_COMMON;
  if (reg(rs1) != reg(rs2))
    pc = addr;
  NEXT;
}

blt: {
  BRANCH_COMMON;
  if (i32(reg(rs1)) < i32(reg(rs2)))
    pc = addr;
  NEXT;
}

bge: {
  BRANCH_COMMON;
  if (i32(reg(rs1)) >= i32(reg(rs2)))
    pc = addr;
  NEXT;
}

bltu: {
  BRANCH_COMMON;
  if (reg(rs1) < reg(rs2))
    pc = addr;
  NEXT;
}

bgeu: {
  BRANCH_COMMON;
  if (reg(rs1) >= reg(rs2))
    pc = addr;
  NEXT;
}
}

op_load: {
  static void *load_targets[8] = {
      &&lb, &&lh, &&lw, nullptr, &&lbu, &&lhu,
  };

  goto *load_targets[FUNCT3(instr)];

lb: {
  LOAD_COMMON;
  set_reg(rd, u32(i8(memory[addr])));
  NEXT;
}

lh: {
  LOAD_COMMON;
  set_reg(rd, u32(i16(load16_(memory, addr))));
  NEXT;
}

lw: {
  LOAD_COMMON;
  set_reg(rd, load32_(memory, addr));
  NEXT;
}

lbu: {
  LOAD_COMMON;
  set_reg(rd, memory[addr]);
  NEXT;
}

lhu: {
  LOAD_COMMON;
  set_reg(rd, load16_(memory, addr));
  NEXT;
}
}

op_store: {
  static void *store_targets[8] = {
      &&sb,
      &&sh,
      &&sw,
  };

  goto *store_targets[FUNCT3(instr)];

sb: {
  STORE_COMMON;
  memory[addr] = u8(reg(rs2));
  NEXT;
}

sh: {
  STORE_COMMON;
  store16_(memory, addr, u16(reg(rs2)));
  NEXT;
}

sw: {
  STORE_COMMON;
  store32_(memory, addr, reg(rs2));
  NEXT;
}
}

op_imm: {
  static void *imm_targets[8] = {
      &&addi, &&slli, &&slti, &&sltiu, &&xori, &&srxi, &&ori, &&andi,
  };

  goto *imm_targets[FUNCT3(instr)];

addi: {
  IMM_COMMON;
  u32 imm_se = u32(i32(instr) >> 20);
  set_reg(rd, reg(rs1) + imm_se);
  NEXT;
}

slli: {
  IMM_COMMON;
  u32 funct7 = instr >> 25;
  u32 rs2 = (instr >> 20) & 0x1f;
  u32 shamt = rs2;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) << shamt);
  else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

slti: {
  IMM_COMMON;
  u32 imm_se = u32(i32(instr) >> 20);
  set_reg(rd, i32(reg(rs1)) < i32(imm_se));
  NEXT;
}

sltiu: {
  IMM_COMMON;
  u32 imm_se = u32(i32(instr) >> 20);
  set_reg(rd, reg(rs1) < imm_se);
  NEXT;
}

xori: {
  IMM_COMMON;
  u32 imm_se = u32(i32(instr) >> 20);
  set_reg(rd, reg(rs1) ^ imm_se);
  NEXT;
}

srxi: {
  IMM_COMMON;
  u32 funct7 = instr >> 25;
  u32 rs2 = (instr >> 20) & 0x1f;
  u32 shamt = rs2;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) >> shamt); // SRLI
  else if (funct7 == 0b0100000)
    set_reg(rd, u32(i32(reg(rs1)) >> shamt)); // SRAI
  else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

ori: {
  IMM_COMMON;
  u32 imm_se = u32(i32(instr) >> 20);
  set_reg(rd, reg(rs1) | imm_se);
  NEXT;
}

andi: {
  IMM_COMMON;
  u32 imm_se = u32(i32(instr) >> 20);
  set_reg(rd, reg(rs1) & imm_se);
  NEXT;
}
}

op_reg: {
  static void *reg_targets[8] = {
      &&add, &&sll, &&slt, &&sltu, &&xorr, &&srx, &&orr, &&andr,
  };

  goto *reg_targets[FUNCT3(instr)];

add: {
  REG_COMMON;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) + reg(rs2));
  else if (funct7 == 0b0000001) // MUL
    set_reg(rd, reg(rs1) * reg(rs2));
  else if (funct7 == 0b0100000)
    set_reg(rd, reg(rs1) - reg(rs2));
  else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

sll: {
  REG_COMMON;
  u32 shamt = reg(rs2) & 0x1f;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) << shamt);
  else if (funct7 == 0b0000001) // MULH
    set_reg(rd, u32((i64(i32(reg(rs1))) * i64(i32(reg(rs2)))) >> 32));
  else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

slt: {
  REG_COMMON;
  if (funct7 == 0b0000000)
    set_reg(rd, i32(reg(rs1)) < i32(reg(rs2)));
  else if (funct7 == 0b0000001) // MULHSU
    set_reg(rd, u32((i64(i32(reg(rs1))) * i64(u64(reg(rs2)))) >> 32));
  else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

sltu: {
  REG_COMMON;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) < reg(rs2));
  else if (funct7 == 0b0000001) // MULHU
    set_reg(rd, u32((u64(reg(rs1)) * u64(reg(rs2))) >> 32));
  else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

xorr: {
  REG_COMMON;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) ^ reg(rs2));
  else if (funct7 == 0b0000001) { // DIV
    i32 divisor = i32(reg(rs2)), dividend = i32(reg(rs1));
    if (divisor == 0)
      set_reg(rd, 0xffffffff);
    else if (dividend == INT32_MIN && divisor == -1)
      set_reg(rd, 0x80000000);
    else
      set_reg(rd, u32(dividend / divisor));
  } else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

srx: {
  REG_COMMON;
  u32 shamt = reg(rs2) & 0x1f;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) >> shamt); // SRL
  else if (funct7 == 0b0000001) {   // DIVU
    u32 divisor = reg(rs2), dividend = reg(rs1);
    if (divisor == 0)
      set_reg(rd, 0xffffffff);
    else
      set_reg(rd, dividend / divisor);
  } else if (funct7 == 0b0100000)
    set_reg(rd, u32(i32(reg(rs1)) >> shamt)); // SRA
  else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

orr: {
  REG_COMMON;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) | reg(rs2));
  else if (funct7 == 0b0000001) { // REM
    i32 divisor = i32(reg(rs2)), dividend = i32(reg(rs1));
    if (divisor == 0)
      set_reg(rd, u32(dividend));
    else if (dividend == INT32_MIN && divisor == -1)
      set_reg(rd, 0);
    else
      set_reg(rd, u32(dividend % divisor));
  } else
    EXCEPTION("Unhandled funct7");
  NEXT;
}

andr: {
  REG_COMMON;
  if (funct7 == 0b0000000)
    set_reg(rd, reg(rs1) & reg(rs2));
  else if (funct7 == 0b0000001) { // REMU
    u32 divisor = reg(rs2), dividend = reg(rs1);
    if (divisor == 0)
      set_reg(rd, dividend);
    else
      set_reg(rd, dividend % divisor);
  } else
    EXCEPTION("Unhandled funct7");
  NEXT;
}
}

op_fence: {
  WARN_PRINT("FENCE not implemented");
  NEXT;
}

op_system: {
  u32 imm_se = u32(i32(instr) >> 20);
  switch (imm_se) {
  case 0: { // ECALL
    // DEBUG_PRINT("ECALL {}", reg(17));
    switch (reg(17)) {
    case 0: { // _sbrk
      i32 increment = i32(reg(10));
      u32 old_ptr = heap_ptr;
      u32 new_ptr = u32(i32(heap_ptr) + increment);
      if (new_ptr >= reg(2))
        EXCEPTION("_sbrk requested a block too big: {} bytes", increment);
      heap_ptr = new_ptr;
      set_reg(10, old_ptr);
      break;
    }
    case 1: { // _open
      u32 start = reg(10);
      int flags = translate_open_flags(int(reg(11)));
      mode_t mode = mode_t(reg(12));
#if defined(_WIN32) && !defined(__CYGWIN__)
      flags |= O_BINARY;
#endif
      int fd = open(host_char_ptr(memory, start), flags, mode);
      set_reg(10, u32(fd));
      break;
    }
    case 2: { // _read
      int fd = int(reg(10));
      u32 start = reg(11);
      u32 nbytes = reg(12);
      ssize_t bytes_written = read(fd, &memory[start], nbytes);
      set_reg(10, u32(i32(bytes_written)));
      break;
    }
    case 3: { // _write
      int fd = int(reg(10));
      u32 start = reg(11);
      u32 nbytes = reg(12);
      ssize_t bytes_written = write(fd, &memory[start], nbytes);
      set_reg(10, u32(i32(bytes_written)));
      break;
    }
    case 4: { // _lseek
      int fd = int(reg(10));
      off_t start = off_t(reg(11));
      int whence = i32(reg(12));
      off_t offset = lseek(fd, start, whence);
      set_reg(10, u32(offset));
      break;
    }
    case 5: { // _close
      int fd = int(reg(10));
      int ret = close(fd);
      set_reg(10, u32(ret));
      break;
    }
    case 6: { // _gettimeofday
      struct timeval time;
      int ret = gettimeofday(&time, nullptr);
      set_reg(10, u32(time.tv_sec));
      set_reg(11, u32(time.tv_usec));
      set_reg(12, u32(ret));
      break;
    }
    case 7: { // _usleep
      u32 usec = reg(10);
      SDL_Delay(usec / 1000);
      set_reg(10, 0);
      break;
    }
    case 8: { // _render_frame
      should_render = true;
      return;
    }
    case 9: { // _link
      u32 oldpath = reg(10);
      u32 newpath = reg(11);
#if defined(_WIN32) && !defined(__CYGWIN__)
      BOOL res = CreateHardLinkA(host_char_ptr(memory, newpath),
                                 host_char_ptr(memory, oldpath), nullptr);
      int ret = (res == TRUE) ? 0 : -1;
#else
      int ret =
          link(host_char_ptr(memory, oldpath), host_char_ptr(memory, newpath));
#endif
      set_reg(10, u32(ret));
      break;
    }
    case 10: { // _unlink
      u32 path = reg(10);
      int ret = unlink(host_char_ptr(memory, path));
      set_reg(10, u32(ret));
      break;
    }
    case 11: { // mkdir
      u32 path = reg(10);
#if defined(_WIN32) && !defined(__CYGWIN__)
      int ret = mkdir(host_char_ptr(memory, path));
#else
      u32 mode = reg(11);
      int ret = mkdir(host_char_ptr(memory, path), mode);
#endif
      set_reg(10, u32(ret));
      break;
    }
    case 12: { // _rmdir
      u32 path = reg(10);
      int ret = rmdir(host_char_ptr(memory, path));
      set_reg(10, u32(ret));
      break;
    }
    case 15: // _exit
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
  NEXT;
}
}
