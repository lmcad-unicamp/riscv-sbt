# 09 lui
# 10 add
# 11 addi
# 12 slti
# 13 sltiu
# 14 slt
# 15 sltu
# 16 and
# 17 andi
# 18 or
# 19 ori
# 20 xor
# 21 xori
# 22 sll
# 23 slli
# 24 srl
# 25 srli
# 26 sra
# 27 srai
# 28 auipc
# 29 sub

.option norelax
.include "macro.s"

.text
.global main
main:
    # s4: pc
    auipc s4, 0

    # s1: original ra
    add s1, zero, ra

    # s3: printf's fmt
    lui s3, %hi(w_fmt)
    addi s3, s3, %lo(w_fmt)

    # print test
    lui a0, %hi(str)
    addi a0, a0, %lo(str)
    call printf

    # lui
    lsym a0, lui_str
    call printf
    mv a0, s3
    lui a1, 0xCDEF5
    call printf

    # add/addi
    lsym a0, add_str
    call printf
    # 0
    addi t0, zero, 0x222
    addi t1, zero, 0x333
    mv a0, s3
    add a1, t0, t1
    call printf
    # 1
    addi t0, zero, 0x777
    addi t1, zero, -0x333
    mv a0, s3
    add a1, t0, t1
    call printf

    # slti
    lsym a0, slt_str
    call printf
    addi t0, zero, -0x111
    mv a0, s3
    slti a1, t0, 0x111
    call printf

    # sltiu
    addi t0, zero, -0x111
    mv a0, s3
    sltiu a1, t0, 0x111
    call printf

    # slt
    lui t0, 0x81234
    addi t1, zero, 1
    mv a0, s3
    slt a1, t0, t1
    call printf

    # sltu
    lui t0, 0x71234
    addi t1, zero, 1
    mv a0, s3
    slt a1, t0, t1
    call printf

    # and/andi
    lsym a0, and_str
    call printf
    addi t0, zero, -1
    andi t1, t0, 0x555
    mv a0, s3
    and a1, t0, t1
    call printf

    # or/ori
    lsym a0, or_str
    call printf
    lui t0, 0x12340
    ori t1, zero, 0x567
    mv a0, s3
    or a1, t0, t1
    call printf

    # xor/xori
    lsym a0, xor_str
    call printf
    lui t0, 0x12340
    addi t0, t0, 0x55
    xori t0, t0, 0xAA
    mv a0, s3
    xori t1, zero, 0x700
    xor a1, t0, t1
    call printf

    # sll/slli
    lsym a0, sll_str
    call printf
    addi t0, zero, 0xFF
    slli t0, t0, 0x8
    addi t1, zero, 4
    mv a0, s3
    sll a1, t0, t1
    call printf

    # srl/srli
    lsym a0, srl_str
    call printf
    addi t0, zero, 0x700
    srli t0, t0, 0x8
    addi t1, zero, 0x780
    mv a0, s3
    srl a1, t1, t0
    call printf

    # sra/srai
    lsym a0, sra_str
    call printf
    lui t0, 0x80000
    srai t0, t0, 0x7
    addi t1, zero, 0x8
    mv a0, s3
    sra a1, t0, t1
    call printf

    # sub
    lsym a0, sub_str
    call printf
    lui t0, 0x7
    addi t0, t0, 0x777
    lui t1, 0x4
    addi t1, t1, 0x321
    sub a1, t0, t1
    mv a0, s3
    call printf

#   For some reason that I have no time to investigate auipc test does not
#   work on clang/mc.
#
# gcc/as
.if 1
# clang/mc
#.if 0
    # auipc
    # symbol
    lsym a0, auipc_str
    call printf
    # non-symbol
    mv a0, s3
    auipc a1, 0x12345
    sub a1, a1, s4
    call printf
.endif

    # restore ra
    add ra, zero, s1

    # return 0
    add a0, zero, zero
    jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-alu-ops ***\n"

w_fmt: .asciz "w=0x%08X\n"

lui_str:    .asciz "lui\n"
add_str:    .asciz "add\n"
slt_str:    .asciz "slt\n"
and_str:    .asciz "and\n"
or_str:     .asciz "or\n"
xor_str:    .asciz "xor\n"
sll_str:    .asciz "sll\n"
sra_str:    .asciz "sra\n"
srl_str:    .asciz "srl\n"
sub_str:    .asciz "sub\n"
auipc_str:  .asciz "auipc\n"
