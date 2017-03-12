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


.global syscall_init
syscall_init:
  push %ebx

  ###
  ### get CPU frequency
  ###

  # get CPU brand string
  movl $0x80000002, %esi
  movl $brand_str, %edi
loop:
  mov %esi, %eax
  cpuid
  movl %eax, (%edi)
  movl %ebx, 4(%edi)
  movl %ecx, 8(%edi)
  movl %edx, 12(%edi)

  addl $1, %esi
  addl $16, %edi
  cmpl $0x80000005, %esi
  jne loop

  # print it
  # pushl $brand_str
  # calll printf
  # addl $4, %esp
  # pushl $nl
  # calll puts
  # addl $4, %esp

  # point esi to first non space char of brand_str,
  # starting from the end
  movl $brand_str, %esi
  addl $47, %esi

loop2:
  subl $1, %esi
  movb (%esi), %al
  cmpb $0x20, %al
  jne loop2
  addl $1, %esi

  # print it
  # pushl %esi
  # calll puts
  # addl $4, %esp
  # pushl $nl
  # calll puts
  # addl $4, %esp

  # now parse frequency
  pushl $freq_u
  pushl $freq_r
  pushl $freq_l
  pushl $freq_str
  pushl %esi
  calll sscanf
  addl $20, %esp

  cmpl $3, %eax
  jne error

  # freq_l = number to the left of the '.'
  movl freq_l, %eax

  # GHz: multiply by 1000
  movb freq_u, %dl
  cmpb $'G', %dl
  jne mhz
  movl $1000, %ebx
  mul %ebx
  # then add freq_r: number to the right of the '.'
  addl freq_r, %eax
  jmp save_freq

mhz:
  # MHz: not tested: should work if brand_str contains a '.'
  cmpb $'M', %dl
  jne error

save_freq:
  movl %eax, freq

  # print final frequency
  # pushl %eax
  # pushl $fmt
  # calll printf
  # addl $8, %esp

  jmp end

error:
  pushl $error_str
  calll puts
  addl $4, %esp

  pushl $1
  calll exit

end:
  pop %ebx
  ret


.global get_cycles
get_cycles:
  push %ebx

  # flush pipeline
  # TODO does this really changes ebx?
  xor %eax, %eax
  cpuid
  # get cycles in edx:eax
  rdtsc

  pop %ebx
  ret

# time in usec (10^-6) (edx:eax)
.global get_time
get_time:
  push %ebx
  mov freq, %ebx

  # cycles: edx:eax
  call get_cycles

  # time =  cycles / freq (64-bit div)
  # cycles_hi (edx) = cycles >> 32
  # cycles_lo (eax) = cycles & 0xFFFFFFFF
  # time_hi (ecx) = cycles_hi / freq
  # time_hi_remainder (edx) = cycles_hi % freq
  # time_lo (eax) = ((time_hi_remainder << 32) | cycles_lo) / freq

  # time_hi
  push %eax
  mov %edx, %eax
  xor %edx, %edx
  div %ebx
  mov %eax, %ecx
  pop %eax

  # time_lo
  div %ebx
  mov %ecx, %edx

  pop %ebx
  ret

.global get_instret
get_instret:
  mov $0x40000001, %ecx
  rdpmc
  #pushl %eax
  #pushl $instret_fmt
  #calll printf
  #addl $8, %esp
  ret

.data
.p2align 4
freq:   .int 0
freq_l: .int 0
freq_r: .int 0
freq_u: .byte 0
freq_str: .asciz "%d.%d%c"

error_str:  .asciz "syscall_init failed!\n"
fmt:        .asciz "freq=%uMHz\n"
test_str:   .asciz "TEST\n"
brand_str:  .zero 64
nl:         .asciz "\n"

instret_fmt:   .asciz "instret=0x%08X\n"
