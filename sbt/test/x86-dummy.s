.text

SYS_EXIT  = 1

.global exit
.type exit,@function
exit:
  movl $SYS_EXIT, %eax
  movl $0, %ebx
  int $0x80

.global sscanf
.type sscanf,@function
sscanf:
  call exit

.global puts
.type puts,@function
puts:
  call exit
