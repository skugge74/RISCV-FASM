# ==========================================
# Kdex String Library (kstring.s)
# ==========================================

# Print a null-terminated string
# Usage: print_str [label]
macro print_str %1
    addi sp, sp, -8
    sw ra, 0(sp)
    sw a0, 4(sp)
    
    la a0, %1          # Load string address
    call strlen        # Calculate length
    mv t0, a0          # Move length to t0
    
    la a0, %1          # Reload address
    print a0, t0       # Call kstdio print macro
    
    lw a0, 4(sp)
    lw ra, 0(sp)
    addi sp, sp, 8
endm

.text
.align 4

# Function: strlen
# Input:  a0 = string pointer
# Output: a0 = length
.global strlen
strlen:
    mv t0, a0          # Save original pointer
@@:
    lb t1, 0(t0)       # Load byte
    beqz t1, @f        # If null terminator, jump forward to end
    addi t0, t0, 1     # Increment pointer
    j @b               # Jump back to loop start
@@:
    sub a0, t0, a0     # Length = current pointer - original pointer
    ret

# Function: strcmp
# Input:  a0 = string 1 pointer, a1 = string 2 pointer
# Output: a0 = 0 if equal, non-zero if different
.global strcmp
strcmp:
@@:
    lb t0, 0(a0)       # Load char from str1
    lb t1, 0(a1)       # Load char from str2
    bne t0, t1, @f     # If they differ, jump to diff
    beqz t0, .L_eq     # If we hit null terminator and they match, jump to equal
    addi a0, a0, 1     # Next char
    addi a1, a1, 1
    j @b
@@:
    sub a0, t0, t1     # Return the difference
    ret
.L_eq:
    li a0, 0           # Return 0 (Match)
    ret


.global kitoa
kitoa:
    addi sp, sp, -32
    sw   ra, 28(sp)
    sw   s0, 24(sp)
    sw   s1, 20(sp) 
    
    mv   s0, a1         # s0 = buffer
    mv   t0, a0         # t0 = number to convert
    
    # Null terminator at the end of 12-byte buffer
    addi t1, s0, 11
    sb   zero, 0(t1)
    
    # Handle 0
    bnez t0, .L_loop
    addi t1, t1, -1
    li   t2, '0'
    sb   t2, 0(t1)
    j    .L_copy_final

.L_loop:
    li   t2, 10
    mv   t3, zero       # quotient
    mv   t4, t0         # remainder candidate
.L_manual_div:          # Software division: t4 = t0 % 10, t3 = t0 / 10
    blt  t4, t2, .L_div_done
    sub  t4, t4, t2
    addi t3, t3, 1
    j    .L_manual_div
.L_div_done:
    addi t4, t4, 48     # ASCII conversion
    addi t1, t1, -1
    sb   t4, 0(t1)
    mv   t0, t3         # Next iteration with quotient
    bnez t0, .L_loop

.L_copy_final:
    # Shift result to the front of the buffer
    mv   a0, s0
.L_copy_inner:
    lb   t2, 0(t1)
    sb   t2, 0(s0)
    addi t1, t1, 1
    addi s0, s0, 1
    bnez t2, .L_copy_inner
    
    lw   s1, 20(sp) 
    lw   s0, 24(sp)
    lw   ra, 28(sp)
    addi sp, sp, 32
    ret
