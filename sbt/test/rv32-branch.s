# 30 jal
# 31 jalr

.text
.global main
main:
  # s4: pc
  auipc s4, 0

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

  # jal
  la a0, jal_str
  jalr ra, s2, 0
  # jal ra, jal_test

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

jal_test:
  mv s4, ra

  la a0, jal_test_str
  jalr ra, s2, 0

  mv ra, s4
  jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-branch ***\n"

w_fmt: .asciz "w=0x%08X\n"

jal_str:    .asciz "jal\n"
jalr_str:   .asciz "jalr\n"

jal_test_str: .asciz "jal_test\n"
