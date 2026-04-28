#!/bin/bash

riscv64-unknown-elf-gcc -ffreestanding -nostdlib hello.c -T link.ld -o hello -march=rv32i -mabi=ilp32 -lc -lgcc
# riscv64-unknown-elf-ld -m elf32lriscv -T link.ld hello.o -o hello.elf
# riscv64-unknown-elf-objcopy -O binary hello hello.bin