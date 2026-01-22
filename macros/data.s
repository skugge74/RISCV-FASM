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
    blt %2, %3, .pick_b_%u
    mv %1, %2
    j .done_%u
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
# --- Print Hexadecimal (e.g. print_hex t0) ---
macro print_hex_reg %1
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    
    mv t0, %1          
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

    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 8
endm
# --- Print Hexadecimal (e.g. print_hex t0) ---
macro print_hex_imm %1
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    
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


macro for %1, %2, %3
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    %2:
        la t1, %1
        lw t0, 0(t1)
        li t2, 10
        beq t0, t2, %3
endm

macro endfor %1, %2, %3
    la t1, %1
    lw t0, 0(t1)
    addi t0, t0, 1
    sw t0, 0(t1)
    j %2
    %3:
        lw t1, 4(sp)
        lw t0, 0(sp)
        addi sp, sp, 8
endm

macro for_range %1, %2, %3
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    
    la t0, %1  
    li t1, %2
    sw t1, 0(t0)
    .loop_%u:
        la t1, %1
        lw t0, 0(t1)
        li t2, %3
        
        beq t0, t2, .end_%u
endm

macro endfor_range %1

    la t1, %1
    lw t0, 0(t1)
    addi t0, t0, 1
    sw t0, 0(t1)
    
    j .loop_%u
    
    .end_%u:
        lw t1, 4(sp)
        lw t0, 0(sp)
        addi sp, sp, 8
endm

; Usage: if condition, reg1, reg2
macro if %1 %2 %3
    b%1 %2, %3, .if_body_%u
    j .if_end_%u
    .if_body_%u:
endm

macro endif
    .if_end_%u:
endm

; ==========================================================
; WHILE LOOP
; Usage: while condition, reg1, reg2
; Example: while lt, t0, t1  (While t0 < t1)
; ==========================================================
macro while %1 %2 %3
    .while_start_%u:
        ; Check condition. If True, go to body.
        b%1 %2, %3, .while_body_%u
        
        ; If False, skip to the end (Exit Loop)
        j .while_end_%u
        
    .while_body_%u:
endm

; ==========================================================
; END WHILE
; Usage: endwhile
; ==========================================================
macro endwhile
    ; Jump back to the start to check condition again
    j .while_start_%u
    
    .while_end_%u:
endm

.data
.align 4
ln: .asciz "\n" 
space: .asciz " " 
