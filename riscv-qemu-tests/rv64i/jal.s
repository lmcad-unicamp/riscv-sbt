.include "test.s"

START

  # Test 2: Basic test

test_2:
    li gp, 2
    li ra, 0

    jal x3, target_2
linkaddr_2:
    nop
    nop

    call failfunc

target_2:
    lsym x2, linkaddr_2
    FAILIF bne x2, x3

  # Test delay slot instructions not executed nor bypassed

    li  ra, 1
    jal x0, 1f
    addi ra, ra, 1
    addi ra, ra, 1
    addi ra, ra, 1
    addi ra, ra, 1
1:  addi ra, ra, 1
    addi ra, ra, 1
  TEST_CASE 3, ra, 3

EXIT
