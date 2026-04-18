.global _start
.global shared_var
.global add_to_shared_var
.global compute_offset 
.extern main

.data
.align 4
shared_var:
    .word 10
    .word 30

.text
.align 4
_start:
    call main
    li a7, 93
    ecall

add_to_shared_var:
    la t0, shared_var + 4
    lw t1, 0(t0)
    add t1, t1, a0
    sw t1, 0(t0)
    mv a0, t1
    ret

# Test: li with expression
compute_offset:
    li t0, 4 * 8         # Should encode as li t0, 32
    li t1, 1 << 3        # Should encode as li t1, 8
    add a0, t0, t1       # Return 40
    ret
