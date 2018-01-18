.include "macro.s"

.text
.global main
main:
    # save ra
    mv s1, ra

    # print test
    la a0, str
    call printf

    # load f
    la  t0, f
    fld f0, 0(t0)

    # print it
    call putd

    # restore ra
    mv ra, s1

    # return 0
    li a0, 0
    ret

.global putd
putd:
    # save ra
    mv s2, ra

    # arg0 = f_fmt
    la a0, f_fmt
    # arg1 = ' '
    li a1, 0x20
    # arg2 = d
    la t0, tmp
    fsd f0, 0(t0)
    lw a2, 0(t0)
    lw a3, 4(t0)
    call printf

    # restore ra
    mv ra, s2
    ret

.data
.p2align 2
str:    .asciz "*** rv32-f ***\n"
f_fmt:  .asciz "%c%f\n"
.align 8
f:      .double 1.234
tmp:    .double 0
