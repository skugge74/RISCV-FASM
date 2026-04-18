# ==========================================
# Kdex Standard I/O (kstdio.s)
# ==========================================
.equ STDIN,  0
.equ STDOUT, 1
.equ STDERR, 2

.equ SYS_READ,  63
.equ SYS_WRITE, 64
.equ SYS_EXIT,  93

# Exit the program gracefully
# Usage: exit [status_code]
macro exit %1
    li a0, %1
    li a7, SYS_EXIT
    ecall
endm

# Print a raw buffer
# Usage: print [buffer_address], [length]
macro print %1 %2
    mv a2, %2         # MOVE USER ARG FIRST
    mv a1, %1         # MOVE USER ARG FIRST
    li a0, STDOUT     # THEN safely overwrite a0
    li a7, SYS_WRITE
    ecall
endm

# Print a single ASCII character immediately
# Usage: print_char '\n'
macro print_char %1
    addi sp, sp, -16     # Allocate stack space
    li t0, %1            # Load the ASCII character
    sb t0, 0(sp)         # Store byte on the stack
    
    mv a1, sp            # Pointer is our stack pointer
    li a2, 1             # Length is 1 byte
    li a0, STDOUT        # Safely load stdout
    li a7, SYS_WRITE
    ecall
    
    addi sp, sp, 16      # Free stack space
endm
