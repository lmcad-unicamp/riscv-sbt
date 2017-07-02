.include "test.s"

START
	li	s1, 0x7fffffff
	li	a0, -1
	.option rvc
	c.subw	s1, a0
	.option norvc
	TEST 19, s1, 0xffffffff80000000

EXIT
