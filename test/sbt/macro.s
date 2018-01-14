.macro la reg, sym
    lui \reg, %hi(\sym)
    addi \reg, \reg, %lo(\sym)
.endm

.macro li reg, imm
    addi \reg, zero, \imm
.endm

.macro call func
    lui  ra, %hi(\func)
    jalr ra, ra, %lo(\func)
.endm
