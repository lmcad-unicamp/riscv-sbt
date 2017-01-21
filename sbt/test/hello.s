.data
long0: .long 1234
msg:    .ascii "Hello, world!\n"
msg_len = . - msg
long1: .long 5678

.text
.global _start
_start:

SYS_EXIT  = 93
SYS_WRITE = 64

    # write
    li a0,1
    la a1,long0
    la a1,long1
    la a1,msg
    li a2,msg_len
    li a3,0
    li a7,SYS_WRITE
    ecall

    # exit
    li a0,0
    li a1,0
    li a2,0
    li a3,0
    li a7,SYS_EXIT
    ecall
