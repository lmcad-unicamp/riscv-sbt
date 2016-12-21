.data
msg:    .ascii "Hello, world!\n"
msg_len = . - msg

.text
.global _start
_start:

.equ exit, 93
.equ write, 64

    # print
    li a0,1
    la a1,msg
    li a2,msg_len
    li a3,0
    li a7,write
    ecall

    # exit
    li a0,0
    li a1,0
    li a2,0
    li a3,0
    li a7,exit
    ecall
