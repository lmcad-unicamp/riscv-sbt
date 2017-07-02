.include "test.s"

START

  TEST_INT_FP_OP_S  2,  fcvt.s.w,                   2.0,  2
  TEST_INT_FP_OP_S  3,  fcvt.s.w,                  -2.0, -2

  TEST_INT_FP_OP_S  4, fcvt.s.wu,                   2.0,  2
  TEST_INT_FP_OP_S  5, fcvt.s.wu,           4.2949673e9, -2

  TEST_INT_FP_OP_S  6,  fcvt.s.l,                   2.0,  2
  TEST_INT_FP_OP_S  7,  fcvt.s.l,                  -2.0, -2

  TEST_INT_FP_OP_S  8, fcvt.s.lu,                   2.0,  2
  TEST_INT_FP_OP_S  9, fcvt.s.lu,          1.8446744e19, -2

EXIT
