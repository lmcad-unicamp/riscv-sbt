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
  csrrs a2, RDCYCLE, zero
  csrrs a3, RDCYCLEH, zero
  la a0, cycles
  jalr ra, s2, 0

  # rdtime
  csrrs a2, RDTIME, zero
  csrrs a3, RDTIMEH, zero
  la a0, time
  jalr ra, s2, 0

  # rdinstret
  csrrs a2, RDINSTRET, zero
  csrrs a3, RDINSTRETH, zero
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

cycles: .asciz "cycles=%llu\n"
time:   .asciz "time=%llu\n"
insts:  .asciz "insts=%llu\n"
