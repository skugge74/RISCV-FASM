.global _start
.global add_numbers
.extern main

.text
.align 4

_start:
    # 1. QEMU User-Mode already set up the Stack Pointer (sp) for us.
    # We just need to jump straight into the C code!
    call main

    # 2. main() will return our answer (42) in the a0 register.
    # We leave it in a0, and tell the Linux kernel to exit with that code.
    # 93 is the RISC-V Linux syscall number for "exit".
    li a7, 93
    ecall

# Our custom macro that C will call
add_numbers:
    add a0, a0, a1
    ret
