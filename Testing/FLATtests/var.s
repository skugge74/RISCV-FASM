.org 0x80000000
    j _start

.include "../macros/data.s"


OFFSET = 2

OFFSET_HP = OFFSET        ; OFFSET_HP becomes 2
OFFSET = OFFSET + 4       ; Update OFFSET to 4

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000

    li t6, OFFSET       ; Should compile to: li t0, 2
    print_int_reg t0 
    print_str ln 
    
    li t1, OFFSET_HP
    print_int_reg t1 
    print_str ln 
    
    OFFSET = OFFSET + 8
    print_int_imm OFFSET
    print_str ln 
    
    qemu_off

.data
ln: .asciz "\n"
