.include "test.s"

.macro TEST_LUI T res num shift
	lui	s0, \num
	sra	s0, s0, \shift
	li	s1, \res
	li	gp, \T
	bne	s0, s1, fail
.endm

START

  TEST_LUI 2, 0x0000000000000000, 0x00000, 0
  TEST_LUI 3, 0xfffffffffffff800, 0xfffff, 1
  TEST_LUI 4, 0x00000000000007ff, 0x7ffff, 20
  TEST_LUI 5, 0xfffffffffffff800, 0x80000, 20
  #TEST_LUI 6, 0x0000000000000000, 0x80000, 0 ???

  TEST_LUI 11, 0x0000000012345000, 0x12345, 0
  TEST_LUI 12, 0xFFFFFFFF82345000, 0x82345, 0

EXIT

# vim: ft=asm
