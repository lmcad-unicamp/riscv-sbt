.include "test.s"

START

    la        a3, scratch

    li        a0, 0xffffffff80000000
    li        a1, 0xfffffffffffff800
    sw        a0, 0(a3)
    amominu.w a4, a1, 0(a3)
      TEST 2, a4, 0xffffffff80000000

    lw        a5, 0(a3)
      TEST 3, a5, 0xffffffff80000000

    li        a0, 0x0000000070000000
    li        a1, 0x000000007ffff800
    sw        a0, 0(a3)
    amominu.w a4, a1, 0(a3)
      TEST 4, a4, 0x0000000070000000

    lw        a5, 0(a3)
      TEST 5, a5, 0x0000000070000000

EXIT

.data

scratch: .word 0
