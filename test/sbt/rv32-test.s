.text
.global main
main:
    la a0, str
    mv s0, ra
    call puts
    mv ra, s0
    ret

.data
str: .asciz "test"
