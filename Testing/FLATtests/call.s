.org 0x80000000
j _start

.include "../macros/data.s"


.text
_start:
  init_uart
    # This should generate 2 instructions (auipc + jalr)
    call my_func

    qemu_off
# Define a function far away (or close, doesn't matter)
my_func:
    li t0, 42
    print_int_reg t0
    ret
