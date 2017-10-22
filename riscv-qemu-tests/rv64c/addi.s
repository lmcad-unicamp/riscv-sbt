.include "test.s"

START

	li	a0, 0x1234

	.option rvc
	c.addi	a0, 17
	.option norvc

	TEST 3, a0, 0x1234 + 17

EXIT
