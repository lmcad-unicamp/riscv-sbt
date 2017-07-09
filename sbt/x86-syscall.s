# RISC-V syscall:
#
# syscall#: a7
# arg0:         a0
# arg1:         a1
# arg2:         a2
# arg3:         a3
# arg4:         a4
# arg5:         a5

# X86 syscall:
#
# syscall#: eax
# arg0:         ebx
# arg1:         ecx
# arg2:         edx
# arg3:         esi
# arg4:         edi
# arg5:         ebp

.text

.global syscall0
syscall0:
    movl 4(%esp), %eax
    int $0x80
    ret

.global syscall1
syscall1:
    push %ebx

    movl 8(%esp), %eax
    movl 12(%esp), %ebx
    int $0x80

    pop %ebx
    ret

.global syscall2
syscall2:
    push %ebx

    movl 8(%esp), %eax
    movl 12(%esp), %ebx
    movl 16(%esp), %ecx
    int $0x80

    pop %ebx
    ret

.global syscall3
syscall3:
    push %ebx

    movl 8(%esp), %eax
    movl 12(%esp), %ebx
    movl 16(%esp), %ecx
    movl 20(%esp), %edx
    int $0x80

    pop %ebx
    ret

.global syscall4
syscall4:
    push %ebx
    push %esi

    movl 12(%esp), %eax
    movl 16(%esp), %ebx
    movl 20(%esp), %ecx
    movl 24(%esp), %edx
    movl 28(%esp), %esi
    int $0x80

    pop %esi
    pop %ebx
    ret

.global syscall5
syscall5:
    push %ebx
    push %esi
    push %edi

    movl 16(%esp), %eax
    movl 20(%esp), %ebx
    movl 24(%esp), %ecx
    movl 28(%esp), %edx
    movl 32(%esp), %esi
    movl 36(%esp), %edi
    int $0x80

    pop %edi
    pop %esi
    pop %ebx
    ret

.global syscall6
syscall6:
    push %ebx
    push %esi
    push %edi
    push %ebp

    movl 20(%esp), %eax
    movl 24(%esp), %ebx
    movl 28(%esp), %ecx
    movl 32(%esp), %edx
    movl 36(%esp), %esi
    movl 40(%esp), %edi
    movl 44(%esp), %ebp
    int $0x80

    pop %ebp
    pop %edi
    pop %esi
    pop %ebx
    ret


.global sbtabort
sbtabort:
    # getpid
    movl $0x14, %eax
    int $0x80
    # kill
    movl %eax, %ebx      # pid
    movl $6, %ecx        # SIGABORT
    movl $0x25, %eax
    int $0x80
