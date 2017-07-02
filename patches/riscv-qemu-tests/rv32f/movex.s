.include "test.s"

START

	la        a0, data
	lw        a1, 0(a0)
	lw        a2, 4(a0)
	lw        a3, 8(a0)
	fmv.s.x   f1, a1
	fmv.s.x   f2, a2
	fsub.s    f3, f1, f2
	fmv.x.s   a4, f3
	li        gp, 9
	FAILIF bne       a3, a4

EXIT

data:	.float 3.5
	.float 2.5
	.float 1.0
