# ==========================================
# Kdex Dynamic Memory Allocator (kalloc.s)
# ==========================================
.equ SYS_BRK, 214

.data
.align 4
heap_start: .word 0
heap_end:   .word 0

# ------------------------------------------
# User API Macros
# ------------------------------------------

# Usage: kmalloc [size] -> Returns pointer in a0
macro kmalloc %1
    mv a0, %1
    call _kmalloc
endm

# Usage: kfree [pointer]
macro kfree %1
    mv a0, %1
    call _kfree
endm

# ------------------------------------------
# Internal Allocator Functions
# ------------------------------------------
.text
.align 4

# Function: _kmalloc
# Input:  a0 = requested size
# Output: a0 = pointer to memory (or 0 if out of memory)
.global _kmalloc
_kmalloc:
    # 1. Align requested size to 4 bytes (clear bottom 2 bits)
    addi a0, a0, 3
    srli a0, a0, 2
    slli a0, a0, 2
    mv t0, a0              # t0 = aligned requested size

    # 2. Check if heap is initialized
    la t1, heap_start
    lw t2, 0(t1)
    bnez t2, .L_search     # If heap_start != 0, start searching!

    # Initialize Heap: call brk(0)
    addi sp, sp, -16
    sw t0, 0(sp)           # Save requested size
    sw ra, 4(sp)
    
    li a0, 0
    li a7, SYS_BRK
    ecall                  # Get current program break
    
    la t1, heap_start
    sw a0, 0(t1)           # heap_start = brk
    la t1, heap_end
    sw a0, 0(t1)           # heap_end = brk
    
    lw ra, 4(sp)
    lw t0, 0(sp)           # Restore requested size
    addi sp, sp, 16

.L_search:
    la t1, heap_start
    lw t2, 0(t1)           # t2 = Current Block Pointer
    la t1, heap_end
    lw t3, 0(t1)           # t3 = End of Heap

.L_loop:
    bge t2, t3, .L_extend  # If Current >= End, we need more memory from OS

    lw t4, 0(t2)           # t4 = Block Size
    lw t5, 4(t2)           # t5 = is_free flag

    # Is it free AND large enough?
    beqz t5, .L_next_block # If is_free == 0, skip
    blt t4, t0, .L_next_block # If block_size < requested_size, skip

    # Found a reusable block!
    sw zero, 4(t2)         # Set is_free = 0
    addi a0, t2, 8         # Return pointer = block + 8 (skip header)
    ret

.L_next_block:
    addi t2, t2, 8         # Skip header
    add t2, t2, t4         # Skip payload (curr = curr + 8 + size)
    j .L_loop

.L_extend:
    # Need to ask OS for more memory
    # New Break = heap_end + 8 (header) + requested_size
    addi t4, t0, 8         # t4 = 8 + size
    add a0, t3, t4         # a0 = heap_end + header + size
    
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t3, 4(sp)           # Save old heap_end
    sw ra, 8(sp)

    li a7, SYS_BRK
    ecall                  # sys_brk(new_break)

    lw ra, 8(sp)
    lw t3, 4(sp)           # t3 = old heap_end
    lw t0, 0(sp)           # t0 = size
    addi sp, sp, 16

    # Check for Out Of Memory (if brk didn't change)
    beq a0, t3, .L_oom

    # Update heap_end to new break
    la t1, heap_end
    sw a0, 0(t1)

    # Write Metadata Header at old heap_end
    sw t0, 0(t3)           # Store size
    sw zero, 4(t3)         # Store is_free = 0
    
    addi a0, t3, 8         # Return pointer = old heap_end + 8
    ret

.L_oom:
    li a0, 0               # Return NULL
    ret

# Function: _kfree
# Input:  a0 = pointer to free
.global _kfree
_kfree:
    beqz a0, .L_free_done  # If ptr is NULL, do nothing
    addi a0, a0, -8        # Step back 8 bytes to hit the Header
    li t0, 1
    sw t0, 4(a0)           # Set is_free = 1
.L_free_done:
    ret
