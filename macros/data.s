macro halt
  .halt_%u:
    j .halt_%u
endm

macro init_uart
    li sp, 0x80100000
    li s1, 0x10000000
endm
; Usage: MAX dest, val_a, val_b
macro max %1, %2, %3
    ; Check if val_a (%2) < val_b (%3)
    blt %2, %3, .pick_b_%u
    
    ; Case A: val_a is larger (or equal)
    mv %1, %2
    j .done_%u

    ; Case B: val_b is larger
    .pick_b_%u:
    mv %1, %3

    .done_%u:
endm

macro repeat %1
    addi sp, sp, -4
    li   t0, %1
    sw   t0, 0(sp)
    .rep_start_%u:
        lw   t0, 0(sp)
        beqz t0, .rep_exit_%u
endm

macro endrepeat
        lw   t0, 0(sp)
        addi t0, t0, -1
        sw   t0, 0(sp)
        j .rep_start_%u
    .rep_exit_%u:
        addi sp, sp, 4
endm

macro print_str %1
la s0, %1   
  .print_loop_%u:
    lb s2, 0(s0)
    beqz s2, .print_done_%u
    sb s2, 0(s1)
    addi s0, s0, 1
    j .print_loop_%u
  .print_done_%u:
endm


# --- Print Immediate (e.g. print_int_imm 1234) ---
macro print_int_imm %1
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)

    li t0, %1           # <--- DIFFERENCE: Uses LI
    li t1, 10
    li t3, 0

    # Handle Zero
    bnez t0, .imm_neg_%u
    li t2, 48
    sb t2, 0(s1)
    j .imm_done_%u

.imm_neg_%u:
    # Handle Negative
    bge t0, zero, .imm_extract_%u
    li t2, 45
    sb t2, 0(s1)
    sub t0, zero, t0

.imm_extract_%u:
    beqz t0, .imm_print_%u
    remu t2, t0, t1
    divu t0, t0, t1     # <--- FIXED: t1 not t
    addi t2, t2, 48
    addi sp, sp, -4
    sw t2, 0(sp)
    addi t3, t3, 1
    j .imm_extract_%u

.imm_print_%u:
    beqz t3, .imm_done_%u
    lw t2, 0(sp)
    addi sp, sp, 4
    sb t2, 0(s1)
    addi t3, t3, -1
    j .imm_print_%u

.imm_done_%u:
    lw t3, 12(sp)
    lw t2, 8(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 16
endm


# --- Print Register (e.g. print_int_reg t0) ---
macro print_int_reg %1
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)

    mv t0, %1           # <--- DIFFERENCE: Uses MV
    li t1, 10
    li t3, 0

    # Handle Zero
    bnez t0, .reg_neg_%u
    li t2, 48
    sb t2, 0(s1)
    j .reg_done_%u

.reg_neg_%u:
    # Handle Negative
    bge t0, zero, .reg_extract_%u
    li t2, 45
    sb t2, 0(s1)
    sub t0, zero, t0

.reg_extract_%u:
    beqz t0, .reg_print_%u
    remu t2, t0, t1
    divu t0, t0, t1     # <--- FIXED: t1 not t
    addi t2, t2, 48
    addi sp, sp, -4
    sw t2, 0(sp)
    addi t3, t3, 1
    j .reg_extract_%u

.reg_print_%u:
    beqz t3, .reg_done_%u
    lw t2, 0(sp)
    addi sp, sp, 4
    sb t2, 0(s1)
    addi t3, t3, -1
    j .reg_print_%u

.reg_done_%u:
    lw t3, 12(sp)
    lw t2, 8(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 16
endm
# --- Print Hexadecimal (e.g. print_hex t0) ---
macro print_hex_reg %1
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    
    mv t0, %1           # Value to print
    li t1, 28           # Shift amount (start with top nibble)

.hex_loop_%u:
    srl t2, t0, t1      # Shift right to get the nibble to position 0
    andi t2, t2, 0xF    # Mask out everything except the last 4 bits
    
    li t3, 10
    blt t2, t3, .is_digit_%u
    
    addi t2, t2, 55     # Convert 10-15 to 'A'-'F' (65-70)
    j .print_char_%u

.is_digit_%u:
    addi t2, t2, 48     # Convert 0-9 to '0'-'9' (48-57)

.print_char_%u:
    sb t2, 0(s1)        # Print to UART
    
    addi t1, t1, -4     # Decrease shift by 4 bits
    bge t1, zero, .hex_loop_%u

    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 8
endm
# --- Print Hexadecimal (e.g. print_hex t0) ---
macro print_hex_imm %1
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    
    li t0, %1           # Value to print
    li t1, 28           # Shift amount (start with top nibble)

.hex_loop_%u:
    srl t2, t0, t1      # Shift right to get the nibble to position 0
    andi t2, t2, 0xF    # Mask out everything except the last 4 bits
    
    li t3, 10
    blt t2, t3, .is_digit_%u
    
    addi t2, t2, 55     # Convert 10-15 to 'A'-'F' (65-70)
    j .print_char_%u

.is_digit_%u:
    addi t2, t2, 48     # Convert 0-9 to '0'-'9' (48-57)

.print_char_%u:
    sb t2, 0(s1)        # Print to UART
    
    addi t1, t1, -4     # Decrease shift by 4 bits
    bge t1, zero, .hex_loop_%u

    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 8
endm

macro get_char %1
    li t5, 0x10000000
    .wait_loop_%u:
        lb t6, 5(t5)
        andi t6, t6, 1
        beqz t6, .wait_loop_%u
    lb %1, 0(t5)
endm

.data
.align 4
ln: .asciz "\n" 
