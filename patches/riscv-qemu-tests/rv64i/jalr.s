.include "test.s"

START

  # Test 2: Basic test

test_2:
  li  gp, 2
  li  t0, 0
  la  t1, target_2

  jalr t0, t1, 0
linkaddr_2:
  j fail

target_2:
  la  t1, linkaddr_2
  bne t0, t1, fail

  # Bypassing tests

  #TEST_JALR_SRC1_BYPASS( 4, 0, jalr );
  #TEST_JALR_SRC1_BYPASS( 5, 1, jalr );
  #TEST_JALR_SRC1_BYPASS( 6, 2, jalr );

  # Test delay slot instructions not executed nor bypassed

  .option push
  .option norvc
    li  t0, 1
    la  t1, 1f
    jr  t1, -4
    addi t0, t0, 1
    addi t0, t0, 1
    addi t0, t0, 1
    addi t0, t0, 1
1:  addi t0, t0, 1
    addi t0, t0, 1
  TEST_CASE 7, t0, 4
  .option pop

EXIT
