.data
msg:    .ascii "Hello, World!\n"
msg_len = . - msg

.text
.global _start
_start:

SYS_EXIT  = 93
SYS_WRITE = 64

    # write
    li a0,1
    la a1,msg
    li a2,msg_len
    li a7,SYS_WRITE
    ecall

    # exit
    li a0,0
    li a7,SYS_EXIT
    ecall
