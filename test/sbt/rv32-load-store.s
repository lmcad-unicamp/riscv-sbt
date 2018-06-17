# 01 lb
# 02 lbu
# 03 lh
# 04 lhu
# 05 lw
# 06 sb
# 07 sh
# 08 sw

.include "macro.s"

.text
.global main
main:
    # save ra
    add s1, zero, ra

    # print test
    lui a0, %hi(str)
    addi a0, a0, %lo(str)
    call printf

    ### load ###

    # lb
    lui a0, %hi(lb_str)
    addi a0, a0, %lo(lb_str)
    call printf
    # b0
    lui a0, %hi(b0_fmt)
    addi a0, a0, %lo(b0_fmt)
    lui t1, %hi(b0)
    addi t1, t1, %lo(b0)
    lb a1, 0(t1)
    add a2, a1, zero
    call printf
    # b1
    lui a0, %hi(b1_fmt)
    addi a0, a0, %lo(b1_fmt)
    lui t1, %hi(b1)
    addi t1, t1, %lo(b1)
    lb a1, 0(t1)
    add a2, a1, zero
    call printf

    # lbu
    # bu0
    lui a0, %hi(bu0_fmt)
    addi a0, a0, %lo(bu0_fmt)
    lui t1, %hi(b0)
    addi t1, t1, %lo(b0)
    lbu a1, 0(t1)
    add a2, a1, zero
    call printf
    # bu1
    lui a0, %hi(bu1_fmt)
    addi a0, a0, %lo(bu1_fmt)
    lui t1, %hi(bu1)
    addi t1, t1, %lo(bu1)
    lbu a1, 0(t1)
    add a2, a1, zero
    call printf

    # lh
    lui a0, %hi(lh_str)
    addi a0, a0, %lo(lh_str)
    call printf
    # h0
    lui a0, %hi(h0_fmt)
    addi a0, a0, %lo(h0_fmt)
    lui t1, %hi(h0)
    addi t1, t1, %lo(h0)
    lh a1, 0(t1)
    add a2, a1, zero
    call printf
    # b1
    lui a0, %hi(h1_fmt)
    addi a0, a0, %lo(h1_fmt)
    lui t1, %hi(h1)
    addi t1, t1, %lo(h1)
    lh a1, 0(t1)
    add a2, a1, zero
    call printf

    # lhu
    # hu0
    lui a0, %hi(hu0_fmt)
    addi a0, a0, %lo(hu0_fmt)
    lui t1, %hi(h0)
    addi t1, t1, %lo(h0)
    lhu a1, 0(t1)
    add a2, a1, zero
    call printf
    # hu1
    lui a0, %hi(hu1_fmt)
    addi a0, a0, %lo(hu1_fmt)
    lui t1, %hi(hu1)
    addi t1, t1, %lo(hu1)
    lhu a1, 0(t1)
    add a2, a1, zero
    call printf

    # lw
    lui a0, %hi(lw_str)
    addi a0, a0, %lo(lw_str)
    call printf
    # signed
    lui a0, %hi(w0_fmt)
    addi a0, a0, %lo(w0_fmt)
    lui t1, %hi(w)
    addi t1, t1, %lo(w)
    lw a1, 0(t1)
    add a2, a1, zero
    call printf
    # unsigned
    lui a0, %hi(w1_fmt)
    addi a0, a0, %lo(w1_fmt)
    lui t1, %hi(w)
    addi t1, t1, %lo(w)
    lw a1, 0(t1)
    add a2, a1, zero
    call printf

    ### store ###

    lui s3, %hi(s)
    addi s3, s3, %lo(s)

    lui s4, %hi(s_fmt)
    addi s4, s4, %lo(s_fmt)

    # sb
    lui a0, %hi(sb_str)
    addi a0, a0, %lo(sb_str)
    call printf
    # signed
    addi t0, zero, -128
    sb t0, 0(s3)
    add a0, zero, s4
    lw a1, 0(s3)
    call printf
    # unsigned
    addi t0, zero, 255
    sb t0, 0(s3)
    mv a0, s4
    lw a1, 0(s3)
    call printf

    # sh
    lui a0, %hi(sh_str)
    addi a0, a0, %lo(sh_str)
    call printf
    # signed
    lui t0, 0x10
    addi t0, t0, -1
    sh t0, 0(s3)
    add a0, zero, s4
    lw a1, 0(s3)
    call printf
    # unsigned
    lui t0, 0x8
    addi t0, t0, -1
    sh t0, 0(s3)
    add a0, zero, s4
    lw a1, 0(s3)
    call printf

    # sw
    lui a0, %hi(sw_str)
    addi a0, a0, %lo(sw_str)
    call printf
    # signed
    lui t0, 0x80012
    addi t0, t0, 0x345
    sw t0, 0(s3)
    add a0, zero, s4
    lw a1, 0(s3)
    call printf
    # unsigned
    lui t0, 0x80000
    addi t0, t0, -1
    sw t0, 0(s3)
    add a0, zero, s4
    lw a1, 0(s3)
    call printf

    # test array

    lui s3, %hi(a)
    addi s3, s3, %lo(a)

    lui s4, %hi(a_fmt)
    addi s4, s4, %lo(a_fmt)

    lui a0, %hi(a_str)
    addi a0, a0, %lo(a_str)
    call printf

    # 0
    add a0, zero, s4
    lw a1, 0(s3)
    call printf
    # 1
    add a0, zero, s4
    lw a1, 4(s3)
    call printf
    # 2
    add a0, zero, s4
    lw a1, 8(s3)
    call printf
    # 3
    add a0, zero, s4
    lw a1, 12(s3)
    call printf
    # 0/1: 0x01234567 = 0x67 0x45 0x23 0x01 + 0xAB = 0xAB012345
    add a0, zero, s4
    lw a1, 1(s3)
    call printf
    # 0/2
    add a0, zero, s4
    lw a1, 2(s3)
    call printf
    # 0/3
    add a0, zero, s4
    lw a1, 3(s3)
    call printf
    # store @ a[2]
    lui t0, 0x12345
    addi t0, t0, 0x678
    sw t0, 8(s3)
    lw a1, 8(s3)
    add a0, zero, s4
    call printf

    # restore ra
    add ra, zero, s1

    # return 0
    add a0, zero, zero
    jalr zero, ra, 0

