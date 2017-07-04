.include "test.s"

START

  # Test 2: Basic test

test_2:
  li  gp, 2
  li  t0, 0
  # FIXME SBT is only able to handle
  #       indirect calls to functions/global symbols
  la  t1, target_2

  jalr t0, t1, 0
linkaddr_2:
  call failfunc

# XXX Helping the SBT to identify this as a function
.global target_2
target_2:
  la  t1, linkaddr_2
  FAILIF bne t0, t1

  # Bypassing tests

  #TEST_JALR_SRC1_BYPASS( 4, 0, jalr );
  #TEST_JALR_SRC1_BYPASS( 5, 1, jalr );
  #TEST_JALR_SRC1_BYPASS( 6, 2, jalr );

  # Test delay slot instructions not executed nor bypassed

  .option push
  .option norvc
    li  t0, 1
    la  t1, 1f

    # FIXME indirect jump not yet implemented in SBT!
    # jr  t1, -4
    # remove begin
    j 1f -4
    # remove end

    addi t0, t0, 1
    addi t0, t0, 1
    addi t0, t0, 1
    addi t0, t0, 1
1:  addi t0, t0, 1
    addi t0, t0, 1
  TEST_CASE 7, t0, 4
  .option pop

EXIT
