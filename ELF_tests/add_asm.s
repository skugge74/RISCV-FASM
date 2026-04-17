.global _start
.global shared_var
.global add_to_shared_var
.extern main

# --- WRITABLE DATA SECTION ---
.data
.align 4
shared_var:
    .word 10    # Initialize the variable to 10

# --- READ-ONLY CODE SECTION ---
.text
.align 4

_start:
    call main
    li a7, 93
    ecall

# Function: Adds a0 to shared_var and returns the new value
add_to_shared_var:
    la t0, shared_var     # Load the address of the variable
    lw t1, 0(t0)          # Load its current value into t1
    
    add t1, t1, a0        # Add the C argument (a0) to the value
    sw t1, 0(t0)          # Store it back! (WILL SEGFAULT IF IN .TEXT)
    
    mv a0, t1             # Move the final value to a0 to return it
    ret