.data
.p2align 2
str: .asciz "*** rv32-load-store ***\n"

b0: .byte 127
b0_fmt: .asciz "b0=%hhi 0x%08X\n"
b1: .byte -128
b1_fmt: .asciz "b1=%hhi 0x%08X\n"
bu0_fmt: .asciz "bu0=%hhu 0x%08X\n"
bu1: .byte 255
bu1_fmt: .asciz "bu1=%hhu 0x%08X\n"

h0: .short 32767
h0_fmt: .asciz "h0=%hi 0x%08X\n"
h1: .short -32768
h1_fmt: .asciz "h1=%hi 0x%08X\n"
hu0_fmt: .asciz "hu0=%hu 0x%08X\n"
hu1: .short 65535
hu1_fmt: .asciz "hu1=%hu 0x%08X\n"

w: .int 0x89ABCDEF
w0_fmt: .asciz "w0=%i 0x%08X\n"
w1_fmt: .asciz "w1=%u 0x%08X\n"

s: .int 0
s_fmt: .asciz "s=0x%08X\n"

a:
.int 0x01234567
.int 0x456789AB
.int 0x89ABCDEF
.int 0xCDEF0123
a_fmt: .asciz "a=0x%08X\n"

lb_str: .asciz "lb\n"
lh_str: .asciz "lh\n"
lw_str: .asciz "lw\n"
sb_str: .asciz "sb\n"
sh_str: .asciz "sh\n"
sw_str: .asciz "sw\n"
a_str:  .asciz "array\n"
