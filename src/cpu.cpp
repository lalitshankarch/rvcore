#include "cpu.h"
#include "constants.h"
#include "debug.h"
#include "x86_64.h"
#include <SDL3/SDL_timer.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <memoryapi.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <utility>

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

static Cpu *cpu_ptr;

Cpu::Cpu(std::vector<u8> &mem, u32 pc_start, u32 prog_end, u32 heap_start)
    : pc(pc_start), end(prog_end), heap_ptr(heap_start), sys_code(UINT32_MAX),
      memory(mem), should_render(false) {
  regs = {};
  regs[2] = STACK_START; // Set stack pointer
  cpu_ptr = this;
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

void Cpu::execute_threaded() {
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
      // EXCEPTION("EXIT");
      return;
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

void Cpu::system() {
  switch (sys_code) {
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
      // EXCEPTION("EXIT");
      return;
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
}

void Cpu::execute() {
  std::pair<void *, u64> ret = jit_compile();
  void (*func)() = reinterpret_cast<void (*)()>(ret.first);

  // for (int i = 0; i < 960; i++) {
  //   // %02X prints each byte as a 2-digit uppercase hex number (e.g., 0A, FF)
  //   printf("%02X ", *((u8 *)ret.first + i));

  //   // Optional: Print a newline every 16 bytes for clean formatting
  //   if ((i + 1) % 16 == 0) {
  //     printf("\n");
  //   }
  // }

  constexpr int iterations = 1'00'000'000;

  auto start_time = std::chrono::steady_clock::now();

  pc = 0;
  for (int i = 0; i < iterations; i++)
    for (int j = 0; j < 1; j++) {
      func();
    }

  auto end_time = std::chrono::steady_clock::now();

  double ms =
      std::chrono::duration<double, std::milli>(end_time - start_time).count();

  printf("Time: %.3f ms\n", ms);
  printf("a0: %d, a1: %d, a2: %d, a3: %d, a4: %d, a5: %d, a6: %d, t0: %d, t1: "
         "%d, t2: %d, x0: %d",
         reg(10), reg(11), reg(12), reg(13), reg(14), reg(15), reg(16), reg(5),
         reg(6), reg(7), reg(0));
  EXCEPTION("Done executing basic block");
}

static void call_system() { cpu_ptr->system(); }

std::pair<void *, u64> Cpu::jit_compile() {
  printf("compile basic block at: 0x%x\n", pc);

  std::pair<void *, u64> ret = {nullptr, -1};

  void *ptr =
      VirtualAlloc(nullptr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  x86_64::ptr = ptr;
  if (!ptr) {
    printf("VirtualAlloc failed: %lu\n", GetLastError());
  }
  ret.first = ptr;

  x86_64::mov(RDX, u64(&regs[0]));

  while (pc < end) {
    instr = load32_(memory, pc);
    pc += 4;

    switch (instr & 0x7f) {
    case OP_LUI: {
      u32 rd = (instr >> 7) & 0x1f;
      u32 u_imm = (instr >> 12) << 12;
      if (rd == 0)
        break;
      x86_64::mov(RDX, i32(rd) * 4, u_imm);
      break;
    }
    // case OP_BRANCH: {
    //   u32 imm = (((instr >> 31) & 0x1) << 12) | (((instr >> 7) & 0x1) << 11)
    //   |
    //             (((instr >> 25) & 0x3f) << 5) | (((instr >> 8) & 0xf) << 1);
    //   u32 offset = u32(i32(imm << 19) >> 19);
    //   u32 addr = (pc - 4) + offset;
    //   u32 rs2 = (instr >> 20) & 0x1f;
    //   u32 rs1 = (instr >> 15) & 0x1f;
    //   u32 funct3 = (instr >> 12) & 0x7;
    //   switch (funct3) {
    //   case BNE: {
    //     if (reg(rs1) != reg(rs2))
    //       pc = addr;
    //     break;
    //   }
    //   }
    // }
    case OP_IMM: {
      u32 rd = (instr >> 7) & 0x1f;
      if (rd == 0)
        break;
      u32 funct3 = (instr >> 12) & 0x7;
      u32 rs1 = (instr >> 15) & 0x1f;
      u32 imm_se = u32(i32(instr) >> 20);
      switch (funct3) {
      case ADDI: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::add(EAX, imm_se);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case SLLI: {
        u32 funct7 = instr >> 25;
        u8 shamt = (instr >> 20) & 0x1f;
        if (funct7 == 0b0000000) {
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::sal(EAX, shamt);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        } else
          EXCEPTION("Unhandled funct7");
        break;
      }
      case SLTI: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::cmp(EAX, imm_se);
        x86_64::setl(AL);
        x86_64::movzx(EAX, AL);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case SLTIU: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::cmp(EAX, imm_se);
        x86_64::setb(AL);
        x86_64::movzx(EAX, AL);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case XORI: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::xori(EAX, imm_se);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case SRXI: {
        u32 funct7 = instr >> 25;
        u8 shamt = (instr >> 20) & 0x1f;
        if (funct7 == 0b0000000) { // SRLI
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::shr(EAX, shamt);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        } else if (funct7 == 0b0100000) { // SRAI
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::sar(EAX, shamt);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        } else {
          EXCEPTION("Unhandled funct7");
        }
        break;
      }
      case ORI: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::ori(EAX, imm_se);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case ANDI: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::andi(EAX, imm_se);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      }
      break;
    }
    case OP_REG: {
      u32 rd = (instr >> 7) & 0x1f;
      if (rd == 0)
        break;
      u32 funct3 = (instr >> 12) & 0x7;
      u32 rs1 = (instr >> 15) & 0x1f;
      u32 rs2 = (instr >> 20) & 0x1f;
      u32 funct7 = instr >> 25;
      switch (funct3) {
      case ADD: {
        if (funct7 == 0b0000000) {
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::mov(ECX, RDX, i32(rs2) * 4);
          x86_64::addr(EAX, ECX);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        } else if (funct7 == 0b0100000) {
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::mov(ECX, RDX, i32(rs2) * 4);
          x86_64::subr(EAX, ECX);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        }
        break;
      }
      case SLL: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::mov(ECX, RDX, i32(rs2) * 4);
        x86_64::sal(EAX);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case SLT: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::mov(ECX, RDX, i32(rs2) * 4);
        x86_64::cmp(EAX, ECX);
        x86_64::setl(AL);
        x86_64::movzx(EAX, AL);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case SLTU: {
        x86_64::mov(EAX, RDX, i32(rs1) * 4);
        x86_64::mov(ECX, RDX, i32(rs2) * 4);
        x86_64::cmp(EAX, ECX);
        x86_64::setb(AL);
        x86_64::movzx(EAX, AL);
        x86_64::mov(RDX, i32(rd) * 4, EAX);
        break;
      }
      case XOR: {
        if (funct7 == 0b0000000) {
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::mov(ECX, RDX, i32(rs2) * 4);
          x86_64::xorr(EAX, ECX);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        }
        break;
      }
      case SRX: {
        if (funct7 == 0b0000000) { // SRL
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::mov(ECX, RDX, i32(rs2) * 4);
          x86_64::shr(EAX);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        } else if (funct7 == 0b0100000) { // SRA
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::mov(ECX, RDX, i32(rs2) * 4);
          x86_64::sar(EAX);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        } else {
          EXCEPTION("Unhandled funct7");
        }
        break;
      }
      case OR: {
        if (funct7 == 0b0000000) {
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::mov(ECX, RDX, i32(rs2) * 4);
          x86_64::orr(EAX, ECX);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        }
        break;
      }
      case AND: {
        if (funct7 == 0b0000000) {
          x86_64::mov(EAX, RDX, i32(rs1) * 4);
          x86_64::mov(ECX, RDX, i32(rs2) * 4);
          x86_64::andr(EAX, ECX);
          x86_64::mov(RDX, i32(rd) * 4, EAX);
        }
        break;
      }
      }
      break;
    }
    case OP_SYSTEM: {
      u32 imm_se = u32(i32(instr) >> 20);
      x86_64::mov(RCX, u64(&sys_code));
      x86_64::mov(RCX, 0, imm_se);
      x86_64::mov(RCX, u64(call_system));
      x86_64::call(RCX);
      break;
    }
    default: {
      goto exit;
    }
    }
  }

exit:

  x86_64::ret();

  DWORD old;
  if (!VirtualProtect(ret.first, 4096, PAGE_EXECUTE_READ, &old)) {
    printf("VirtualProtect failed: %lu\n", GetLastError());
  }

  printf("Wrote: %lld bytes\n", u64(x86_64::ptr) - u64(ret.first));

  return ret;
}