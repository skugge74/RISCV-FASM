.org 0x80000000
j _start

.include "./data.s"

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
 
    print_str hello
    la t0, len
    lw t1 0(t0)
    print_int_reg t1

.data
hello: .asciz "Hello World\n"
.align 4
len: .word $-hello
  
