.include "test.s"

START
	la	sp, data
	li	s0, 0x11223344
	.option rvc
	c.swsp	s0, 4(sp)
	.option norvc
	lw	s1, 4(sp)
	TEST 2, s1, 0x11223344

EXIT

.data

data:	.word	0
	.word	0
	.word	0
