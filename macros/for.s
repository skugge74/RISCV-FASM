; --- Fixed Transparent For-Range (A to B) ---
macro for_range %1, %2
    addi sp, sp, -16       ; [0:Counter, 4:Limit, 8:SavedT0, 12:SavedT1]
    sw   t0, 8(sp)         ; Step 1: Backup user registers
    sw   t1, 12(sp)
    
    li   t0, %1            ; Step 2: Set Counter to 'A'
    sw   t0, 0(sp)
    
    li   t0, %2            ; Step 3: Set Limit to 'B'
    sw   t0, 4(sp)
    
    .for_top_%u:
        lw   t0, 0(sp)     ; Load current counter
        lw   t1, 4(sp)     ; Load the limit
        bge  t0, t1, .for_exit_%u
        
        lw   t0, 8(sp)     ; Step 4: RESTORE user T0/T1 for their code
        lw   t1, 12(sp)
endm

macro endfor
        sw   t0, 8(sp)     ; Step 5: SAVE user T0/T1 before math
        sw   t1, 12(sp)
        
        lw   t0, 0(sp)     ; Increment logic
        addi t0, t0, 1
        sw   t0, 0(sp)
        
        j .for_top_%u      ; Jump back
        
    .for_exit_%u:
        lw   t0, 8(sp)     ; Final restore
        lw   t1, 12(sp)
        addi sp, sp, 16    ; Pop frame
endm
main:
    li t2, 0       ; Our result register
    li t1, 10      ; Our limit
    
    for_range 3, t1
        addi t2, t2, 1
    endfor

    ; At this point, t2 should be 7 (10 - 3)
    ; And sp should be exactly what it was at the start of main
    li a7, 93
    li a0, 0
    ecall
