.text

FAIL:	.ascii	"FAIL\n"
.equ FLEN, .-FAIL
.type FAIL, object
.size FAIL, FLEN

.text
.align 4

.macro START
	.text
	.globl _start
	.align 4
	_start:
.endm

.macro EXIT
_exit:
	li	a0, 0
	li	a7, 93
	ecall

fail:
	li	a0, 2
	la	a1, FAIL
	li	a2, FLEN
	li	a7, 64 /* write */
	ecall

	mv	a0, gp
	li	a7, 93 /* exit */
	ecall
.endm

.include "test_case.s"
.include "test_imm.s"
.include "test_dop.s"
.include "test_fop.s"
.include "test_rr.s"
.include "test_ld.s"
.include "test_st.s"
.include "test_br.s"
