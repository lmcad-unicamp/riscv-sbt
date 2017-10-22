.include "test.s"

START
	li	s1, 20
	li	a0, 6
	.option rvc
	c.xor	s1, a0
	.option norvc
	TEST 16, s1, 18
EXIT
