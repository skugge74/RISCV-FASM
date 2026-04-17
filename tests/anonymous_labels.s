.org 0x80000000
j _start
.include "./macros/data.s" 

.text
_start:
    li sp, 0x80100000
    li s1, 0x10000000
    
    li s2, 0      # Outer counter
    li s3, 3      # Outer limit
    
_outer_loop:      # GLOBAL ANCHOR
    print_str ln
    print_int_imm 10
    
    li s4, 0      # Inner counter
    li s5, 2      # Inner limit
    
.inner:           # LOCAL SCOPE (Belongs to _outer_loop)
    print_str space
    print_int_imm 1
    
    addi s4, s4, 1
    blt  s4, s5, .inner       # Target is specific: Inner Loop
    
    addi s2, s2, 1
    blt  s2, s3, _outer_loop  # Target is specific: Outer Loop

    # --- Use @@ only for micro-skips ---
    beq x0, x0, @f
    print_int_imm 999 
@@:
    print_str ln
    print_int_imm 20  
    qemu_off
