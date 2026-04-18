# Focus: .equ, Bitwise Math, Load Pseudos
.org 0x80000000
j _start

.include "../macros/data.s"

macro loadb %1, %2
    la %1, %2    
    lb %1, 0(%1)  
endm

macro loadbu %1, %2
    la %1, %2       
    lbu %1, 0(%1)  
endm
macro loadh %1, %2
    la %1, %2
    lh %1, 0(%1)
endm

macro loadw %1, %2
    la %1, %2
    lw %1, 0(%1)
endm



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

.text
_start:
  init_uart
    # --------------------------------------
    # TEST 2: Verify .equ Values
    # --------------------------------------
    li t0, MASK_A      # Should be 0x18 (24)
    li t1, PREC_TEST   # Should be 0x08 (8)
    print_int_reg t0 
    print_str ln
    print_int_reg t1 
    print_str ln
    # --------------------------------------
    # TEST 3: Pseudo-Instructions (Global Loads)
    # --------------------------------------
    
    loadb  s0, byte_val   # Load 0xAA (sign-extended -> 0xFFFFFFAA)
    print_hex_reg s0 
    print_str ln

    loadbu s2, byte_val   # Load 0xAA (zero-extended -> 0x000000AA)
    print_hex_reg s2 
    print_str ln

    loadh  s3, half_val   # Load 0x1234
    print_hex_reg s3 
    print_str ln

    loadw  s4, word_val   # Load 0xDEADBEEF
    print_hex_reg s4 
    print_str ln


    # --------------------------------------
    # TEST 4: Inline Bitwise Math
    # --------------------------------------
    # You should be able to do math inside instructions too
    li t2, (0xF0 & 0x30) # Should be 0x30 (48)
    li t3, (0x0F ^ 0xFF) # Should be 0xF0 (240)
   
    print_int_reg t2 
    print_str ln
    print_int_reg t3 
    print_str ln

    # Halt loop
    qemu_off

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
