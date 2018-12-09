.include "macro.s"

.text
.global main
main:
    # save return address
    mv      s0, ra

    addi    t0, zero, 2
    blt     a0, t0, no_args

    lui     a0, %hi(str_first_arg)
    addi    a0, a0, %lo(str_first_arg)
    lw      a1, 4(a1)
    call    printf
    j       out

no_args:
    lui     a0, %hi(str_no_args)
    addi    a0, a0, %lo(str_no_args)
    call    puts

out:
    mv      a0, zero
    mv      ra, s0
    ret

.data
.align 4
str_first_arg:  .asciz  "First argument: %s\n"
str_no_args:    .asciz  "No arguments"
