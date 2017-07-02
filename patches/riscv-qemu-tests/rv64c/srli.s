.include "test.s"

START
	.option rvc
	c.lui	s0, 0xfffe1
	c.srli	s0, 12
	.option norvc

	TEST 11, s0, 0x000fffffffffffe1
EXIT
