.include "test.s"

# Only tests that apply to linux-user.
# No multi-threading (or multi-core, whatever it is) for now.

START

# Make sure that sc without a reservation fails.
    la	  a0, dat
    sc.w  a4, x0, (a0)
  TEST 2, a4, 1

# Make sure that sc with the wrong reservation fails.
# TODO is this actually mandatory behavior?
    la    a0, dat
    add   a1, a0, 100
    lr.w  a1, (a1)
    sc.w  a4, a1, (a0)
   TEST 3, a4, 1

# Try the same, with a proper reservation now
    la    a0, dat
    lr.w  a1, (a0)
    li    a2, 0x01020304
    sc.w  a4, a2, (a0)
   TEST 4, a4, 0
    lw    a1, (a0)
   TEST 5, a1, 0x01020304

# Check the guard value, just in case
    la    a0, dat
    ld    a1, 4(a0)
   TEST 6, a1, 0x1020304050607080

EXIT

.data

dat: .word 0x11223344
     .quad 0x1020304050607080
     .fill 100, 4, 0
