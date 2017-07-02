.include "test.s"

START

  # Arithmetic tests

  TEST_RR_OP 2, rem,  2,  20,   6
  TEST_RR_OP 3, rem, -2, -20,   6
  TEST_RR_OP 4, rem,  2,  20,  -6
  TEST_RR_OP 5, rem, -2, -20,  -6

  TEST_RR_OP 6, rem,  0, -1<<63,  1
  TEST_RR_OP 7, rem,  0, -1<<63, -1

  TEST_RR_OP 8, rem, -1<<63, -1<<63, 0
  TEST_RR_OP 9, rem,      1,      1, 0
  TEST_RR_OP 10, rem,      0,      0, 0

EXIT
