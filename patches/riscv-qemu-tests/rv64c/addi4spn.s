.include "test.s"

START

	li	sp, 0x11223344
	.option rvc
	c.addi4spn a1, sp, 32
	.option norvc
	TEST 2, a1, (0x11223344 + 32)

EXIT
