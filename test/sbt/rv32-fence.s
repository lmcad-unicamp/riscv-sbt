# 38 fence
# 39 fence.i

.text
.global main
main:
    # save ra
    add s1, zero, ra

    # print test
    lui a0, %hi(str)
    addi a0, a0, %lo(str)
    call printf

    # XXX this actually tests nothing for now...

    # store
    lui t2, %hi(x)
    addi t2, t2, %lo(x)
    lui t1, 0x12345
    addi t1, t1, 0x678
    sw t1, 0(t2)

    # fence
    fence
    fence.i

    # load
    lui a0, %hi(fmt)
    addi a0, a0, %lo(fmt)
    lw a1, 0(t2)
    call printf

    # restore ra
    add ra, zero, s1

    # return 0
    add a0, zero, zero
    jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-fence ***\n"

x: .int 0
fmt:  .asciz "0x%08X\n"
