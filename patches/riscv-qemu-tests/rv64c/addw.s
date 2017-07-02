.include "test.s"

START
	li	s1, 0x7fffffff
	li	a0, 1
	.option rvc
	c.addw	s1, a0
	.option norvc
	TEST 20, s1, 0xffffffff80000000

EXIT
