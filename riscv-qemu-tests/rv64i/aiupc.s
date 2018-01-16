.include "test.s"

#test_str: .ascii "test_str\n"
#.equ TLEN, . - test_str
#.type test_str, object
#.size test_str, TLEN
#
#.align 4
#.global test
#test:
#    li a0, 2
#    la a1, test_str
#    li a2, TLEN
#    li a7, 64 /* write */
#    ecall
#    ret

START

    # HI:
    # a0 = pc + ((1f + 10000 - pc) & 0xFFFFF000)
    # LO:
    # a0 = a0 + ((1f + 10000 - pc) & 0xFFF)
    L1    = 1f + 10000
    L1_LO = (L1 - .) & 0xFFF

    # lla a0, 1f + 10000
    auipc a0, %pcrel_hi(L1)
    addi  a0, a0, %lo(L1_LO)

    jal a1, 1f
    1: sub a0, a0, a1
  TEST_CASE 2, a0, 10000

    L2    = 1f - 10000
    L2_LO = (L2 - .) & 0xFFF

    # lla a0, 1f - 10000
    auipc a0, %pcrel_hi(L2)
    addi  a0, a0, %lo(L2_LO)

    jal a1, 1f
    1: sub a0, a0, a1
  TEST_CASE 3, a0, -10000

EXIT
