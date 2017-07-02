.include "test.s"

START

     .option rvc
    c.lui a1, 13
     .option norvc
    TEST 2, a1, (13 << 12)

     .option rvc
    c.lui a1, 0xfffe1
     .option norvc
    TEST 3, a1, 0xfffffffffffe1000

EXIT
