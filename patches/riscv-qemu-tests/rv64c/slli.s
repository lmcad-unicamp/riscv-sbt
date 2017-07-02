.include "test.s"

START
	li	s0, 0x1234
	.option rvc
	c.slli	s0, 4
	.option norvc
	TEST 21, s0, 0x12340
EXIT
