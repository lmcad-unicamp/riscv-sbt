SYS_EXIT = 1
SYS_WRITE = 4

.data
msg: .ascii "X86 syscall test\n"
msg_len = . - msg

.text
.global _start
.extern syscall1
.extern syscall4
_start:
  pushl $msg_len
  pushl $msg
  pushl $1
  pushl $SYS_WRITE
  call syscall4

  pushl $0
  pushl $SYS_EXIT
  call syscall1
