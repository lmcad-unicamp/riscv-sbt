# 38 fence
# 39 fence.i

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

  # XXX this actually tests nothing for now...

  # store
  la t2, x
  li t1, 0x12345678
  sw t1, 0(t2)

  # fence
  fence
  fence.i

  # load
  la a0, fmt
  lw a1, 0(t2)
  jalr ra, s2, 0

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-fence ***\n"

x: .word 0
fmt:  .asciz "0x%08X\n"
