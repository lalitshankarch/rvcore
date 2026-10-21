## Build instructions

1. To build rvcore, CMake and a C++ compiler are required. On Linux/macOS, use GCC or Clang. On Windows, use MSVC, MSYS2 (MinGW-w64), or Cygwin.

2. Run

    ```
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
        cmake --build build --config Release
    ```

## Build instructions for doomgeneric

1. Clone https://github.com/lalitshankarch/doomgeneric
2. `riscv64-unknown-elf-gcc` or the `riscv64-gnu-toolchain` configured with `./configure --with-multilib-generator="rv32im-ilp32--"` must be installed
3. Navigate to `doomgeneric` and run `make`