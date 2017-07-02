.include "test.s"

START

  TEST_RR_OP  2, divuw,         3,  20,   6
  TEST_RR_OP  3, divuw, 715827879, -20 << 32 >> 32,   6
  TEST_RR_OP  4, divuw,         0,  20,  -6
  TEST_RR_OP  5, divuw,         0, -20,  -6

  TEST_RR_OP  6, divuw, -1<<31, -1<<31,  1
  TEST_RR_OP  7, divuw, 0,      -1<<31, -1

  TEST_RR_OP  8, divuw, -1, -1<<31, 0
  TEST_RR_OP  9, divuw, -1,      1, 0
  TEST_RR_OP 10, divuw, -1,      0, 0

EXIT
