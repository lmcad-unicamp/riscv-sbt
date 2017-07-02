.include "test.s"

.macro TEST_FCLASS_D T res val
	li	a0, \val
	fmv.d.x fa0, a0
	fclass.d a0, fa0
	li	a1, \res
	li	gp, \T
	bne	a0, a1, fail
.endm

START

  TEST_FCLASS_D  2, 1 << 0, 0xfff0000000000000
  TEST_FCLASS_D  3, 1 << 1, 0xbff0000000000000
  TEST_FCLASS_D  4, 1 << 2, 0x800fffffffffffff
  TEST_FCLASS_D  5, 1 << 3, 0x8000000000000000
  TEST_FCLASS_D  6, 1 << 4, 0x0000000000000000
  TEST_FCLASS_D  7, 1 << 5, 0x000fffffffffffff
  TEST_FCLASS_D  8, 1 << 6, 0x3ff0000000000000
  TEST_FCLASS_D  9, 1 << 7, 0x7ff0000000000000
  TEST_FCLASS_D 10, 1 << 8, 0x7ff0000000000001
  TEST_FCLASS_D 11, 1 << 9, 0x7ff8000000000000

EXIT

# vim: ft=asm
