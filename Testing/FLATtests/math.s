.org 0x80000000
j _start

.include "../macros/data.s"

macro testing %1 %2
  li %1, %2
endm

.text
.align 4
_start:
    li sp, 0x80100000   # Initialize Stack
    li s1, 0x10000000   # UART Base
 
    li t0, 10+5         # Should be 15 (0xF)
    print_int_reg t0
    print_str ln
    li t1, _start+8
    print_hex_reg t1
    print_str ln
    print_hex_imm $
    print_str ln
    li t3, $+2
    print_hex_reg t3
    print_str ln 
    print_hex_imm $$
    print_str ln
    print_hex_imm $$+8
    print_str ln
    li t2, 2+3*4        # Should be 14 (0xE), NOT 20
    
    print_int_reg t2

    print_str ln
    print_str ln
    print_str ln
    
    testing t6 4/2*3+1  # 7
    print_int_reg t6
    print_str ln
    print_str ln
    print_str ln

    qemu_off

.data
ln: .asciz "\n"
