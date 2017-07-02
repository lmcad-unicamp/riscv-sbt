.include "test.s"

START

  TEST_FP_CMP_OP_S 2, feq.s, 1, -1.36, -1.36
  TEST_FP_CMP_OP_S 3, fle.s, 1, -1.36, -1.36
  TEST_FP_CMP_OP_S 4, flt.s, 0, -1.36, -1.36

  TEST_FP_CMP_OP_S 5, feq.s, 0, -1.37, -1.36
  TEST_FP_CMP_OP_S 6, fle.s, 1, -1.37, -1.36
  TEST_FP_CMP_OP_S 7, flt.s, 1, -1.37, -1.36

EXIT
