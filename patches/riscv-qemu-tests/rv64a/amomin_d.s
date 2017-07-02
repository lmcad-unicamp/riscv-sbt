.include "test.s"

START

    li        a0, 0xffffffff80000000
    li        a1, 0xfffffffffffff800
    la        a3, scratch
    sd        a0, 0(a3)
    amomin.d  a4, a1, 0(a3)
      TEST 2, a4, 0xffffffff80000000

    ld        a5, 0(a3)
      TEST 3, a5, 0xffffffff80000000

EXIT

.data

scratch: .dword 0
