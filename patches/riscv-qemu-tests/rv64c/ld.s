.include "test.s"

START
  	la	a1, data
	.option rvc
	c.ld	a0, 8(a1)
	.option norvc
	TEST 2, a0, 0x8100230176543210

EXIT

data:
	.quad 0x0102030405060708
	.quad 0x8100230176543210
