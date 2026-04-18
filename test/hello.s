.section .text
.globl _start

_start:
  lui  a0, %hi(msg)
  addi a0, a0, %lo(msg)

  li   a7, 4
  ecall

  li   a7, 10
  ecall

.section .rodata
msg:
  .asciz "Hello world!"
