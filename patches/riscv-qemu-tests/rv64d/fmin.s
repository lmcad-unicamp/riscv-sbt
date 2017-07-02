.include "test.s"

START

  TEST_FP_OP2_D  2,  fmin.d, 0,        1.0,        2.5,        1.0
  TEST_FP_OP2_D  3,  fmin.d, 0,    -1235.1,    -1235.1,        1.1
  TEST_FP_OP2_D  4,  fmin.d, 0,    -1235.1,        1.1,    -1235.1
  TEST_FP_OP2_D  5,  fmin.d, 0,    -1235.1,        NaN,    -1235.1
  TEST_FP_OP2_D  6,  fmin.d, 0, 0.00000001, 3.14159265, 0.00000001
  TEST_FP_OP2_D  7,  fmin.d, 0,       -2.0,       -1.0,       -2.0

  TEST_FP_OP2_D 12,  fmax.d, 0,        2.5,        2.5,        1.0
  TEST_FP_OP2_D 13,  fmax.d, 0,        1.1,    -1235.1,        1.1
  TEST_FP_OP2_D 14,  fmax.d, 0,        1.1,        1.1,    -1235.1
  TEST_FP_OP2_D 15,  fmax.d, 0,    -1235.1,        NaN,    -1235.1
  TEST_FP_OP2_D 16,  fmax.d, 0, 3.14159265, 3.14159265, 0.00000001
  TEST_FP_OP2_D 17,  fmax.d, 0,       -1.0,       -1.0,       -2.0

EXIT
