.include "test.s"

START
.option rvc

	la	t0, 1f
	li	gp, 2
	li	ra, 0
	c.jr	t0
	c.j	fail
1:	c.j	2f
	c.j	fail
2:
.option norvc
EXIT
