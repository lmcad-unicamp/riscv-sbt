.include "macro.s"

.text
.global main
main:
    # save ra
    mv s1, ra

    # print test
    la a0, str
    call printf

    # print f
    la a0, f_fmt
    la t0, f
    lw a1, 0(t0)
    lw a2, 4(t0)
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
