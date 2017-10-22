.include "test.s"

START

  # Logical tests

  TEST_RR_OP 2, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f
  TEST_RR_OP 3, or, 0xfff0fff0, 0x0ff00ff0, 0xf0f0f0f0
  TEST_RR_OP 4, or, 0x0fff0fff, 0x00ff00ff, 0x0f0f0f0f
  TEST_RR_OP 5, or, 0xf0fff0ff, 0xf00ff00f, 0xf0f0f0f0

  # Source/Destination tests

  TEST_RR_SRC1_EQ_DEST  6, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f
  TEST_RR_SRC2_EQ_DEST  7, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f
  TEST_RR_SRC12_EQ_DEST 8, or, 0xff00ff00, 0xff00ff00


  TEST_RR_ZEROSRC1  24, or, 0xff00ff00, 0xff00ff00
  TEST_RR_ZEROSRC2  25, or, 0x00ff00ff, 0x00ff00ff
  TEST_RR_ZEROSRC12 26, or, 0
  TEST_RR_ZERODEST  27, or, 0x11111111, 0x22222222

EXIT
