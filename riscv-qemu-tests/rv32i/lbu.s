.include "test.s"

START

  # Basic tests

  TEST_LD_OP 2, lbu, 0x000000ff, 0, tdat
  TEST_LD_OP 3, lbu, 0x00000000, 1, tdat
  TEST_LD_OP 4, lbu, 0x000000f0, 2, tdat
  TEST_LD_OP 5, lbu, 0x0000000f, 3, tdat

  # Test with negative offset

  TEST_LD_OP 6, lbu, 0x000000ff, -3, tdat4
  TEST_LD_OP 7, lbu, 0x00000000, -2, tdat4
  TEST_LD_OP 8, lbu, 0x000000f0, -1, tdat4
  TEST_LD_OP 9, lbu, 0x0000000f,  0, tdat4

  # Test with a negative base

    lsym  x1, tdat
    addi x1, x1, -32
    lbu x3, 32(x1)
  TEST_CASE 10, x3, 0x000000ff

  # Test with unaligned base

    lsym  x1, tdat
    addi x1, x1, -6
    lbu x3, 7(x1)
  TEST_CASE 11, x3, 0x00000000

  # Bypassing tests

  TEST_LD_DEST_BYPASS 12, 0, lbu, 0x000000f0, 1, tdat2
  TEST_LD_DEST_BYPASS 13, 1, lbu, 0x0000000f, 1, tdat3
  TEST_LD_DEST_BYPASS 14, 2, lbu, 0x00000000, 1, tdat1

  TEST_LD_SRC1_BYPASS 15, 0, lbu, 0x000000f0, 1, tdat2
  TEST_LD_SRC1_BYPASS 16, 1, lbu, 0x0000000f, 1, tdat3
  TEST_LD_SRC1_BYPASS 17, 2, lbu, 0x00000000, 1, tdat1

  # Test write-after-write hazard

    lsym  x3, tdat
    lbu  x2, 0(x3)
    li  x2, 2
  TEST_CASE 18, x2, 2

    lsym  x3, tdat
    lbu  x2, 0(x3)
    nop
    li  x2, 2
  TEST_CASE 19, x2, 2

EXIT

.data

tdat:
tdat1:  .byte 0xff
tdat2:  .byte 0x00
tdat3:  .byte 0xf0
tdat4:  .byte 0x0f
