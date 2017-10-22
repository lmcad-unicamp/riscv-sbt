.macro TEST_IMM_OP T OP res a imm
	li	t0, \a
	\OP	s0, t0, \imm
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_IMM_DEST_BYPASS T n OP res a imm
  TEST_IMM_OP \T, \OP, \res, \a, \imm
.endm

.macro TEST_IMM_SRC1_BYPASS T n OP res a imm
  TEST_IMM_OP \T, \OP, \res, \a, \imm
.endm

.macro TEST_IMM_SRC1_EQ_DEST T OP res a imm
	li	s0, \a
	\OP	s0, s0, \imm
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_IMM_ZEROSRC1 T OP res imm
	\OP	s0, x0, \imm
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_IMM_ZERODEST T OP a imm
	li	t0, \a
	\OP	x0, t0, \imm
.endm
