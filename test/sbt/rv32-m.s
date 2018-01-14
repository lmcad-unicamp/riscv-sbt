# 01 mul

.text
.global main
main:
    # save ra
    add s1, zero, ra

    # s2: printf
    lui s2, %hi(printf)
    addi s2, s2, %lo(printf)

    # print test
    lui a0, %hi(str)
    addi a0, a0, %lo(str)
    jalr ra, s2, 0

    # mul
    lui a0, %hi(mul)
    addi a0, a0, %lo(mul)
    jalr ra, s2, 0
    addi t1, zero, -2
    addi t2, zero, 45
    mul a1, t1, t2
    lui a0, %hi(fmt)
    addi a0, a0, %lo(fmt)
    jalr ra, s2, 0

    # restore ra
    add ra, zero, s1

    # return 0
    add a0, zero, zero
    jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-m ***\n"

mul:  .asciz "mul\n"
fmt:  .asciz "%i\n"
