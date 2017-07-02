.include "test.s"

START
        .option rvc
        li      gp, 2
        li      a0, 1
        c.beqz  a0, fail

        li      gp, 3
        li      a0, 0
        c.beqz  a0, 1f
	j	fail
1:

EXIT
