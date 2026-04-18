.org 0x80000000


.include "../macros/data.s" 
j _start

; A logging macro that takes all arguments and prints them
macro debug_log %*
    print_str .msg_%u
    j .skip_%u
.msg_%u: .asciz "DEBUG: %*"
.skip_%u:
endm


.text
_start:
    li sp, 0x80100000   # Initialize Stack
    li s1, 0x10000000   # UART Base
    li t1, 2
    li s3, 0
    debug_log t0, t1, s3

    qemu_off

