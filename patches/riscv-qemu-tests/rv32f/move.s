.include "test.s"

START

    li a0, 1
    fssr a0

    li a0, 0x1234
    fssr a1, a0
  TEST 2, a1, 1

    frsr a0
  TEST 3, a0, 0x34

    frsr a0
  TEST 4, a0, 0x34

    li a1, 0xBF812345
    fmv.s.x f0, a1
    fmv.x.s a0, f0
  TEST 5, a0, 0xBF812345

    li a1, 0xBF812345
    fmv.s.x f0, a1
    fsgnj.s f1, f0, f0
    fmv.x.s a0, f1
  TEST_CASE 6, a0, 0xBF812345

    li a1, 0xCBA98765
    fmv.s.x f0, a1
    fsgnjx.s f1, f0, f0
    fmv.x.s a0, f1
  TEST_CASE 7, a0, 0x4BA98765

    li a1, 0xDEADBEEF
    fmv.s.x f0, a1
    fsgnjn.s f1, f0, f0
    fmv.x.s a0, f1
  TEST_CASE 8, a0, 0x5EADBEEF

EXIT
