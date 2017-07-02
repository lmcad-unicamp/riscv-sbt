.include "test.s"

START

  # Arithmetic tests

  TEST_RR_OP 2,  mulhu, 0x00000000, 0x00000000, 0x00000000
  TEST_RR_OP 3,  mulhu, 0x00000000, 0x00000001, 0x00000001
  TEST_RR_OP 4,  mulhu, 0x00000000, 0x00000003, 0x00000007

  TEST_RR_OP 5,  mulhu, 0x0000000000000000, 0x0000000000000000, 0xffffffffffff8000
  TEST_RR_OP 6,  mulhu, 0x0000000000000000, 0xffffffff80000000, 0x00000000
  TEST_RR_OP 7,  mulhu, 0xffffffff7fff8000, 0xffffffff80000000, 0xffffffffffff8000

  TEST_RR_OP 30,  mulhu, 0x000000000001fefe, 0xaaaaaaaaaaaaaaab, 0x000000000002fe7d
  TEST_RR_OP 31,  mulhu, 0x000000000001fefe, 0x000000000002fe7d, 0xaaaaaaaaaaaaaaab

  # Source/Destination tests

  TEST_RR_SRC1_EQ_DEST 8, mulhu, 143, 13<<32, 11<<32
  TEST_RR_SRC2_EQ_DEST 9, mulhu, 154, 14<<32, 11<<32
  TEST_RR_SRC12_EQ_DEST 10, mulhu, 169, 13<<32

  # Bypassing tests

  TEST_RR_DEST_BYPASS 11, 0, mulhu, 143, 13<<32, 11<<32
  TEST_RR_DEST_BYPASS 12, 1, mulhu, 154, 14<<32, 11<<32
  TEST_RR_DEST_BYPASS 13, 2, mulhu, 165, 15<<32, 11<<32

  TEST_RR_SRC12_BYPASS 14, 0, 0, mulhu, 143, 13<<32, 11<<32
  TEST_RR_SRC12_BYPASS 15, 0, 1, mulhu, 154, 14<<32, 11<<32
  TEST_RR_SRC12_BYPASS 16, 0, 2, mulhu, 165, 15<<32, 11<<32
  TEST_RR_SRC12_BYPASS 17, 1, 0, mulhu, 143, 13<<32, 11<<32
  TEST_RR_SRC12_BYPASS 18, 1, 1, mulhu, 154, 14<<32, 11<<32
  TEST_RR_SRC12_BYPASS 19, 2, 0, mulhu, 165, 15<<32, 11<<32

  TEST_RR_SRC21_BYPASS 20, 0, 0, mulhu, 143, 13<<32, 11<<32
  TEST_RR_SRC21_BYPASS 21, 0, 1, mulhu, 154, 14<<32, 11<<32
  TEST_RR_SRC21_BYPASS 22, 0, 2, mulhu, 165, 15<<32, 11<<32
  TEST_RR_SRC21_BYPASS 23, 1, 0, mulhu, 143, 13<<32, 11<<32
  TEST_RR_SRC21_BYPASS 24, 1, 1, mulhu, 154, 14<<32, 11<<32
  TEST_RR_SRC21_BYPASS 25, 2, 0, mulhu, 165, 15<<32, 11<<32

  TEST_RR_ZEROSRC1 26, mulhu, 0, 31<<32
  TEST_RR_ZEROSRC2 27, mulhu, 0, 32<<32
  TEST_RR_ZEROSRC12 28, mulhu, 0
  TEST_RR_ZERODEST 29, mulhu, 33<<32, 34<<32

EXIT
