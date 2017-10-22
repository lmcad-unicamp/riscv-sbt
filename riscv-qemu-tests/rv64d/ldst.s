.include "test.s"

START

  la a1, tdat
  fld f2, 0(a1)
  fsd f2, 16(a1)
  ld a0, 16(a1)
  TEST 2, a0, 0x40000000bf800000

  la a1, tdat
  fld f2, 8(a1)
  fsd f2, 16(a1)
  ld a0, 16(a1)
  TEST 3, a0, 0xc080000040400000

EXIT

.data

tdat:
.word 0xbf800000
.word 0x40000000
.word 0x40400000
.word 0xc0800000
.word 0xdeadbeef
.word 0xcafebabe
.word 0xabad1dea
.word 0x1337d00d
