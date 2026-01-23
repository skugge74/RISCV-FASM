.org 0x80000000
j _start

.include "./macros/data.s"
macro loadb %1, %2
    la %1, %2       # Load the calculated address into the target register
    lb %1, 0(%1)    # Load the byte from that address
endm

macro loadbu %1, %2
    la %1, %2       # Load the calculated address into the target register
    lbu %1, 0(%1)    # Load the byte from that address
endm
macro loadh %1, %2
    la %1, %2
    lh %1, 0(%1)
endm

macro loadw %1, %2
    la %1, %2
    lw %1, 0(%1)
endm



.equ TEST, 10

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
    print_str hello 
    print_str ln
    # 1. Check .byte math (Should be 0x03)
    loadb t0, val_byte
    print_hex_reg t0
    print_str ln
    
    # 2. Check .half math (Should be 0x0014 = 20)
    loadh t1, val_half
    print_hex_reg t1
    print_str ln
    
    # 3. Check .word math (Should be 4)
    loadw t2, val_word
    print_hex_reg t2
    print_str ln
    
    # 4. Check .fill math (Should skip 10 bytes of 0xFF)
    loadbu t3, fill_check
    print_hex_reg t3   # Should be 0xAA, proving we jumped over the fill
   
    print_str ln
    print_int_imm TEST
    halt

.data
ln: .asciz "\n"
hello: .asciz "hello\n"

.align 4
val_byte: .byte 1+2          # = 3

.align 2
val_half: .half 10*2         # = 20

.align 4
mark1:
    .word 0xDEADBEEF
mark2:
val_word: .word mark2-mark1  # = 4 (Difference between addresses)

.align 4
# Fill (5+5) bytes with value 0xFF
.fill 5+5, 1, 0xFF           
fill_check: .byte 0xAA       # Marker to verify fill size
