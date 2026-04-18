# macros.s
macro sum_array %1 %2 %3
    # %1 = array base address (e.g., a0)
    # %2 = length of array (e.g., a1)
    # %3 = accumulator register (e.g., a2)
    
    li %3, 0             # Clear accumulator
    li t0, 0             # Loop counter
    
.loop_start_%u:
    bge t0, %2, .loop_end_%u
    
    slli t1, t0, 2       # offset = counter * 4 bytes
    add t2, %1, t1       # memory address of current element
    lw t3, 0(t2)         # load element from memory
    
    add %3, %3, t3       # Add to accumulator
    addi t0, t0, 1       # Increment counter
    j .loop_start_%u
    
.loop_end_%u:
endm
