.include "test.s"

START
	li	s1, 20
	li	a0, 6
	.option rvc
	c.sub	s1, a0
	.option norvc
	TEST 15, s1, 14
EXIT
