.include "test.s"

START

  # Basic tests

  TEST_ST_OP 2, lb, sb, 0xffffffaa, 0, tdat
  TEST_ST_OP 3, lb, sb, 0x00000000, 1, tdat
  TEST_ST_OP 4, lh, sb, 0xffffefa0, 2, tdat
  TEST_ST_OP 5, lb, sb, 0x0000000a, 3, tdat

  # Test with negative offset

  TEST_ST_OP 6, lb, sb, 0xffffffaa, -3, tdat8
  TEST_ST_OP 7, lb, sb, 0x00000000, -2, tdat8
  TEST_ST_OP 8, lb, sb, 0xffffffa0, -1, tdat8
  TEST_ST_OP 9, lb, sb, 0x0000000a, 0,  tdat8

  # Test with a negative base

    la  x1, tdat9
    li  x2, 0x12345678
    addi x4, x1, -32
    sb x2, 32(x4)
    lb x3, 0(x1)
  TEST_CASE 10, x3, 0x78

  # Test with unaligned base

    la  x1, tdat9
    li  x2, 0x00003098
    addi x1, x1, -6
    sb x2, 7(x1)
    la  x4, tdat10
    lb x3, 0(x4)
  TEST_CASE 11, x3, 0xffffff98

EXIT


.data

tdat:
tdat1:  .byte 0xef
tdat2:  .byte 0xef
tdat3:  .byte 0xef
tdat4:  .byte 0xef
tdat5:  .byte 0xef
tdat6:  .byte 0xef
tdat7:  .byte 0xef
tdat8:  .byte 0xef
tdat9:  .byte 0xef
tdat10: .byte 0xef
