.include "test.s"

# Only tests that apply to linux-user.
# No multi-threading (or multi-core, whatever it is) for now.

START

# Make sure that sc without a reservation fails.
    la	  a0, dat
    sc.d  a4, x0, (a0)
  TEST 2, a4, 1

# Make sure that sc with the wrong reservation fails.
# TODO is this actually mandatory behavior?
    la    a0, dat
    add   a1, a0, 100
    lr.d  a1, (a1)
    sc.d  a4, a1, (a0)
   TEST 3, a4, 1

# Try the same, with a proper reservation now
    la    a0, dat
    lr.d  a1, (a0)
    li    a2, 0x0102030405060708
    sc.d  a4, a2, (a0)
   TEST 4, a4, 0
    ld    a1, (a0)
   TEST 5, a1, 0x0102030405060708

# Check the guard value, just in case
    ld    a1, 8(a0)
   TEST 6, a1, 0x1020304050607080

EXIT

.data

dat: .quad 0x1122334455667788
     .quad 0x1020304050607080
     .fill 100, 4, 0
