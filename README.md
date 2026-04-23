# rvcore: RV32I emulator

`rvcore` is a single-core RISC-V emulator that implements the RV32I ISA, except for the `FENCE` and `EBREAK` instructions, which are currently `NOP`.

## Test output

![Output](image.png)

## Milestones

- [x] Run flat binaries written in assembly language

- [x] Run flat binaries written in C language

- [ ] Run ELF binaries

- [ ] Implement the rest of the `newlib` stubs (`_sbrk`, `_read`, `_fstat`, `_lseek`, etc)
