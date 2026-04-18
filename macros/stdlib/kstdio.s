# ==========================================
# Kdex Standard I/O (kstdio.s)
# ==========================================
.equ STDOUT, 1
.equ SYS_WRITE, 64
.equ SYS_EXIT, 93

# Usage: exit 0
macro exit %1
    li a0, %1
    li a7, SYS_EXIT
    ecall
endm

# Usage: print [buffer_address], [length]
macro print %1, %2
    mv a2, %2          # a2 = length
    mv a1, %1          # a1 = pointer to buffer
    li a0, STDOUT      # a0 = file descriptor
    li a7, SYS_WRITE   # a7 = syscall number
    ecall
endm

macro print_char %1
    addi sp, sp, -16
    sw   t0, 4(sp)       # Save t0 so we don't break previous logic
    sw   a0, 8(sp)       # Save a0
    sw   a1, 12(sp)      # Save a1
    
    li   t0, %1          # Load the char (comma, newline, etc)
    sb   t0, 0(sp)       # Buffer it on stack
    
    # Manually trigger the syscall to avoid macro-in-macro confusion
    li   a0, 1           # stdout
    mv   a1, sp          # buffer pointer
    li   a2, 1           # length
    li   a7, 64          # sys_write
    ecall
    
    lw   a1, 12(sp)      # Restore everything
    lw   a0, 8(sp)
    lw   t0, 4(sp)
    addi sp, sp, 16
endm

# Usage: print_int_reg t0
macro print_int_reg %1
    addi sp, sp, -32
    mv   a0, %1        # Input value
    mv   a1, sp        # Buffer (stack)
    call kitoa         # Returns string pointer in a0
    
    mv   s0, a0        # Save string pointer
    call strlen        # Length returned in a0
    
    print s0, a0       # Print using the saved pointer and length
    addi sp, sp, 32
endm

# Usage: print_int_imm 1337
macro print_int_imm %1
    addi sp, sp, -32
    li   a0, %1        # Input immediate
    mv   a1, sp
    call kitoa
    
    mv   s0, a0
    call strlen
    print s0, a0
    addi sp, sp, 32
endm

# Function: kprintf
# Input: a0 = format string, a1-a7 = arguments
.global kprintf
kprintf:
    addi sp, sp, -64     # Increased stack for safety
    sw ra, 60(sp)
    sw s0, 56(sp)        # s0 = format string pointer
    sw s1, 52(sp)        # s1 = current argument index
    sw s2, 48(sp)        # s2 = current char
    
    # Store arguments a1-a7 into our "argument bank" on the stack
    sw a1, 0(sp)
    sw a2, 4(sp)
    sw a3, 8(sp)
    sw a4, 12(sp)
    sw a5, 16(sp)
    sw a6, 20(sp)
    sw a7, 24(sp)

    mv s0, a0            # s0 = format string
    li s1, 0             # s1 = arg index (starts at 0, which maps to a1)

.L_parse_loop:
    lb s2, 0(s0)
    beqz s2, .L_printf_done
    
    li t0, '%'
    beq s2, t0, .L_handle_specifier
    
    # --- RAW CHARACTER PRINT (Inline to avoid macro issues) ---
    addi sp, sp, -4
    sb s2, 0(sp)         # Put char on stack
    li a0, 1             # stdout
    mv a1, sp            # pointer to char
    li a2, 1             # length
    li a7, 64            # sys_write
    ecall
    addi sp, sp, 4
    j .L_next_char

.L_handle_specifier:
    addi s0, s0, 1       # Skip '%'
    lb s2, 0(s0)         # Get 'd', 's', or 'c'
    
    # Load current argument from our stack bank
    slli t0, s1, 2       # index * 4
    add  t0, t0, sp      # address
    lw   a0, 0(t0)       # a0 = argument value
    
    li t1, 'd'
    beq s2, t1, .L_do_int
    li t1, 's'
    beq s2, t1, .L_do_str
    li t1, 'c'
    beq s2, t1, .L_do_char
    j .L_next_char

.L_do_int:
    addi s1, s1, 1       # Increment arg index
    # Call kitoa manually to be safe
    addi sp, sp, -16     # Buffer for itoa
    mv a1, sp            # a1 = buffer
    # a0 already has the value
    call kitoa
    mv t5, a0            # t5 = string pointer
    call strlen          # a0 = length
    mv a2, a0            # a2 = length
    mv a1, t5            # a1 = pointer
    li a0, 1             # stdout
    li a7, 64            # write
    ecall
    addi sp, sp, 16      # Clean buffer
    j .L_next_char

.L_do_str:
    addi s1, s1, 1
    mv s2, a0            # s2 = string pointer
    call strlen
    mv a2, a0            # a2 = length
    mv a1, s2            # a1 = pointer
    li a0, 1             # stdout
    li a7, 64            # write
    ecall
    j .L_next_char

.L_do_char:
    addi s1, s1, 1
    addi sp, sp, -4
    sb a0, 0(sp)
    li a0, 1             # stdout
    mv a1, sp
    li a2, 1
    li a7, 64
    ecall
    addi sp, sp, 4
    j .L_next_char

.L_next_char:
    addi s0, s0, 1
    j .L_parse_loop

.L_printf_done:
    lw s2, 48(sp)
    lw s1, 52(sp)
    lw s0, 56(sp)
    lw ra, 60(sp)
    addi sp, sp, 64
    ret
