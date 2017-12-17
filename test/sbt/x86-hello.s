.data
msg: .ascii "Hello, World!\n"
msglen = . - msg

SYS_EXIT  = 1
SYS_WRITE = 4

.text
.global _start
_start:
  movl $SYS_WRITE, %eax
  movl $1, %ebx
  movl $msg, %ecx
  movl $msglen, %edx
  int $0x80

  movl $SYS_EXIT, %eax
  movl $0, %ebx
  int $0x80
