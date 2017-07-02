.include "test.s"

.macro TEST_FCVT_DATA_D T res off OP
	la	x1, tdat_d
	fld	f1, \off(x1)
	\OP	x1, f1
	li	x2, \res
	li	gp, \T
	bne	x1, x2, fail
.endm

START

  TEST_FP_INT_OP_D  2,  fcvt.w.d, 0x01,         -1, -1.1, rtz
  TEST_FP_INT_OP_D  3,  fcvt.w.d, 0x00,         -1, -1.0, rtz
  TEST_FP_INT_OP_D  4,  fcvt.w.d, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_D  5,  fcvt.w.d, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_D  6,  fcvt.w.d, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_D  7,  fcvt.w.d, 0x01,          1,  1.1, rtz
  TEST_FP_INT_OP_D  8,  fcvt.w.d, 0x10, 0xffffffff80000000, -3e9, rtz
  TEST_FP_INT_OP_D  9,  fcvt.w.d, 0x10, 0x000000007FFFFFFF,  3e9, rtz

  TEST_FP_INT_OP_D 12, fcvt.wu.d, 0x10,          0, -3.0, rtz
  TEST_FP_INT_OP_D 13, fcvt.wu.d, 0x10,          0, -1.0, rtz
  TEST_FP_INT_OP_D 14, fcvt.wu.d, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_D 15, fcvt.wu.d, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_D 16, fcvt.wu.d, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_D 17, fcvt.wu.d, 0x01,          1,  1.1, rtz
  TEST_FP_INT_OP_D 18, fcvt.wu.d, 0x10,          0, -3e9, rtz
  TEST_FP_INT_OP_D 19, fcvt.wu.d, 0x00, 0xffffffffb2d05e00, 3e9, rtz

  TEST_FP_INT_OP_D 22,  fcvt.l.d, 0x01,         -1, -1.1, rtz
  TEST_FP_INT_OP_D 23,  fcvt.l.d, 0x00,         -1, -1.0, rtz
  TEST_FP_INT_OP_D 24,  fcvt.l.d, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_D 25,  fcvt.l.d, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_D 26,  fcvt.l.d, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_D 27,  fcvt.l.d, 0x01,          1,  1.1, rtz
  TEST_FP_INT_OP_D 28,  fcvt.l.d, 0x00,-3000000000, -3e9, rtz
  TEST_FP_INT_OP_D 29,  fcvt.l.d, 0x00, 3000000000,  3e9, rtz
  TEST_FP_INT_OP_D 20,  fcvt.l.d, 0x10, 0x8000000000000000,-3e19, rtz
  TEST_FP_INT_OP_D 21,  fcvt.l.d, 0x10, 0x7fffffffffffffff, 3e19, rtz

  TEST_FP_INT_OP_D 32, fcvt.lu.d, 0x10,          0, -3.0, rtz
  TEST_FP_INT_OP_D 33, fcvt.lu.d, 0x10,          0, -1.0, rtz
  TEST_FP_INT_OP_D 34, fcvt.lu.d, 0x01,          0, -0.9, rtz
  TEST_FP_INT_OP_D 35, fcvt.lu.d, 0x01,          0,  0.9, rtz
  TEST_FP_INT_OP_D 36, fcvt.lu.d, 0x00,          1,  1.0, rtz
  TEST_FP_INT_OP_D 37, fcvt.lu.d, 0x01,          1,  1.1, rtz
  TEST_FP_INT_OP_D 38, fcvt.lu.d, 0x10,          0, -3e9, rtz
  TEST_FP_INT_OP_D 39, fcvt.lu.d, 0x00, 3000000000,  3e9, rtz

  # test negative NaN, negative infinity conversion
  TEST_FCVT_DATA_D 42, 0x000000007fffffff,  0, fcvt.w.d
  TEST_FCVT_DATA_D 43, 0x7fffffffffffffff,  0, fcvt.l.d
  TEST_FCVT_DATA_D 44, 0xffffffff80000000, 16, fcvt.w.d
  TEST_FCVT_DATA_D 45, 0x8000000000000000, 16, fcvt.l.d

  # test positive NaN, positive infinity conversion
  TEST_FCVT_DATA_D 52, 0x000000007fffffff,  8, fcvt.w.d
  TEST_FCVT_DATA_D 53, 0x7fffffffffffffff,  8, fcvt.l.d
  TEST_FCVT_DATA_D 54, 0x000000007fffffff, 24, fcvt.w.d
  TEST_FCVT_DATA_D 55, 0x7fffffffffffffff, 24, fcvt.l.d

  # test NaN, infinity conversions to unsigned integer
  TEST_FCVT_DATA_D 62, 0xffffffffffffffff,  0, fcvt.wu.d
  TEST_FCVT_DATA_D 63, 0xffffffffffffffff,  8, fcvt.wu.d
  TEST_FCVT_DATA_D 64,                  0, 16, fcvt.wu.d
  TEST_FCVT_DATA_D 65, 0xffffffffffffffff, 24, fcvt.wu.d
  TEST_FCVT_DATA_D 66, 0xffffffffffffffff,  0, fcvt.lu.d
  TEST_FCVT_DATA_D 67, 0xffffffffffffffff,  8, fcvt.lu.d
  TEST_FCVT_DATA_D 68,                  0, 16, fcvt.lu.d
  TEST_FCVT_DATA_D 69, 0xffffffffffffffff, 24, fcvt.lu.d

EXIT

# -NaN, NaN, -inf, +inf
tdat_d:
.dword 0xffffffffffffffff
.dword 0x7fffffffffffffff
.dword 0xfff0000000000000
.dword 0x7ff0000000000000

# vim: ft=asm
