.org 0x80000000
j _start

# This includes your macros AND the 'ln' variable
.include "./macros/data.s"

# --------------------------------------
# CONSTANTS
# --------------------------------------
.equ BIT_3,  (1 << 3)        # 8
.equ BIT_4,  (1 << 4)        # 16
.equ MASK_A, (BIT_3 | BIT_4) # 24

# Using parens to force (Shift) then (Add)
.equ PREC_TEST, ((1 << 2) + 1) # 5

.text
.align 4
_start:
    # init_uart now sets SP to 0x80100000 automatically!
    init_uart
    print_str hello
    print_str ln
    # 3. Run Tests
    li t0, BIT_3
    print_int_reg t0
    print_str ln
    
    li t0, MASK_A      
    print_int_reg t0
    print_str ln
    
    li t1, PREC_TEST   
    print_int_reg t1
    print_str ln

    li t2, (0xF0 & 0x30) 
    print_int_reg t2
    print_str ln
    
    li t3, (0x0F ^ 0xFF) 
    print_int_reg t3
    print_str ln
    
    halt
.data
hello: .asciz "Hello, this is a print test\n"
