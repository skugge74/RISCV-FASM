.org 0x80000000
j _start

.include "./included.s"
.include "../macros/cond_loop.s"     

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
    
    repeat 3
      print_str secret_msg 
    endrepeat 
    
    print_str msg 
    qemu_off


.data
msg: .asciz "Main file message...\n"
