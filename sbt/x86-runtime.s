.text

.global sbtabort
sbtabort:
    # getpid
    movl $0x14, %eax
    int $0x80
    # kill
    movl %eax, %ebx     # pid
    movl $6, %ecx       # SIGABORT
    movl $0x25, %eax
    int $0x80
