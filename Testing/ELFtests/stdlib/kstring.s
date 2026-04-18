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
