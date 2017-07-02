.macro TEST_LD_OP T OP res off base
	la	t1, \base
	\OP	s0, \off(t1)
	li	s1, \res
	li	gp, \T
	bne	s0, s1, fail
.endm

.macro TEST_LD_DEST_BYPASS T n OP res off base
  TEST_LD_OP \T \OP \res \off \base
.endm

.macro TEST_LD_SRC1_BYPASS T n OP res off base
  TEST_LD_OP \T \OP \res \off \base
.endm
