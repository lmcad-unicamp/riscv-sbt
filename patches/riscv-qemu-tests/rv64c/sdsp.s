.include "test.s"

START
	la	sp, data
	li	s0, 0x1122334455667788
	.option rvc
	c.sdsp	s0, 8(sp)
	.option norvc
	ld	s1, 8(sp)
	TEST 2, s1, 0x1122334455667788

EXIT

.data

data:	.quad	0
	.quad	0
	.quad	0
