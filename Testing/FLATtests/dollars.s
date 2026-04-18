.org 0x80000000
j _start

.include "../macros/data.s" 

.text
_start:
    li sp, 0x80100000   # Initialize Stack
    li s1, 0x10000000   # UART Base
    print_str msg  
    print_str hex  
    print_hex_imm $  
    print_str nl  
    print_str int
    print_int_imm $
    print_str nl  

    print_str msg2  
    print_str hex  
    print_hex_imm $$  
    print_str nl  
    print_str int
    print_int_imm $$
    print_str ln  
    qemu_off
.data
msg: .asciz "$ is: \n"
msg2: .asciz "$$ is: \n"
hex: .asciz "as hex: "
int: .asciz "as int: "
nl: .asciz "\n"
