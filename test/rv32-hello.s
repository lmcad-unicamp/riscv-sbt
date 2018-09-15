.data
dummy: .long 1234
msg:    .ascii "Hello, World!\n"
msg_len = . - msg

SYS_WRITE = 64
SYS_EXIT = 93

.text
.global _start
_start:
    # print
    addi a0, zero, 1
    lui a1, %hi(msg)
    addi a1, a1, %lo(msg)
# gcc/as
    addi a2, zero, %lo(msg_len)
# clang/mc
#   addi a2, zero, 14
    addi a7, zero, %lo(SYS_WRITE)
    ecall

    # exit
    addi a0, zero, 0
    addi a7, zero, %lo(SYS_EXIT)
    ecall
