.include "test.s"

START

  # Arithmetic tests

  TEST_RR_OP 2,  slt, 0, 0x00000000, 0x00000000
  TEST_RR_OP 3,  slt, 0, 0x00000001, 0x00000001
  TEST_RR_OP 4,  slt, 1, 0x00000003, 0x00000007
  TEST_RR_OP 5,  slt, 0, 0x00000007, 0x00000003

  TEST_RR_OP 6,  slt, 0, 0x00000000, 0xffff8000
  TEST_RR_OP 7,  slt, 1, 0x80000000, 0x00000000
  TEST_RR_OP 8,  slt, 1, 0x80000000, 0xffff8000

  TEST_RR_OP 9,  slt, 1, 0x00000000, 0x00007fff
  TEST_RR_OP 10, slt, 0, 0x7fffffff, 0x00000000
  TEST_RR_OP 11, slt, 0, 0x7fffffff, 0x00007fff

  TEST_RR_OP 12, slt, 1, 0x80000000, 0x00007fff
  TEST_RR_OP 13, slt, 0, 0x7fffffff, 0xffff8000

  TEST_RR_OP 14, slt, 0, 0x00000000, 0xffffffff
  TEST_RR_OP 15, slt, 1, 0xffffffff, 0x00000001
  TEST_RR_OP 16, slt, 0, 0xffffffff, 0xffffffff

  # Source/Destination tests

  TEST_RR_SRC1_EQ_DEST 17, slt, 0, 14, 13
  TEST_RR_SRC2_EQ_DEST 18, slt, 1, 11, 13
  TEST_RR_SRC12_EQ_DEST 19, slt, 0, 13


  TEST_RR_ZEROSRC1 35, slt, 0, -1
  TEST_RR_ZEROSRC2 36, slt, 1, -1
  TEST_RR_ZEROSRC12 37, slt, 0
  TEST_RR_ZERODEST 38, slt, 16, 30

EXIT
