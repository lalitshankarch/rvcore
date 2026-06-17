#!/bin/bash

riscv64-unknown-elf-gcc -nostartfiles hello.c -T link.ld -o hello -march=rv32im -mabi=ilp32
# riscv64-unknown-elf-ld -m elf32lriscv -T link.ld hello.o -o hello.elf
# riscv64-unknown-elf-objcopy -O binary hello hello.bin