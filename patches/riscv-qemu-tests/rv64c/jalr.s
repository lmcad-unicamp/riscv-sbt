.include "test.s"

START
.option rvc

	la	t0, 1f
	li	gp, 2
	li	ra, 0
	c.jalr	t0
3:	c.j	fail
1:	c.j	2f
	c.j	fail

2:	
.option norvc
	li	gp, 3
	la	t1, 3b
	bne	t1, ra, fail

EXIT
