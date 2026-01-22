# ==========================================
# RISC-V ASSEMBLER TEST SUITE V2
# Focus: .equ, Bitwise Math, Load Pseudos
# ==========================================
.org 0x80000000
.text
.global _start

# --------------------------------------
# TEST 1: Bitwise Constants & Precedence
# --------------------------------------
# (1 << 3) = 8
.equ BIT_3,  (1 << 3)  
# (1 << 4) = 16
.equ BIT_4,  (1 << 4)  
# (8 | 16) = 24 (0x18)
.equ MASK_A, (BIT_3 | BIT_4) 

# Precedence Check: 
# In C/Assembler, '+' binds tighter than '<<'
# 1 << 2 + 1  should be  1 << (2 + 1) = 8
# NOT (1 << 2) + 1 = 5
.equ PREC_TEST, 1 << 2 + 1 

_start:
    # --------------------------------------
    # TEST 2: Verify .equ Values
    # --------------------------------------
    li t0, MASK_A      # Should be 0x18 (24)
    li t1, PREC_TEST   # Should be 0x08 (8)
    
    # --------------------------------------
    # TEST 3: Pseudo-Instructions (Global Loads)
    # --------------------------------------
    # These instructions now auto-expand into 3 instructions:
    # lui + addi + lb/lh/lw
    
    lb  s0, byte_val   # Load 0xAA (sign-extended -> 0xFFFFFFAA)
    lbu s1, byte_val   # Load 0xAA (zero-extended -> 0x000000AA)
    
    lh  s2, half_val   # Load 0x1234
    lw  s3, word_val   # Load 0xDEADBEEF

    # --------------------------------------
    # TEST 4: Inline Bitwise Math
    # --------------------------------------
    # You should be able to do math inside instructions too
    li t2, (0xF0 & 0x30) # Should be 0x30 (48)
    li t3, (0x0F ^ 0xFF) # Should be 0xF0 (240)

    # Halt loop
    ebreak
    j _start

# ==========================================
# DATA SECTION
# ==========================================
.data
.align 4
byte_val: .byte 0xAA
.align 2
half_val: .half 0x1234
.align 4
word_val: .word 0xDEADBEEF
