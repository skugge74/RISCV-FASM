.org 0x80000000

j _start
.include "../macros/data.s" 

.text
_start:
    li sp, 0x80100000
    li s1, 0x10000000  # UART Base

    # --- Test 1: Simple Forward Jump ---
    print_int_imm 1
    j @f
    print_int_imm 999  # SKIPPED
@@:
    print_str space

    # --- Test 2: Backward Jump (Loop) ---
    li a0, 0
@@:
    addi a0, a0, 1
    li   t0, 3
    blt  a0, t0, @b    # Loop until a0 == 3
    print_int_imm 3    # Should print '3'
    print_str space

    # --- Test 3: Nested/Multiple Anonymous ---
    beq x0, x0, @f
    print_int_imm 888  # SKIPPED
@@:
    print_int_imm 5    # Should print '5'
    
    # --- Test 4: Address Math Verification ---
    # Using 'jalr' instead of 'jr' to match your encoder
    la t1, @f
    jalr x0, t1, 0     # Jump to the @@ below (jalr rd, rs1, imm)
    print_int_imm 777  # SKIPPED
@@:
    
    print_str ln       # Changed from 'nl' to 'ln'
    qemu_off
