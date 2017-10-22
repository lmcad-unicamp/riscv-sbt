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

    .align 3
    lla a0, 1f + 10000
    jal a1, 1f
    1: sub a0, a0, a1
  TEST_CASE 2, a0, 10000

    .align 3
    lla a0, 1f - 10000
    jal a1, 1f
    1: sub a0, a0, a1
  TEST_CASE 3, a0, -10000

EXIT
