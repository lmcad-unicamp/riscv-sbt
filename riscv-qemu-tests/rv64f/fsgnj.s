.include "test.s"

START

  TEST_FP_OP2_S  2,  fsgnj.s, 0, -6.3,  6.3, -1.0
  TEST_FP_OP2_S  3,  fsgnj.s, 0,  7.3,  7.3,  2.0
  TEST_FP_OP2_S  4,  fsgnj.s, 0, -8.3, -8.3, -3.0
  TEST_FP_OP2_S  5,  fsgnj.s, 0,  9.3, -9.3,  4.0

  TEST_FP_OP2_S 12, fsgnjn.s, 0,  6.3,  6.3, -1.0
  TEST_FP_OP2_S 13, fsgnjn.s, 0, -7.3,  7.3,  2.0
  TEST_FP_OP2_S 14, fsgnjn.s, 0,  8.3, -8.3, -3.0
  TEST_FP_OP2_S 15, fsgnjn.s, 0, -9.3, -9.3,  4.0

  TEST_FP_OP2_S 22, fsgnjx.s, 0, -6.3,  6.3, -1.0
  TEST_FP_OP2_S 23, fsgnjx.s, 0,  7.3,  7.3,  2.0
  TEST_FP_OP2_S 24, fsgnjx.s, 0,  8.3, -8.3, -3.0
  TEST_FP_OP2_S 25, fsgnjx.s, 0, -9.3, -9.3,  4.0

EXIT
