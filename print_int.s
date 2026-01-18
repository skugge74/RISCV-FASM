.org 0x80000000
j _start

.include "./data.s"  

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
    
    
    print_int_imm 1234
    
   
    li a0, 10 
    sb a0, 0(s1)

  
    li t0, -42
    print_int_reg t0
    
    halt
