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

SYS_WRITE = 64

.data
ecall:  .ascii "ecall\n"
len = . - ecall

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
  li t5, 1000   # 1 ms
loop:
  csrrs t4, RDTIME, zero
  sub t4, t4, t3
  blt t4, t5, loop

  csrrs t4, RDTIME, zero
  csrrs s4, RDCYCLE, zero

  sub a1, t4, t3
  bge a1, t5, time_l
  la a0, error_str
  jalr ra, s2, 0
  j time_l2

time_l:
  la a0, time
  jalr ra, s2, 0
time_l2:

  sub a1, s4, s3
  li t1, 2000
  bge a1, t1, cycles_l
  la a0, error_str
  jalr ra, s2, 0
  j cycles_l2

cycles_l:
  la a0, cycles
  jalr ra, s2, 0
cycles_l2:

  # rdinstret
  csrrs t1, RDINSTRET, zero
  nop
  nop
  nop
  nop
  nop
#  la t3, test
#  li t4, 123
#  sw t4, 0(t3)
#  li t4, 456
#  sw t4, 0(t3)
#  li t4, 789
#  sw t4, 0(t3)
  csrrs t2, RDINSTRET, zero
  sub a1, t2, t1
  li t1, 5
  bge a1, t1, instret_l
  la a0, error_str
  jalr ra, s2, 0
  j instret_l2

instret_l:
  la a0, insts
  jalr ra, s2, 0
instret_l2:

  # ecall
  li a0, 0
  la t1, fflush
  jalr ra, t1, 0

  li a0, 1
  la a1, ecall
  li a2, len
  li a7, SYS_WRITE
  ecall

  # ebreak
  la a0, ebreak
  jalr ra, s2, 0
  #ebreak

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

.data
.p2align 2
str:    .asciz "*** rv32-system ***\n"
test:   .int 0

cycles: .asciz "cycles OK\n"
time:   .asciz "time OK\n"
insts:  .asciz "instret OK\n"
error_str:  .asciz "ERROR\n"
ebreak: .asciz "ebreak\n"
