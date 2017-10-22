.include "test.s"

START
  	la	a1, data
	.option rvc
	c.lw	a0, 8(a1)
	.option norvc
	TEST 2, a0, 0xffffffff81002301

EXIT

data:
	.word 0x01020304
	.word 0x05060708
	.quad 0x81002301
	.word 0x76543210
