.org 0x80000000
j _start

.include "./macros/data.s"
.include "./macros/cond_loop.s"


.text
.align 4
_start:
    init_uart

    li t0, 20
    li t1, 20
    
    if eq, t0, t1
        print_str match_msg
    endif 

    if ne, t0, t1
        print_str no_match_msg
    endif

  qemu_off


.data
match_msg: .asciz "Matched!\n"
no_match_msg: .asciz "Not matched\n"
success_msg: .asciz "succes\n"
