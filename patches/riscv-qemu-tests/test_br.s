.macro TEST_BR2_OP_TAKEN T OP a b
	li	gp, \T
	li	t1, \a
	li	t2, \b
	\OP	t1, t2, 2f
	bne	x0, gp, fail
1:	bne	x0, gp, 3f
2:	\OP	t1, t2, 1b
	bne	x0, gp, fail
3:
.endm

.macro TEST_BR2_OP_NOTTAKEN T OP a b
	li	gp, \T
	li	t1, \a
	li	t2, \b
	\OP	t1, t2, 1f
	bne	x0, gp, 2f
1:	bne	x0, gp, fail
2:	\OP	t1, t2, 1b
3:
.endm

.macro TEST_BR2_SRC12_BYPASS T n1 n2 OP a b
	li	gp, \T
	li	t4, 0
1:	li	t1, \a
	# ...
	li	t2, \b
	# ...
	\OP	t1, t2, fail
	addi	t4, t4, 1
	li	t5, 2
	bne	t4, t5, 1b
.endm

.macro TEST_BR2_SRC21_BYPASS T n1 n2 OP a b
	li	gp, \T
	li	t4, 0
1:	li	t2, \b
	# ...
	li	t1, \a
	# ...
	\OP	t1, t2, fail
	addi	t4, t4, 1
	li	t5, 2
	bne	t4, t5, 1b
.endm
