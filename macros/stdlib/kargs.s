# ==========================================
# Kdex Argument Parser (kargs.s)
# ==========================================

.data
.align 4
k_argc: .word 0
k_argv: .word 0

.text
.align 4

# Function: k_init_args
# Must be called at the very start of _start
# Input: Expects sp to be at the Linux entry state
.global k_init_args
k_init_args:
    lw t0, 0(sp)        # Load argc from stack
    la t1, k_argc
    sw t0, 0(t1)        # Save to k_argc
    
    addi t0, sp, 4      # argv starts at sp + 4
    la t1, k_argv
    sw t0, 0(t1)        # Save pointer to argv array
    ret

# Macro: get_argc
# Returns argc in a0
macro get_argc
    la t0, k_argc
    lw a0, 0(t0)
endm

# Macro: get_argv
# Input: %1 = index (0 for prog name, 1 for first arg, etc.)
# Returns: a0 = pointer to the string
macro get_argv %1
    mv t0, %1           # Get index
    slli t0, t0, 2      # index * 4 (word size)
    la t1, k_argv
    lw t1, 0(t1)        # t1 = start of argv array
    add t1, t1, t0      # t1 = address of argv[index]
    lw a0, 0(t1)        # a0 = pointer to the string
endm
