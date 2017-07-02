.include "test.s"

START

    .option rvc
	c.li	a1, 15
    .option norvc
	TEST 2, a1, 15

EXIT
