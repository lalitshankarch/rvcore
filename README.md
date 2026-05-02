# rvcore: RV32IM emulator

`rvcore` is a single-core RISC-V emulator that implements the RV32IM ISA, except for the `FENCE` and `EBREAK` instructions, which are currently `NOP`.

![Output](image.png)

<div align="center">
  <a href="https://github.com/lalitshankarch/doomgeneric">doomgeneric</a> running on rvcore
</div>

## Build instructions

1. To build, CMake and a compiler that support C++20 is required
2. Dependencies: `libsdl3-dev`
3. Run

    ```
        mkdir build
        cmake -B build/ -DCMAKE_BUILD_TYPE=Release
        cd build
        cmake --build .
    ```

## Build instructions for doomgeneric

1. The `riscv64-gnu-toolchain` configured with `./configure --with-multilib-generator="rv32im-ilp32--"` must be present in the PATH
2. Navigate to `doomgeneric` and run `make`

## Milestones

- [x] Run flat binaries written in assembly language

- [x] Run flat binaries written in C language

- [x] Run ELF binaries (binaries with a single `PT_LOAD` segment work)

- [x] Implement the `newlib` stubs needed to run DOOM

- [x] Get DOOM to boot

- [x] Make DOOM playable

- [x] Implement the M extension