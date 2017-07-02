.macro TEST_ST_OP T iL iS res off base
	la	t0, \base
	li	t1, \res
	\iS	t1, \off(t0)
	\iL	s0, \off(t0)
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST_ST_SRC12_BYPASS T n1 n2 iL iS res off base
  TEST_ST_OP \T \iL \iS \res \off \base
.endm

.macro TEST_ST_SRC21_BYPASS T n1 n2 iL iS res off base
  TEST_ST_OP \T \iL \iS \res \off \base
.endm
