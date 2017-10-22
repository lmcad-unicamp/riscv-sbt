.include "test.s"

/* Make sure AMOs do not corrupt zero */

START

    li        a0, 0xFFFFFFFF00000000
    li        a1, 0x00000000FFFFFFFF
    la        a3, scratch
    sd        a0, 0(a3)
    amoadd.d  x0, a1, 0(a3)
      TEST 2, x0, 0x0

    ld        a5, 0(a3)
      TEST 3, a5, 0xFFFFFFFFFFFFFFFF

    lr.d      x0, 0(a3)
      TEST 4, x0, 0x0

    sc.d      x0, a1, 0(a3)
      TEST 5, x0, 0x0

EXIT

.data

scratch: .dword 0
