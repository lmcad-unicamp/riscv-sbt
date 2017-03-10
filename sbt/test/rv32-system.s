# 40 csrrw
# 41 csrrs
# 42 csrrc
# 43 csrrwi
# 44 csrrsi
# 45 csrrci
# 46 ecall
# 47 ebreak

RDCYCLE     = 0xC00
RDCYCLEH    = 0xC80
RDTIME      = 0xC01
RDTIMEH     = 0xC81
RDINSTRET   = 0xC02
RDINSTRETH  = 0xC82

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

  # rdcycle
  # rdtime
  csrrs s3, RDCYCLE, zero

  csrrs t3, RDTIME, zero
  li t5, 500
loop:
  csrrs t4, RDTIME, zero
  sub t4, t4, t3
  blt t4, t5, loop

  csrrs t4, RDTIME, zero
  csrrs s4, RDCYCLE, zero

  sub a1, t4, t3
  la a0, time
  jalr ra, s2, 0

  sub a1, s4, s3
  la a0, cycles
  jalr ra, s2, 0

  # rdinstret
  csrrs t1, RDINSTRET, zero
  nop
  nop
  nop
  csrrs t2, RDINSTRET, zero
  sub a1, t2, t1
  la a0, insts
  jalr ra, s2, 0

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-system ***\n"

cycles: .asciz "cycles=%u\n"
time:   .asciz "time=%u\n"
insts:  .asciz "insts=%u\n"
