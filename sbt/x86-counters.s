.text

# XXX
#
# The code below is needed only to emulate
# - get_cycles
# - get_time
# - get_instret

.global counters_init
counters_init:
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

    # point esi to first non space char of brand_str,
    # starting from the end
    # (brand_str len is 48 bytes)

    movl $brand_str, %esi
    addl $47, %esi

loop2:
    subl $1, %esi
    movb (%esi), %al
    cmpb $' ', %al
    jne loop2
    addl $1, %esi

    # now parse the frequency

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
    # XXX does this really changes ebx?
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

    # time =    cycles / freq (64-bit/32-bit div)
    #
    # cycles_hi (edx) = cycles >> 32
    # cycles_lo (eax) = cycles & 0xFFFFFFFF
    # time_hi (ecx) = cycles_hi / freq
    # time_hi_remainder (edx) = cycles_hi % freq
    # time_lo (eax) = ((time_hi_remainder << 32) | cycles_lo) / freq

    # time_hi
    push %eax             # cycles_lo
    mov %edx, %eax        # eax = cycles_hi
    xor %edx, %edx
    div %ebx              # cycles_hi / freq
    mov %eax, %ecx        # ecx = cicles_hi / freq (time_hi)
    pop %eax              # eax = cycles_lo

    # time_lo
    div %ebx              # remainder:cycles_lo / freq (time_lo)
    mov %ecx, %edx        # edx = time_hi

    pop %ebx
    ret

.global get_instret
get_instret:
    mov $0x40000000, %ecx
    rdpmc
    ret

.data
.p2align 4

# CPU frequency
freq:     .int 0
# number to the left of the point
freq_l: .int 0
# number to the right of the point
freq_r: .int 0
# unity (M|G)
freq_u: .byte 0
# format string
freq_str: .asciz "%d.%d%c"

# error string
error_str:    .asciz "syscall_init failed!\n"

# CPU brand string
brand_str:    .zero 64
