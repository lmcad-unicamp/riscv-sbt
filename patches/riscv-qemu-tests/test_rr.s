.macro TEST_RR_OP T OP r a b
	li	t0, \a
	li	t1, \b
	\OP	s0, t0, t1
	li	s1, \r
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_RR_DEST_BYPASS T n OP res a b
  TEST_RR_OP \T \OP \res \a \b
.endm

.macro TEST_RR_SRC12_BYPASS T n1 n2 OP res a b
  TEST_RR_OP \T \OP \res \a \b
.endm

.macro TEST_RR_SRC21_BYPASS T n1 n2 OP res a b
  TEST_RR_OP \T \OP \res \a \b
.endm

.macro TEST_RR_SRC1_EQ_DEST T OP res a b
	li	s0, \a
	li	t1, \b
	\OP	s0, s0, t1
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_RR_SRC2_EQ_DEST T OP res a b
	li	t0, \a
	li	s0, \b
	\OP	s0, t0, s0
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_RR_SRC12_EQ_DEST T OP res a
	li	s0, \a
	\OP	s0, s0, s0
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_RR_ZEROSRC1 T OP r b
	li	t1, \b
	\OP	s0, x0, t1
	li	s1, \r
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_RR_ZEROSRC2 T OP r a
	li	t0, \a
	\OP	s0, t0, x0
	li	s1, \r
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_RR_ZEROSRC12 T OP r
	\OP	s0, x0, x0
	li	s1, \r
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_RR_ZERODEST T OP a b
	li	t0, \a
	li	t1, \b
	\OP	x0, t0, t1
.endm
