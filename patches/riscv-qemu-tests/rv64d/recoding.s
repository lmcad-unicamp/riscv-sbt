.include "test.s"

START

  # Make sure infinities with different mantissas compare as equal.
  fld f0, minf, a0
  fld f1, three, a0
  fmul.d f1, f1, f0
  feq.d a0, f0, f1
  TEST 2, a0, 1
  fle.d a0, f0, f1
  TEST 3, a0, 1
  flt.d a0, f0, f1
  TEST 4, a0, 0

  # Likewise, but for zeroes.
  fcvt.d.w f0, x0
  li a0, 1
  fcvt.d.w f1, a0
  fmul.d f1, f1, f0
  feq.d a0, f0, f1
  TEST 5, a0, 1
  fle.d a0, f0, f1
  TEST 6, a0, 1
  flt.d a0, f0, f1
  TEST 7, a0, 0

  # When converting small doubles to single-precision subnormals,
  # ensure that the extra precision is discarded.
  flw f0, big, a0
  fld f1, tiny, a0
  fcvt.s.d f1, f1
  fmul.s f0, f0, f1
  fmv.x.s a0, f0
  lw a1, small
  sub a0, a0, a1
  TEST 10, a0, 0

  # Make sure FSD+FLD correctly saves and restores a single-precision value.
  flw f0, three, a0
  fadd.s f1, f0, f0
  fadd.s f0, f0, f0
  fsd f0, tiny, a0
  fld f0, tiny, a0
  feq.s a0, f0, f1
  TEST 20, a0, 1

EXIT

.data

minf: .double -Inf
three: .double 3.0
big: .float 1221
small: .float 2.9133121e-37
tiny: .double 2.3860049081905093e-40
