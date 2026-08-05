#!/bin/bash

riscv64-unknown-elf-gcc -nostartfiles hello.c -T link.ld -o hello -march=rv32im -mabi=ilp32
