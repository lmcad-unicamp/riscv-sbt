# RISC-V syscall:
#
# syscall#: a7
# arg1:     a0
# arg2:     a1
# arg3:     a2
# arg4:     a3
# arg5:     a4
# arg6:     a5

# X86 syscall:
#
# syscall#: eax
# arg1:     ebx
# arg2:     ecx
# arg3:     edx
# arg4:     esi
# arg5:     edi
# arg6:     ebp

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

.global get_cycles
get_cycles:
  push %ebx

  xor %eax, %eax
  cpuid
  rdtsc

  pop %ebx
  ret

# time in msec (10^-3)
.global get_time
get_time:
  push %ebx

  # cpuid 0x16: max freq in MHz: ebx[15..0]
  mov $0x16, %eax
  cpuid

  # ebx = (ebx & 0xFFFF) * 1000
  and $0xFFFF, %ebx
  mov %ebx, %eax
  mov $1000, %ebx
  mul %ebx
  mov %eax, %ebx

  call get_cycles
  div %ebx
  pop %ebx
  ret

.global get_instret
get_instret:
  # INST_RETIRED
P6CTR_U = 0x010000
P6CTR_K = 0x020000
P6CTR_EN = 0x400000
  mov $0x004300C0, %ecx
  rdpmc
  ret

###
### UNUSED ###
###

#.data
#
#SYS_EXIT = 1
#SYS_WRITE = 4
#
#args:
#.long 9
#.long 1  # 1: exit
#.long 9
#.long 9
#.long 3  # 4: write
#args_max = (. - args) / 4
#
#targets:
#.long args0
#.long args1
#.long args2
#.long args3
#.long args4
#.long args5
#.long args6
#
#errmsg:
#.ascii "Invalid syscall: "
#errmsg_len = . - errmsg
#
#endl: .byte 0xA
#
#c: .byte 0
#
#### .text ###
#.text
#
#print_c:
#  push %ebx
#
#  movl $SYS_WRITE, %eax
#  movl $2, %ebx
#  movl $c, %ecx
#  movl $1, %edx
#  int $0x80
#
#  pop %ebx
#  ret
#
#
#print_eax:
#  # shift amount and stop condition
#  movl $28, %ecx
#loop:
#  movl %eax, %edx
#  shr %cl, %edx
#  andb $0xF, %dl
#  # isdigit?
#  cmpb $9, %dl
#  jg letter
#  addb $0x30, %dl
#  jmp loop2
#letter:
#  addb $0x37, %dl
#loop2:
#
#  # print char
#  movb %dl, c
#  push %eax
#  push %ecx
#  call print_c
#  pop %ecx
#  pop %eax
#
#  # loop
#  subl $4, %ecx
#  jge loop
#  ret
#
#
## X86
##
## syscall:
##
## syscall#: eax
## arg1:     ebx
## arg2:     ecx
## arg3:     edx
## arg4:     esi
## arg5:     edi
## arg6:     ebp
##
## int x86_syscall(
##     int nr,   # syscall#
##     int arg,* # args (n)
##
#.global syscall
#syscall:
#  push %ebp
#  movl %esp, %ebp
#
#  # syscall#
#  movl 8(%ebp), %eax
#  # < max?
#  cmpl $args_max, %eax
#  jae abort
#  # get # of args
#  push %ebx
#  movl $args, %edx
#  movl 0(%edx,%eax,4), %ebx
#  # 9 == invalid
#  cmpl $9, %ebx
#  je abort
#  push %ebx
#
#  # get jump target from table
#  movl $targets, %edx
#  movl 0(%edx,%ebx,4), %edx
#  jmp *%edx
#
#args6:
#  # unsupported
#  jmp abort
#args5:
#  push %edi
#  movl 28(%ebp), %edi
#args4:
#  push %esi
#  movl 24(%ebp), %esi
#args3:
#  movl 20(%ebp), %edx
#args2:
#  movl 16(%ebp), %ecx
#args1:
#  movl 12(%ebp), %ebx
#args0:
#  int $0x80
#
#  pop %ebx
#  # pop esi?
#  cmpl $4, %ebx
#  jb end
#  pop %esi
#  # pop edi?
#  cmpl $5, %ebx
#  jb end
#  pop %edi
#
#end:
#  pop %ebx
#  movl %ebp, %esp
#  pop %ebp
#  ret
#
#abort:
#  # print error
#  push %eax
#  movl $SYS_WRITE, %eax
#  movl $2, %ebx
#  movl $errmsg, %ecx
#  movl $errmsg_len, %edx
#  int $0x80
#  pop %eax
#  # print eax
#  call print_eax
#  # print \n
#  movl $SYS_WRITE, %eax
#  movl $2, %ebx
#  movl $endl, %ecx
#  movl $1, %edx
#  int $0x80
#
#  # exit
#  movl $SYS_EXIT, %eax
#  movl $1, %ebx
#  int $0x80
