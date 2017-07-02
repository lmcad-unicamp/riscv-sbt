.include "test.s"

START

  TEST_INT_FP_OP_D 2,  fcvt.d.w,                   2.0,  2
  TEST_INT_FP_OP_D 3,  fcvt.d.w,                  -2.0, -2

  TEST_INT_FP_OP_D 4, fcvt.d.wu,                   2.0,  2
  TEST_INT_FP_OP_D 5, fcvt.d.wu,           4294967294.0, -2

  TEST_INT_FP_OP_D 6,  fcvt.d.l,                   2.0,  2
  TEST_INT_FP_OP_D 7,  fcvt.d.l,                  -2.0, -2

  TEST_INT_FP_OP_D 8, fcvt.d.lu,                   2.0,  2
  TEST_INT_FP_OP_D 9, fcvt.d.lu, 1.8446744073709552e19, -2

  TEST_FCVT_S_D 10, -1.5, -1.5
  TEST_FCVT_D_S 11, -1.5, -1.5

    la a1, test_data_22
    ld a2, 0(a1)
    fmv.d.x f2, a2
    fcvt.s.d f2, f2
    fcvt.d.s f2, f2
    fmv.x.d a0, f2
  TEST 12, a0, 0x7ff8000000000000

EXIT

.data

test_data_22: .dword 0x7ffcffffffff8004
