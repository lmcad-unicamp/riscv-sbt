.include "test.s"

START

  # Arithmetic tests

  TEST_RR_OP 2,  sll, 0x00000001, 0x00000001, 0 
  TEST_RR_OP 3,  sll, 0x00000002, 0x00000001, 1 
  TEST_RR_OP 4,  sll, 0x00000080, 0x00000001, 7 
  TEST_RR_OP 5,  sll, 0x00004000, 0x00000001, 14
  TEST_RR_OP 6,  sll, 0x80000000, 0x00000001, 31

  TEST_RR_OP 7,  sll, 0xffffffff, 0xffffffff, 0 
  TEST_RR_OP 8,  sll, 0xfffffffe, 0xffffffff, 1 
  TEST_RR_OP 9,  sll, 0xffffff80, 0xffffffff, 7 
  TEST_RR_OP 10, sll, 0xffffc000, 0xffffffff, 14
  TEST_RR_OP 11, sll, 0x80000000, 0xffffffff, 31

  TEST_RR_OP 12, sll, 0x21212121, 0x21212121, 0 
  TEST_RR_OP 13, sll, 0x42424242, 0x21212121, 1 
  TEST_RR_OP 14, sll, 0x90909080, 0x21212121, 7 
  TEST_RR_OP 15, sll, 0x48484000, 0x21212121, 14
  TEST_RR_OP 16, sll, 0x80000000, 0x21212121, 31

  # Verify that shifts only use bottom six bits

  TEST_RR_OP 17, sll, 0x21212121, 0x21212121, 0xffffffc0
  TEST_RR_OP 18, sll, 0x42424242, 0x21212121, 0xffffffc1
  TEST_RR_OP 19, sll, 0x90909080, 0x21212121, 0xffffffc7
  TEST_RR_OP 20, sll, 0x48484000, 0x21212121, 0xffffffce
  TEST_RR_OP 21, sll, 0x80000000, 0x21212121, 0xffffffff

  # Source/Destination tests

  TEST_RR_SRC1_EQ_DEST 22, sll, 0x00000080, 0x00000001, 7 
  TEST_RR_SRC2_EQ_DEST 23, sll, 0x00004000, 0x00000001, 14
  TEST_RR_SRC12_EQ_DEST 24, sll, 24, 3

  TEST_RR_ZEROSRC1  40, sll, 0, 15
  TEST_RR_ZEROSRC2  41, sll, 32, 32
  TEST_RR_ZEROSRC12 42, sll, 0
  TEST_RR_ZERODEST  43, sll, 1024, 2048

EXIT
