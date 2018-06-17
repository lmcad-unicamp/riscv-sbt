.include "macro.s"

.text
.global main
main:
    lsym a0, str
    mv s0, ra
    call puts
    mv ra, s0
    li a0, 0
    ret

.data
str: .asciz "test"
