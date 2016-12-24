SYS_EXIT = 1
SYS_WRITE = 4

.data
msg: .ascii "X86 syscall test\n"
msg_len = . - msg

.text
.global _start
.extern syscall
_start:
  pushl $msg_len
  pushl $msg
  pushl $1
  pushl $SYS_WRITE
#  pushl $0x12345678
#  pushl $0x9ABCDEF0
  call syscall

  pushl $0
  pushl $SYS_EXIT
  call syscall
