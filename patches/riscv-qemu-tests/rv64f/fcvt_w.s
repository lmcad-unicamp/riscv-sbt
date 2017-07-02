.include "test.s"

.macro TEST_FCVT_DATA_S T res off OP
	la	x1, tdat
	flw	f1, \off(x1)
	\OP	x1, f1
	li	x2, \res
	li	gp, \T
	bne	x1, x2, fail
.endm

START

  TEST_FP_INT_OP_S  2,  fcvt.w.s, 0x01,         -1, -1.1, rtz
  TEST_FP_INT_OP_S  3,  fcvt.w.s, 0x00,         -1, -1.0, rtz
  TEST_FP_INT_OP_S  4,  fcvt.w.s, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_S  5,  fcvt.w.s, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_S  6,  fcvt.w.s, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_S  7,  fcvt.w.s, 0x01,          1,  1.1, rtz
  TEST_FP_INT_OP_S  8,  fcvt.w.s, 0x10, 0xffffffff80000000, -3e9, rtz
  TEST_FP_INT_OP_S  9,  fcvt.w.s, 0x10, 0x000000007FFFFFFF,  3e9, rtz

  TEST_FP_INT_OP_S 12, fcvt.wu.s, 0x10,          0, -3.0, rtz
  TEST_FP_INT_OP_S 13, fcvt.wu.s, 0x10,          0, -1.0, rtz
  TEST_FP_INT_OP_S 14, fcvt.wu.s, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_S 15, fcvt.wu.s, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_S 16, fcvt.wu.s, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_S 17, fcvt.wu.s, 0x01,          1,  1.1, rtz
  TEST_FP_INT_OP_S 18, fcvt.wu.s, 0x10,          0, -3e9, rtz
  #TEST_FP_INT_OP_S 19, fcvt.wu.s, 0x00, 3000000000,  3e9, rtz # FIXME

  TEST_FP_INT_OP_S 22,  fcvt.l.s, 0x01,         -1, -1.1, rtz
  TEST_FP_INT_OP_S 23,  fcvt.l.s, 0x00,         -1, -1.0, rtz
  TEST_FP_INT_OP_S 24,  fcvt.l.s, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_S 25,  fcvt.l.s, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_S 26,  fcvt.l.s, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_S 27,  fcvt.l.s, 0x01,          1,  1.1, rtz

  TEST_FP_INT_OP_S 32, fcvt.lu.s, 0x10,          0, -3.0, rtz
  TEST_FP_INT_OP_S 33, fcvt.lu.s, 0x10,          0, -1.0, rtz
  TEST_FP_INT_OP_S 34, fcvt.lu.s, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_S 35, fcvt.lu.s, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_S 36, fcvt.lu.s, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_S 37, fcvt.lu.s, 0x01,          1,  1.1, rtz
  TEST_FP_INT_OP_S 38, fcvt.lu.s, 0x10,          0, -3e9, rtz

  # test negative NaN, negative infinity conversion
  TEST_FCVT_DATA_S 42, 0x000000007fffffff, 0, fcvt.w.s
  TEST_FCVT_DATA_S 43, 0x7fffffffffffffff, 0, fcvt.l.s
  TEST_FCVT_DATA_S 44, 0xffffffff80000000, 8, fcvt.w.s
  TEST_FCVT_DATA_S 45, 0x8000000000000000, 8, fcvt.l.s

  # test positive NaN, positive infinity conversion
  TEST_FCVT_DATA_S 52, 0x000000007fffffff,  4, fcvt.w.s
  TEST_FCVT_DATA_S 53, 0x7fffffffffffffff,  4, fcvt.l.s
  TEST_FCVT_DATA_S 54, 0x000000007fffffff, 12, fcvt.w.s
  TEST_FCVT_DATA_S 55, 0x7fffffffffffffff, 12, fcvt.l.s

  ## test NaN, infinity conversions to unsigned integer
  TEST_FCVT_DATA_S 62, 0xffffffffffffffff,  0, fcvt.wu.s
  TEST_FCVT_DATA_S 63, 0xffffffffffffffff,  4, fcvt.wu.s
  TEST_FCVT_DATA_S 64,                  0,  8, fcvt.wu.s
  TEST_FCVT_DATA_S 65, 0xffffffffffffffff, 12, fcvt.wu.s
  TEST_FCVT_DATA_S 66, 0xffffffffffffffff,  0, fcvt.lu.s
  TEST_FCVT_DATA_S 67, 0xffffffffffffffff,  4, fcvt.lu.s
  TEST_FCVT_DATA_S 68,                  0,  8, fcvt.lu.s
  TEST_FCVT_DATA_S 69, 0xffffffffffffffff, 12, fcvt.lu.s
   
EXIT

# -NaN, NaN, -inf, +inf
tdat:
.word 0xffffffff
.word 0x7fffffff
.word 0xff800000
.word 0x7f800000

# vim: ft=asm
