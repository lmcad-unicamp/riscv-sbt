# 01 mul

.text
.global main
main:
  # save ra
  add s1, zero, ra

  # s2: printf
  lui s2, %hi(printf)
  addi s2, s2, %lo(printf)

  # print test
  lui a0, %hi(str)
  addi a0, a0, %lo(str)
  jalr ra, s2, 0

  # mul
  la a0, mul
  jalr ra, s2, 0
  li t1, -2
  li t2, 45
  mul a1, t1, t2
  la a0, fmt
  jalr ra, s2, 0

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-m ***\n"

mul:  .asciz "mul\n"
fmt:  .asciz "%i\n"
