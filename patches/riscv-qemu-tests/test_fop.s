.macro TEST_FP_OP2_S T OP flags res a b
	j	2f
1:	.float \a
	.float \b
	.float \res
2:	li	gp, \T
	la	a0, 1b
	flw	f0, 0(a0)
	flw	f1, 4(a0)
	lw	a3, 8(a0)
	\OP	f3, f0, f1
	fmv.x.s a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_FP_CMP_OP_S T, OP, res, a, b
	j	2f
1:	.float	\a
	.float	\b
2:	la	a0, 1b
	flw	f0, 0(a0)
	flw	f1, 4(a0)
	\OP	a0, f0, f1
	li	a1, \res
	FAILIF bne	a0, a1
.endm

.macro TEST_INT_FP_OP_S T OP res int
	j	2f
1:	.float	\res
2:	la 	a0, 1b
	lw	a3, 0(a0)
	li	a0, \int
	\OP	f0, a0
	fmv.x.s a0, f0
	li	gp, \T
	FAILIF bne	a0, a3
.endm

.macro TEST_FP_OP1_S T OP flags res a
	j	2f
1:	.float \a
	.float \res
2:	li	gp, \T
	la	a0, 1b
	flw	f0, 0(a0)
	lw	a3, 4(a0)
	\OP	f3, f0
	fmv.x.s a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_FP_OP1_S_DWORD_RESULT T OP flags res a
	j	2f
1:	.float \a
	.quad \res
2:	li	gp, \T
	la	a0, 1b
	flw	f0, 0(a0)
	lw	a3, 4(a0)
	\OP	f3, f0
	fmv.x.s a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_FP_OP3_S T OP flags res a b c
	j	2f
1:	.float \a
	.float \b
	.float \c
	.float \res
2:	li	gp, \T
	la	a0, 1b
	flw	f0, 0(a0)
	flw	f1, 4(a0)
	flw	f2, 8(a0)
	lw	a3, 12(a0)
	\OP	f3, f0, f1, f2
	fmv.x.s a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_FP_INT_OP_S T OP flags res val rm
	j	2f
1:	.float \val
	.quad \res
2:	la	a0, 1b
	fld	f0, 0(a0)
	ld	a3, 4(a0)
	\OP	a0, f0, \rm
	fsflags	a1, x0
	li	a2, \flags
	li	gp, \T
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm
