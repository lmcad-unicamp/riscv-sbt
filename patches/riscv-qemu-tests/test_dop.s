.macro TEST_FP_OP1_D T OP flags res a
	j	2f
1:	.double \a
	.double \res
2:	li	gp, \T
	la	a0, 1b
	fld	f0, 0(a0)
	ld	a3, 8(a0)
	\OP	f3, f0
	fmv.x.d a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_FP_OP1_D_DWORD_RESULT T OP flags res a
	j	2f
1:	.double \a
	.quad   \res
2:	li	gp, \T
	la	a0, 1b
	fld	f0, 0(a0)
	ld	a3, 8(a0)
	\OP	f3, f0
	fmv.x.d a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_FP_OP2_D T OP flags res a b
	j	2f
1:	.double \a
	.double \b
	.double \res
2:	li	gp, \T
	la	a0, 1b
	fld	f0, 0(a0)
	fld	f1, 8(a0)
	ld	a3, 16(a0)
	\OP	f3, f0, f1
	fmv.x.d a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_FP_OP3_D T OP flags res a b c
	j	2f
1:	.double \a
	.double \b
	.double \c
	.double \res
2:	li	gp, \T
	la	a0, 1b
	fld	f0, 0(a0)
	fld	f1, 8(a0)
	fld	f2, 16(a0)
	ld	a3, 24(a0)
	\OP	f3, f0, f1, f2
	fmv.x.d a0, f3
	fsflags	a1, x0
	li	a2, \flags
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

.macro TEST_INT_FP_OP_D T OP res a
	j	2f
1:	.double \res
2:	la	a0, 1b
	ld	a3, 0(a0)
	li	a0, \a
	\OP	f0, a0
	fsflags	x0
	fmv.x.d	a0, f0
	li	gp, \T
	FAILIF bne	a0, a3
.endm

.macro TEST_FCVT_S_D T res a
	j         2f
1:	.double \a
	.double \res
2:	la        a0, 1b
	fld       f0, 0(a0)
	ld        a3, 8(a0)
	fcvt.s.d  f3, f0
	fcvt.d.s  f3, f3
	fmv.x.d   a0, f3
	fsflags   a1, x0
	li        a2, 0
	li        gp, \T
	FAILIF bne       a0, a3
	FAILIF bne       a1, a2
.endm

.macro TEST_FCVT_D_S T res a
	j          2f
1:	.float	\a
	.float	\res
2:	la         a0, 1b
	flw        f0, 0(a0)
	lw         a3, 4(a0)
	fcvt.d.s   f3, f0
        fcvt.s.d   f3, f3
	fmv.x.s    a0, f3
        fsflags	   a1, x0
	li         a2, 0
	li         gp, \T
	FAILIF bne        a0, a3
	FAILIF bne        a1, a2
.endm

.macro TEST_FP_INT_OP_D T OP flags res val rm
	j	2f
1:	.double \val
	.quad \res
2:	la	a0, 1b
	fld	f0, 0(a0)
	ld	a3, 8(a0)
	\OP	a0, f0, \rm
	fsflags	a1, x0
	li	a2, \flags
	li	gp, \T
	FAILIF bne	a0, a3
	FAILIF bne	a1, a2
.endm

# vim: ft=asm
