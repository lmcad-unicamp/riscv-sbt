# 09 lui
# 10 add
# 11 addi
# 12 slti
# 13 sltiu

.text
.global main
main:
  # s1: original ra
  add s1, zero, ra

  # s2: printf
  lui s2, %hi(printf)
  addi s2, s2, %lo(printf)

  # s3: printf's fmt
  lui s3, %hi(w_fmt)
  addi s3, s3, %lo(w_fmt)

  # print test
  lui a0, %hi(str)
  addi a0, a0, %lo(str)
  jalr ra, s2, 0

  # lui
  mv a0, s3
  lui a1, 0xCDEF5
  jalr ra, s2, 0

  # add/addi
  # 0
  addi t0, zero, 0x222
  addi t1, zero, 0x333
  mv a0, s3
  add a1, t0, t1
  jalr ra, s2, 0
  # 1
  addi t0, zero, 0x777
  addi t1, zero, -0x333
  mv a0, s3
  add a1, t0, t1
  jalr ra, s2, 0

  # slti
  addi t0, zero, -0x111
  mv a0, s3
  slti a1, t0, 0x111
  jalr ra, s2, 0

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-alu-ops ***\n"

w_fmt: .asciz "w=0x%08X\n"
