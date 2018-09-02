.include "macro.s"

.text
.global main
main:
    lui     t0, %hi(var)
    addi    t0, t0, %lo(var)
    lbu     a0, 3(t0)
    ret

.data
var:    .byte 12
        .byte 34
        .byte 56
        .byte 0
