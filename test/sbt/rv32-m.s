# 01 mul

.text
.global main
main:
    # save ra
    add s1, zero, ra

    # print test
    lui a0, %hi(str)
    addi a0, a0, %lo(str)
    call printf

    # mul
    lui a0, %hi(mul)
    addi a0, a0, %lo(mul)
    call printf
    addi t1, zero, -2
    addi t2, zero, 45
    mul a1, t1, t2
    lui a0, %hi(fmt)
    addi a0, a0, %lo(fmt)
    call printf

    # restore ra
    add ra, zero, s1

    # return 0
    add a0, zero, zero
    ret

.data
.p2align 2
str: .asciz "*** rv32-m ***\n"

mul:  .asciz "mul\n"
fmt:  .asciz "%i\n"
