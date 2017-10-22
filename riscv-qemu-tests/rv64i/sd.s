.include "test.s"

START

  # Basic tests

  TEST_ST_OP 2, ld, sd, 0x00aa00aa00aa00aa, 0,  tdat
  TEST_ST_OP 3, ld, sd, 0xaa00aa00aa00aa00, 8,  tdat
  TEST_ST_OP 4, ld, sd, 0x0aa00aa00aa00aa0, 16,  tdat
  TEST_ST_OP 5, ld, sd, 0xa00aa00aa00aa00a, 24, tdat

  # Test with negative offset

  TEST_ST_OP 6, ld, sd, 0x00aa00aa00aa00aa, -24, tdat8
  TEST_ST_OP 7, ld, sd, 0xaa00aa00aa00aa00, -16, tdat8
  TEST_ST_OP 8, ld, sd, 0x0aa00aa00aa00aa0, -8,  tdat8
  TEST_ST_OP 9, ld, sd, 0xa00aa00aa00aa00a, 0,   tdat8

  # Test with a negative base

    la  x1, tdat9
    li  x2, 0x1234567812345678
    addi x4, x1, -32
    sd x2, 32(x4)
    ld x3, 0(x1)
  TEST_CASE 10, x3, 0x1234567812345678

  # Test with unaligned base

    la  x1, tdat9
    li  x2, 0x5821309858213098
    addi x1, x1, -3
    sd x2, 11(x1)
    la  x4, tdat10
    ld x3, 0(x4)
  TEST_CASE 11, x3, 0x5821309858213098

EXIT


.data

tdat:
tdat1:  .dword 0xdeadbeefdeadbeef
tdat2:  .dword 0xdeadbeefdeadbeef
tdat3:  .dword 0xdeadbeefdeadbeef
tdat4:  .dword 0xdeadbeefdeadbeef
tdat5:  .dword 0xdeadbeefdeadbeef
tdat6:  .dword 0xdeadbeefdeadbeef
tdat7:  .dword 0xdeadbeefdeadbeef
tdat8:  .dword 0xdeadbeefdeadbeef
tdat9:  .dword 0xdeadbeefdeadbeef
tdat10: .dword 0xdeadbeefdeadbeef
