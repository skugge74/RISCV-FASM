.org 0x80000000
j _start

.include "./macros/data.s"  

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000

    print_str msg_core
    
    csrr t0, 0xF14  
    print_hex_reg t0
    
    print_str newline

    print_str msg_write
    
    li t1, 0xDEADBEEF
    
    csrw 0x340, t1 
    
    li t1, 0
    
    csrr t1, 0x340 
    print_hex_reg t1
    
    print_str newline
    qemu_off

.data
msg_core:  .asciz "Core ID: "
msg_write: .asciz "Wrote DEADBEEF to mscratch. Read back: "
newline:   .asciz "\n"
