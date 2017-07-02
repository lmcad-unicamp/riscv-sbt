.macro TEST_CASE T reg res
	mv	s0, \reg
	li	s1, \res
	li	gp, \T
	bne	s0, s1, fail
.endm

.macro TEST T reg res
	mv	s0, \reg
	li	s1, \res
	li	gp, \T
	bne	s0, s1, fail
.endm
