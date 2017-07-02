.include "test.s"

START
        .option rvc
        li      gp, 2
        li      a0, 1
        c.bnez  a0, 1f
        c.j     2f
1:      c.j     3f
2:      j       fail
3:

        li      gp, 3
        li      a0, 0
        c.bnez  a0, fail

EXIT
