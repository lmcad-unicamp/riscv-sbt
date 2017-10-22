.include "test.s"

START
	li	s1, 20
	li	a0, 6
	.option rvc
	c.or	s1, a0
	.option norvc
	TEST 17, s1, 22
EXIT
