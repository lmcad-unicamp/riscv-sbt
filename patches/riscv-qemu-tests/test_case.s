.macro TEST_CASE T reg res
	mv	s0, \reg
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm

.macro TEST T reg res
	mv	s0, \reg
	li	s1, \res
	li	gp, \T
	FAILIF bne	s0, s1
.endm
