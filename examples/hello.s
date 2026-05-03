.section .text
.globl _start

_start:
  addi a0, x0, 1
  lui  a1, %hi(msg)
  addi a1, a0, %lo(msg)
  addi a2, x0, 13

  li   a7, 4
  ecall

  li   a7, 10
  ecall

.section .rodata
msg:
  .asciz "Hello world!\n"
