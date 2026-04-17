# --- Halt system (e.g. halt) ---
macro halt
  .halt_%u:
    j .halt_%u
endm

# --- Initialize Uart (e.g. init_uart) ---
macro init_uart
    li sp, 0x80100000
    li s1, 0x10000000
endm

macro qemu_off
    li t0, 0x100000     ; Address of QEMU "SiFive Test" device
    li t1, 0x5555       ; 0x5555 = Power Off (Pass)
    sw t1, 0(t0)        ; Write to address -> QEMU closes instantly
endm

; ==========================================================
; MAXIMUM of 2 registers
; Usage: max dst, src1, src2
; Example: max t2, t0, t1  (t2 <- (t0 > t1) ? t0 : t1))
; ==========================================================
macro max %1, %2, %3
    blt %2, %3, .pick_b_%u
    mv %1, %2
    j .done_%u
    .pick_b_%u:
    mv %1, %3
    .done_%u:
endm

macro print_str %1
    addi sp, sp, -8
    sw t0, 0(sp)
    sw t1, 4(sp)
    la t0, %1   
  .print_loop_%u:
    lb t1, 0(t0)            # Use t1 instead of s2
    beqz t1, .print_done_%u
    sb t1, 0(s1)            # s1 is UART base
    addi t0, t0, 1
    j .print_loop_%u
  .print_done_%u:
    lw t1, 4(sp)
    lw t0, 0(sp)
    addi sp, sp, 8
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


; ==========================================================
; GET STRING
; Usage: get_string buffer_addr, max_length
; Reads characters until 'Enter' (Newline) is pressed.
; ==========================================================
macro get_string %1, %2
    li t3, 0             ; t3 = Counter (i)
    la t4, %1            ; t4 = Buffer Pointer
    li t5, %2            ; t5 = Max Length

    .loop_%u:
        ; Get one character
        get_char t0      
        
        ; Check for Newline (Enter key = 10 '\n' or 13 '\r')
        li t1, 10
        beq t0, t1, .done_%u
        li t1, 13
        beq t0, t1, .done_%u
        
        ; Echo char back to screen (so user sees what they type!)
        sb t0, 0(s1)     ; s1 is UART Base from init_uart
        
        ; Store in buffer: buffer[i] = char
        add t2, t4, t3   ; Address = Base + i
        sb t0, 0(t2)
        
        addi t3, t3, 1   ; i++
        
        ; Safety: Don't overflow buffer
        blt t3, t5, .loop_%u

    .done_%u:
        ; Null-terminate the string (important for strcmp!)
        add t2, t4, t3
        sb zero, 0(t2)
        
        ; Print a newline so the shell reply is on the next line
        li t1, 10
        sb t1, 0(s1)
endm

; ==========================================================
; STRING COMPARE (strcmp)
; Usage: strcmp str1_addr, str2_addr, result_reg
; ==========================================================
macro strcmp %1, %2, %3
    la t0, %1           ; Ptr 1
    la t1, %2           ; Ptr 2
    
    .loop_%u:
        lb t2, 0(t0)    ; Load char from Str1
        lb t3, 0(t1)    ; Load char from Str2
        
        ; If chars are different, stop -> Return 1
        bne t2, t3, .diff_%u
        
        ; If we hit NULL (0) and they were equal, we are done -> Return 0
        beqz t2, .same_%u
        
        addi t0, t0, 1
        addi t1, t1, 1
        j .loop_%u

    .diff_%u:
        li %3, 1        ; Result = 1 (Different)
        j .done_%u
    .same_%u:
        li %3, 0        ; Result = 0 (Same)
    .done_%u:
endm


.data
.align 4
ln: .asciz "\n" 
space: .asciz " " 
