.include "test.s"

#  RVC_TEST_CASE (41, a2, 0xfedcba9976543211, c.ldsp a0, 8(sp); addi a0, a0, 1; c.sdsp a0, 8(sp); c.ldsp a2, 8(sp))

START

	la	sp, data
	.option rvc
	c.ldsp	a0, 16(sp)
	.option norvc
	TEST 62, a0, 0x1122334455667788

	.option rvc
	c.ldsp	a0, 24(sp)
	.option norvc
	TEST 63, a0, 0x8122334455667788

EXIT

data:
	.quad 0x1000000010000000
	.quad 0x2000000020000000
	.quad 0x1122334455667788
	.quad 0x8122334455667788
.type data, object
