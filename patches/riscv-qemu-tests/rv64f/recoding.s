.include "test.s"

START

  # Make sure infinities with different mantissas compare as equal.
  flw f0, minf, a0
  flw f1, three, a0
  fmul.s f1, f1, f0

  feq.s a0, f0, f1
  TEST 2, a0, 1

  fle.s a0, f0, f1
  TEST_CASE 3, a0, 1

  flt.s a0, f0, f1
  TEST_CASE 4, a0, 0

  # Likewise, but for zeroes.
  fcvt.s.w f0, x0
  li a0, 1
  fcvt.s.w f1, a0
  fmul.s f1, f1, f0

  feq.s a0, f0, f1
  TEST_CASE 5, a0, 1

  fle.s a0, f0, f1
  TEST_CASE 6, a0, 1

  flt.s a0, f0, f1
  TEST_CASE 7, a0, 0

EXIT

.data

minf: .float -Inf
three: .float 3.0
