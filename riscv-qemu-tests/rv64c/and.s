.include "test.s"

START
	li	s1, 20
	li	a0, 6
	.option rvc
	c.and	s1, a0
	.option norvc
	TEST 18, s1, 4
EXIT
