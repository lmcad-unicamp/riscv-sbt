.include "test.s"

START

	li	sp, 0x11223344
	.option rvc
	c.addi16sp sp, 13*16
	.option norvc
	TEST 2, sp, (0x11223344 + 13*16)

	li	sp, 0x11223344
	.option rvc
	c.addi16sp sp, -7*16
	.option norvc
	TEST 3, sp, (0x11223344 - 7*16)

EXIT
