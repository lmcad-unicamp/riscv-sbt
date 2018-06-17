.macro lsym reg, sym
    lui \reg, %hi(\sym)
    addi \reg, \reg, %lo(\sym)
.endm

# needed by LLVM, for now
.macro li reg, imm
   lui  \reg, %hi(\imm)
   addi \reg, \reg, %lo(\imm)
.endm

# .macro call func
#    lui  ra, %hi(\func)
#    jalr ra, ra, %lo(\func)
# .endm

.data

FAIL:	.ascii	"FAIL\n"
.equ FLEN, .-FAIL
.type FAIL, object
.size FAIL, FLEN

.macro START
	.text
	.globl _start
	_start:
.endm

.macro FAILFUNC
.global failfunc
failfunc:
	li	a0, 2
	lsym a1, FAIL
	li	a2, FLEN
	li	a7, 64 # write
	ecall

	mv	a0, gp
	li	a7, 93 # exit
	ecall
.endm

.macro FAILIF CBR a b
	\CBR \a, \b, 8f
	j 9f
	8: call failfunc
	9: nop
.endm

.macro EXIT
_exit:
	li	a0, 0
	li	a7, 93
	ecall

FAILFUNC
.endm

.include "test_case.s"
.include "test_imm.s"
.include "test_dop.s"
.include "test_fop.s"
.include "test_rr.s"
.include "test_ld.s"
.include "test_st.s"
.include "test_br.s"
