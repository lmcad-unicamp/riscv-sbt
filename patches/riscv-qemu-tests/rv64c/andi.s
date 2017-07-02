.include "test.s"

# RVC_TEST_CASE (14, s0, ~0x11, c.li s0, -2; c.andi s0, ~0x10)

START
	li	s0, -2
	.option rvc
	c.andi	s0, ~0x10
	.option norvc
	TEST 14, s0, ~0x11
EXIT
