#!/bin/bash

riscv64-linux-gnu-as -march=rv32i -mabi=ilp32 hello.s -o hello.o

riscv64-linux-gnu-ld -m elf32lriscv -T link.ld hello.o -o hello.elf

riscv64-linux-gnu-objcopy -O binary hello.elf hello.bin