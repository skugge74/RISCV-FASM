; ==========================================================
; REPEAT LOOP
; Usage: repeat imm
; Example: 
;   repeat 5
;     LOGIC
;   endrepeat
; ==========================================================
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

; ==========================================================
; FOR RANGE LOOP
; Usage: for_range addr, start, end
; Example: 
;   for_range i, 3, 10
;     LOGIC
;   endfor_range
; ==========================================================
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


; ==========================================================
; IF CONDITION
; Usage: if conditionan_operator, reg1, reg2
; Example: 
;   if eq, t0, t1 (t0 == t1)
;     LOGIC
;   endif 
; ==========================================================
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
; Example: 
;   while lt, t0, t1  (While t0 < t1)
;     LOGIC
;   endwhile
; ==========================================================
macro while %1 %2 %3
    .while_start_%u:
        b%1 %2, %3, .while_body_%u
        j .while_end_%u
    .while_body_%u:
endm

macro endwhile
    j .while_start_%u
    
    .while_end_%u:
endm

