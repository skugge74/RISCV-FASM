# ==========================================================
# SYSTEM MACROS
# ==========================================================

# --- Halt system ---
macro halt
  .halt_%u:
    j .halt_%u
endm

# --- Initialize UART ---
macro init_uart
    li sp, 0x80100000
    li s1, 0x10000000
endm

# --- Shut down QEMU ---
macro qemu_off
    li t0, 0x100000         # Address of QEMU "SiFive Test" device
    li t1, 0x5555           # 0x5555 = Power Off (Pass)
    sw t1, 0(t0)            # Write to address -> QEMU closes instantly
endm

# ==========================================================
# MATH & LOGIC MACROS
# ==========================================================

# --- MAXIMUM of 2 registers ---
# Usage: max dst, src1, src2
# Example: max t2, t0, t1  (t2 <- (t0 > t1) ? t0 : t1)
macro max %1, %2, %3
    blt %2, %3, .pick_b_%u
    mv %1, %2
    j .done_%u
  .pick_b_%u:
    mv %1, %3
  .done_%u:
endm

# ==========================================================
# PRINTING MACROS
# ==========================================================

# --- Print String ---
macro print_str %1
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    
    la t0, %1   
  .print_loop_%u:
    lb t1, 0(t0)            
    beqz t1, .print_done_%u
    sb t1, 0(s1)            # s1 is UART base
    addi t0, t0, 1
    j .print_loop_%u
    
  .print_done_%u:
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 8
endm

# --- Print Integer (Immediate) ---
macro print_int_imm %1
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)

    li t0, %1            
    li t1, 10
    li t3, 0

    bnez t0, .imm_neg_%u
    li t2, 48
    sb t2, 0(s1)
    j .imm_done_%u

  .imm_neg_%u:
    bge t0, zero, .imm_extract_%u
    li t2, 45
    sb t2, 0(s1)
    sub t0, zero, t0

  .imm_extract_%u:
    beqz t0, .imm_print_%u
    remu t2, t0, t1
    divu t0, t0, t1      
    addi t2, t2, 48
    addi sp, sp, -4         # Dynamically push digits to stack
    sw t2, 0(sp)
    addi t3, t3, 1
    j .imm_extract_%u

  .imm_print_%u:
    beqz t3, .imm_done_%u
    lw t2, 0(sp)
    addi sp, sp, 4          # Pop digits from stack
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

# --- Print Integer (Register) ---
macro print_int_reg %1
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)

    mv t0, %1          
    li t1, 10
    li t3, 0
    
    bnez t0, .reg_neg_%u
    li t2, 48
    sb t2, 0(s1)
    j .reg_done_%u

  .reg_neg_%u:
    bge t0, zero, .reg_extract_%u
    li t2, 45
    sb t2, 0(s1)
    sub t0, zero, t0

  .reg_extract_%u:
    beqz t0, .reg_print_%u
    remu t2, t0, t1
    divu t0, t0, t1 
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

# --- Print Hexadecimal (Register) ---
macro print_hex_reg %1
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)
    
    mv t0, %1          
    li t1, 28          

  .hex_loop_%u:
    srl t2, t0, t1   
    andi t2, t2, 0xF
    
    li t3, 10
    blt t2, t3, .is_digit_%u
    
    addi t2, t2, 55         # 'A' - 10
    j .print_char_%u

  .is_digit_%u:
    addi t2, t2, 48         # '0'

  .print_char_%u:
    sb t2, 0(s1)      
    addi t1, t1, -4  
    bge t1, zero, .hex_loop_%u

    lw t3, 12(sp)
    lw t2, 8(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 16
endm

# --- Print Hexadecimal (Immediate) ---
macro print_hex_imm %1
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)
    
    li t0, %1          
    li t1, 28         

  .hex_loop_%u:
    srl t2, t0, t1   
    andi t2, t2, 0xF 
    
    li t3, 10
    blt t2, t3, .is_digit_%u
    
    addi t2, t2, 55 
    j .print_char_%u

  .is_digit_%u:
    addi t2, t2, 48  

  .print_char_%u:
    sb t2, 0(s1)    
    addi t1, t1, -4
    bge t1, zero, .hex_loop_%u

    lw t3, 12(sp)
    lw t2, 8(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 16
endm

# ==========================================================
# INPUT MACROS
# ==========================================================

# --- Get Single Character ---
# Note: Do not pass t5 or t6 as the target register.
macro get_char %1
    addi sp, sp, -8
    sw t5, 0(sp)
    sw t6, 4(sp)

    li t5, 0x10000000
  .wait_loop_%u:
    lb t6, 5(t5)            # Read LSR (Line Status Register)
    andi t6, t6, 1          # Check Data Ready bit
    beqz t6, .wait_loop_%u
    lb %1, 0(t5)            # Read character
    
    lw t6, 4(sp)
    lw t5, 0(sp)
    addi sp, sp, 8
endm

# --- Get String ---
# Usage: get_string buffer_addr, max_length
# Reads characters until 'Enter' (Newline) is pressed.
macro get_string %1, %2
    addi sp, sp, -24
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)
    sw t4, 16(sp)
    sw t5, 20(sp)

    li t3, 0                # t3 = Counter (i)
    la t4, %1               # t4 = Buffer Pointer
    li t5, %2               # t5 = Max Length

  .loop_%u:
    get_char t0      
    
    # Check for Newline (10 '\n' or 13 '\r')
    li t1, 10
    beq t0, t1, .done_%u
    li t1, 13
    beq t0, t1, .done_%u
    
    # Echo char back to screen
    sb t0, 0(s1)         
    
    # Store in buffer: buffer[i] = char
    add t2, t4, t3   
    sb t0, 0(t2)
    
    addi t3, t3, 1          # i++
    blt t3, t5, .loop_%u    # Prevent buffer overflow

  .done_%u:
    # Null-terminate the string
    add t2, t4, t3
    sb zero, 0(t2)
    
    # Print a newline
    li t1, 10
    sb t1, 0(s1)

    lw t5, 20(sp)
    lw t4, 16(sp)
    lw t3, 12(sp)
    lw t2, 8(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 24
endm

# ==========================================================
# STRING COMPARE (strcmp)
# Usage: strcmp str1_addr, str2_addr, result_reg
# ==========================================================
macro strcmp %1, %2, %3
    addi sp, sp, -16
    sw t0, 0(sp)
    sw t1, 4(sp)
    sw t2, 8(sp)
    sw t3, 12(sp)

    la t0, %1               # Ptr 1
    la t1, %2               # Ptr 2
    
  .loop_%u:
    lb t2, 0(t0)            # Load char from Str1
    lb t3, 0(t1)            # Load char from Str2
    
    bne t2, t3, .diff_%u    # Different -> Return 1
    beqz t2, .same_%u       # Both NULL -> Return 0
    
    addi t0, t0, 1
    addi t1, t1, 1
    j .loop_%u

  .diff_%u:
    # Pop stack FIRST
    lw t3, 12(sp)
    lw t2, 8(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 16
    
    li %3, 1                # THEN set Result = 1
    j .done_%u
    
  .same_%u:
    # Pop stack FIRST
    lw t3, 12(sp)
    lw t2, 8(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 16
    
    li %3, 0                # THEN set Result = 0
    
  .done_%u:
endm
# ==========================================================
# DATA SECTION
# ==========================================================
.data
.align 4
ln: .asciz "\n" 
space: .asciz " "
