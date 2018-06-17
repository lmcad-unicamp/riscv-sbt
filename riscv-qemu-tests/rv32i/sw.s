.include "test.s"

START

  # Basic tests

  TEST_ST_OP 2, lw, sw, 0x00aa00aa, 0,  tdat
  TEST_ST_OP 3, lw, sw, 0xaa00aa00, 4,  tdat
  TEST_ST_OP 4, lw, sw, 0x0aa00aa0, 8,  tdat
  TEST_ST_OP 5, lw, sw, 0xa00aa00a, 12, tdat

  # Test with negative offset

  TEST_ST_OP 6, lw, sw, 0x00aa00aa, -12, tdat8
  TEST_ST_OP 7, lw, sw, 0xaa00aa00, -8,  tdat8
  TEST_ST_OP 8, lw, sw, 0x0aa00aa0, -4,  tdat8
  TEST_ST_OP 9, lw, sw, 0xa00aa00a, 0,   tdat8

  # Test with a negative base

    lsym  x1, tdat9
    li  x2, 0x12345678
    addi x4, x1, -32
    sw x2, 32(x4)
    lw x3, 0(x1)
  TEST_CASE 10, x3, 0x12345678

  # Test with unaligned base

    lsym  x1, tdat9
    li  x2, 0x58213098
    addi x1, x1, -3
    sw x2, 7(x1)
    lsym  x4, tdat10
    lw x3, 0(x4)
  TEST_CASE 11, x3, 0x58213098

  # Bypassing tests

  TEST_ST_SRC12_BYPASS 12, 0, 0, lw, sw, 0xaabbccdd, 0,  tdat
  TEST_ST_SRC12_BYPASS 13, 0, 1, lw, sw, 0xdaabbccd, 4,  tdat
  TEST_ST_SRC12_BYPASS 14, 0, 2, lw, sw, 0xddaabbcc, 8,  tdat
  TEST_ST_SRC12_BYPASS 15, 1, 0, lw, sw, 0xcddaabbc, 12, tdat
  TEST_ST_SRC12_BYPASS 16, 1, 1, lw, sw, 0xccddaabb, 16, tdat
  TEST_ST_SRC12_BYPASS 17, 2, 0, lw, sw, 0xbccddaab, 20, tdat

  TEST_ST_SRC21_BYPASS 18, 0, 0, lw, sw, 0x00112233, 0,  tdat
  TEST_ST_SRC21_BYPASS 19, 0, 1, lw, sw, 0x30011223, 4,  tdat
  TEST_ST_SRC21_BYPASS 20, 0, 2, lw, sw, 0x33001122, 8,  tdat
  TEST_ST_SRC21_BYPASS 21, 1, 0, lw, sw, 0x23300112, 12, tdat
  TEST_ST_SRC21_BYPASS 22, 1, 1, lw, sw, 0x22330011, 16, tdat
  TEST_ST_SRC21_BYPASS 23, 2, 0, lw, sw, 0x12233001, 20, tdat

EXIT

.data

tdat:
tdat1:  .int 0xdeadbeef
tdat2:  .int 0xdeadbeef
tdat3:  .int 0xdeadbeef
tdat4:  .int 0xdeadbeef
tdat5:  .int 0xdeadbeef
tdat6:  .int 0xdeadbeef
tdat7:  .int 0xdeadbeef
tdat8:  .int 0xdeadbeef
tdat9:  .int 0xdeadbeef
tdat10: .int 0xdeadbeef
