# ==========================================
# RISC-V ASSEMBLER TEST SUITE
# ==========================================
.org 0x80000000

j _start
.include "../macros/data.s"

.text
_start:
    # --------------------------------------
    # TEST 1: Basic Arithmetic & Registers
    # --------------------------------------
    li  t0, 10          # Valid: t0 maps to x5
    li  t2, -5          # Valid: t2 maps to x7
    add t1, t0, t2      # t1 (x6) = 10 + (-5) = 5
    
    # --------------------------------------
    # TEST 2: Large Immediates (Pseudo-op split)
    # --------------------------------------
    # Value 0x12345678 requires LUI + ADDI
    # 0x12345 = 74565 (upper 20 bits)
    # 0x678   = 1656  (lower 12 bits)
    li  s0, 0x12345678  

    # Value 4096 (0x1000) is just on the edge
    li  s1, 4096       

    # --------------------------------------
    # TEST 3: Local Labels & Branching
    # --------------------------------------
    li  a0, 5
    li  a1, 0

.loop:
    add a1, a1, a0      # Accumulate
    addi a0, a0, -1     # Decrement
    bnez a0, .loop      # Branch back if not zero

    # --------------------------------------
    # TEST 4: ERROR TRAPS (Uncomment to test)
    # --------------------------------------
    
    # ERROR A: Invalid Register 't7'
    # Expected: [ERROR] ... Undefined symbol: 't7'
     li s20, 100        

    # ERROR B: Undefined Label
    # Expected: [ERROR] ... Undefined symbol: 'nowhere'
     j nowhere

    # ERROR C: Invalid Register Index (x32)
    # Expected: [ERROR] ... Undefined symbol: 'x32'
     add x32, x1, x2

    # --------------------------------------
    # FINISH
    # --------------------------------------
    qemu_off
