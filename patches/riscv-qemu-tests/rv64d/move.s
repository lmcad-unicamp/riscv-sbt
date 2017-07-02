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

  li a1, 0x3FF02468A0000000
  fmv.d.x f1, a1
  fmv.x.d a0, f1
  TEST 5, a0, 0x3FF02468A0000000

  li a1, 0x3FF02468A0001000
  li a2, -1
  fmv.d.x f1, a1
  fmv.d.x f2, a2
  fsgnj.d f0, f1, f2
  fmv.x.d a0, f0
  TEST 6, a0, 0xBFF02468A0001000

EXIT
