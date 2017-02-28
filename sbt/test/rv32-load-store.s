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

  # lb
  # b0
  lui a0, %hi(b0_fmt)
  addi a0, a0, %lo(b0_fmt)
  lui t1, %hi(b0)
  addi t1, t1, %lo(b0)
  lb a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0
  # b1
  lui a0, %hi(b1_fmt)
  addi a0, a0, %lo(b1_fmt)
  lui t1, %hi(b1)
  addi t1, t1, %lo(b1)
  lb a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0

  # lbu
  # bu0
  lui a0, %hi(bu0_fmt)
  addi a0, a0, %lo(bu0_fmt)
  lui t1, %hi(b0)
  addi t1, t1, %lo(b0)
  lbu a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0
  # bu1
  lui a0, %hi(bu1_fmt)
  addi a0, a0, %lo(bu1_fmt)
  lui t1, %hi(bu1)
  addi t1, t1, %lo(bu1)
  lbu a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0

  # lh
  # h0
  lui a0, %hi(h0_fmt)
  addi a0, a0, %lo(h0_fmt)
  lui t1, %hi(h0)
  addi t1, t1, %lo(h0)
  lh a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0
  # b1
  lui a0, %hi(h1_fmt)
  addi a0, a0, %lo(h1_fmt)
  lui t1, %hi(h1)
  addi t1, t1, %lo(h1)
  lh a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0

  # lhu
  # hu0
  lui a0, %hi(hu0_fmt)
  addi a0, a0, %lo(hu0_fmt)
  lui t1, %hi(h0)
  addi t1, t1, %lo(h0)
  lhu a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0
  # hu1
  lui a0, %hi(hu1_fmt)
  addi a0, a0, %lo(hu1_fmt)
  lui t1, %hi(hu1)
  addi t1, t1, %lo(hu1)
  lhu a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0

  # lw
  # signed
  lui a0, %hi(w0_fmt)
  addi a0, a0, %lo(w0_fmt)
  lui t1, %hi(w)
  addi t1, t1, %lo(w)
  lw a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0
  # unsigned
  lui a0, %hi(w1_fmt)
  addi a0, a0, %lo(w1_fmt)
  lui t1, %hi(w)
  addi t1, t1, %lo(w)
  lw a1, 0(t1)
  add a2, a1, zero
  jalr ra, s2, 0

  # restore ra
  add ra, zero, s1

  # return 0
  add a0, zero, zero
  jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-load-store ***\n"

b0: .byte 127
b0_fmt: .asciz "b0=%hhi 0x%08X\n"
b1: .byte -128
b1_fmt: .asciz "b1=%hhi 0x%08X\n"
bu0_fmt: .asciz "bu0=%hhu 0x%08X\n"
bu1: .byte 255
bu1_fmt: .asciz "bu1=%hhu 0x%08X\n"

h0: .word 32767
h0_fmt: .asciz "h0=%hi 0x%08X\n"
h1: .word -32768
h1_fmt: .asciz "h1=%hi 0x%08X\n"
hu0_fmt: .asciz "hu0=%hu 0x%08X\n"
hu1: .word 65535
hu1_fmt: .asciz "hu1=%hu 0x%08X\n"

w: .dword 0x89ABCDEF
w0_fmt: .asciz "w0=%i 0x%08X\n"
w1_fmt: .asciz "w1=%u 0x%08X\n"
