.org 0x80000000
j _start

.include "./included.s"
.include "../macros/data.s"     

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
    
    repeat 3
      print_str secret_msg 
    endrepeat 
    
    print_str msg 
    halt


.data
msg: .asciz "Main file message...\n"
