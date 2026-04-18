.data
hello_msg: .asciz "Linking success! The value is: %d\n"

.text
.global _start
_start:
    la   a0, hello_msg    # format string
    li   a1, 42           # argument
    call kprintf

    li   a0, 0
    li   a7, 93
    ecall
