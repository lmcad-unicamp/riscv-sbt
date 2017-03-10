# 30 jal
# 31 jalr
# 32 beq
# 33 bne
# 34 blt
# 35 bltu
# 36 bge
# 37 bgeu

.text

.type jalr_test,@function
jalr_test:
  mv s5, ra

  la a0, jalr_test_str
  jalr ra, s2, 0

  # jalr with offset
  mv t1, s2
  addi t1, t1, -0x10
  la a0, jalr_test_str
  jalr ra, t1, 0x10

  mv ra, s5
  jalr zero, ra, 0


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
  jal ra, jal_test

  # jalr
  la t1, jalr_test
  jalr ra, t1, 0

  # beq
  la a0, beq_str
  jalr ra, s2, 0
  li t1, 5
  li t2, 5
  beq t1, t2, beq_l
  la a0, error_str
  jalr ra, s2, 0
beq_l:

  # bne
  la a0, bne_str
  jalr ra, s2, 0
  li t1, 5
  li t2, 5
  bne t1, t2, bne_l
  la a0, blt_str
  li t2, 6
  bne t1, t2, bne_l
  la a0, error_str
  jalr ra, s2, 0
bne_l:

  # blt
  jalr ra, s2, 0
  li t1, -5
  li t2, -10
  blt t2, t1, blt_l
  la a0, error_str
  jalr ra, s2, 0
blt_l:

  # bltu
  la a0, bltu_str
  jalr ra, s2, 0
  li t1, -5
  li t2, -10
  bltu t2, t1, bltu_l
  la a0, error_str
  jalr ra, s2, 0
bltu_l:

  # bge
  li s5, 2
bge_l:
  la a0, bge_str
  jalr ra, s2, 0
  addi s5, s5, -1
  bge s5, zero, bge_l

  # bgeu
  la a0, bgeu_str
  jalr ra, s2, 0
  li t1, -1
  bgeu t1, zero, bgeu_l
  la a0, error_str
  jalr ra, s2, 0
bgeu_l:

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

.type jal_test,@function
jal_test:
  mv s4, ra

  la a0, jal_test_str
  jalr ra, s2, 0

  jal zero, jal_test_1
jal_test_2:
  la a0, jal_test2_str
  jalr ra, s2, 0
  jal zero, jal_test_3

jal_test_1:
  la a0, jal_test1_str
  jalr ra, s2, 0
  jal zero, jal_test_2

jal_test_3:
  mv ra, s4
  jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-branch ***\n"

w_fmt: .asciz "w=0x%08X\n"

jal_str:    .asciz "jal\n"
jalr_str:   .asciz "jalr\n"

jal_test_str: .asciz "jal_test\n"
jal_test1_str: .asciz "jal_test1\n"
jal_test2_str: .asciz "jal_test2\n"

jalr_test_str: .asciz "jalr_test\n"

beq_str:  .asciz "beq\n"
bne_str:  .asciz "bne\n"
blt_str:  .asciz "blt\n"
bltu_str: .asciz "bltu\n"
bge_str:  .asciz "bge\n"
bgeu_str: .asciz "bgeu\n"

error_str:  .asciz "error\n"
