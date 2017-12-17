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
  call printf

  # jalr with offset
  la t1, printf
  addi t1, t1, -0x10
  la a0, jalr_test_str
  jalr ra, t1, 0x10

  mv ra, s5
  ret


.global main
main:
  # s2: original ra
  mv s2, ra

  # print test
  la a0, str
  call printf

  # jal
  la a0, jal_str
  call printf
  jal jal_test

  # jalr
  la t1, jalr_test
  jalr t1

  # beq
  la a0, beq_str
  call printf
  li t1, 5
  li t2, 5
  beq t1, t2, beq_l
  call test_error
beq_l:

  # bne
  la a0, bne_str
  call printf
  li t1, 5
  li t2, 5
  bne t1, t2, bne_err
  li t2, 6
  bne t1, t2, bne_l
bne_err:
  call test_error
bne_l:

  # blt
  la a0, blt_str
  call printf
  li t1, -5
  li t2, -10
  blt t2, t1, blt_l
  call test_error
blt_l:

  # bltu
  la a0, bltu_str
  call printf
  li t1, -5
  li t2, -10
  bltu t2, t1, bltu_l
  call test_error
bltu_l:

  # bge
  li s5, 2
bge_l:
  la a0, bge_str
  call printf
  addi s5, s5, -1
  bge s5, zero, bge_l

  # bgeu
  la a0, bgeu_str
  call printf
  li t1, -1
  bgeu t1, zero, bgeu_l
  call test_error
bgeu_l:

  # restore ra
  add ra, zero, s2

  # return 0
  mv a0, zero
  ret

.type jal_test,@function
jal_test:
  mv s4, ra

  la a0, jal_test_str
  call printf

  j jal_test_1
jal_test_2:
  la a0, jal_test2_str
  call printf
  j jal_test_3

jal_test_1:
  la a0, jal_test1_str
  call printf
  j jal_test_2

jal_test_3:
  mv ra, s4
  ret


.type test_error,@function
test_error:
  la a0, error_str
  call printf
  call abort


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
dbg_str:    .asciz "dbg\n"
