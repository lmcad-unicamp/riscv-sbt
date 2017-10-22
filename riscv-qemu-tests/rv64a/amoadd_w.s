.include "test.s"

START

    li        a0, 0xffffffff80000000
    li        a1, 0xfffffffffffff800
    la        a3, scratch
    sw        a0, 0(a3)
    amoadd.w  a4, a1, 0(a3)
      TEST 2, a4, 0xffffffff80000000

    lw        a5, 0(a3)
      TEST 3, a5, 0x000000007ffff800

EXIT

.data

scratch: .word 0
