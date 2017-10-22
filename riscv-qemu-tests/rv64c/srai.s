.include "test.s"

START
	.option rvc
	c.lui	s0, 0xfffe1
	c.srai	s0, 12
	.option norvc
	TEST 11, s0, 0xffffffffffffffe1
EXIT
