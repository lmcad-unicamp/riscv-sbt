.include "macro.s"

.text
.global main
main:
    # save ra
    mv s1, ra

    # print test
    lsym a0, str
    call printf

    # print f
    lsym a0, f_fmt
    lsym t0, f
    fld fa0, 0(t0)
    call sbt_printf_d

    # restore ra
    mv ra, s1

    # return 0
    li a0, 0
    ret

.data
.p2align 2
str:    .asciz "*** rv32-f ***\n"
f_fmt:  .asciz "%f\n"
.align 8
f:      .double 1.234
