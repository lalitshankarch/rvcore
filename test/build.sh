#!/bin/bash
riscv32-linux-gnu-as -march=rv32i -mabi=ilp32 hello.s -o hello.o

riscv32-linux-gnu-ld -T link.ld hello.o -o hello.elf

riscv32-linux-gnu-objcopy -O binary hello.elf hello.bin