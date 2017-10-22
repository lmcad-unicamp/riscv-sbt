.include "test.s"

START

	la	sp, data
	.option rvc
	c.lwsp	a0, 12(sp)
	.option norvc
	TEST 60, a0, 0x11223344

	.option rvc
	c.lwsp	a0, 16(sp)
	.option norvc
	TEST 61, a0, 0xffffffff81223344

EXIT

data:
	.word 0x01000000
	.word 0x02000000
	.word 0x03000000
	.word 0x11223344
	.word 0x81223344
