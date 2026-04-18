.text
.global _start

.include "../macros/stdlib/kstdio.s"
.include "../macros/stdlib/kalloc.s"
.include "../macros/stdlib/kstring.s"

_start:
    # 1. First Allocation (16 bytes)
    li t0, 16
    kmalloc t0
    mv s0, a0              # Save Pointer A to s0

    # 2. Write "Kdex" into Pointer A
    li t1, 'K'
    sb t1, 0(s0)
    li t1, 'd'
    sb t1, 1(s0)
    li t1, 'e'
    sb t1, 2(s0)
    li t1, 'x'
    sb t1, 3(s0)
    li t1, '\n'
    sb t1, 4(s0)

    # Print it
    li a1, 5
    print s0, a1

    # 3. FREE the memory!
    kfree s0

    # 4. Second Allocation (16 bytes)
    li t0, 16
    kmalloc t0
    mv s1, a0              # Save Pointer B to s1

    # 5. Proof of Memory Reuse!
    # Because we freed Pointer A, the allocator should have given us 
    # the exact same address for Pointer B. 
    # If s0 == s1, our Free-List works!
    
    bne s0, s1, .L_fail
    
    # Write "Wins" into the recycled memory
    li t1, 'W'
    sb t1, 0(s1)
    li t1, 'i'
    sb t1, 1(s1)
    li t1, 'n'
    sb t1, 2(s1)
    li t1, 's'
    sb t1, 3(s1)
    li t1, '\n'
    sb t1, 4(s1)

    li a1, 5
    print s1, a1

    exit 0

.L_fail:
    exit 1
