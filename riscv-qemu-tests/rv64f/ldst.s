.include "test.s"

START

    la a1, tdat
    flw f1, 4(a1)
    fsw f1, 20(a1)
    ld a0, 16(a1)
  TEST 2, a0, 0x40000000deadbeef

    la a1, tdat
    flw f1, 0(a1)
    fsw f1, 24(a1)
    ld a0, 24(a1)
  TEST 3, a0, 0x1337d00dbf800000

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
