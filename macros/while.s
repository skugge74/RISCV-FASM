; --- General While Macro ---
; Usage: while_lt reg1, reg2
macro while_lt %1, %2
    .start_%u:
        bge %1, %2, .end_%u    ; If cond is false, jump to the end
endm

; Usage: endwhile
macro endwhile
        j .start_%u            ; Jump back to the start of the loop
    .end_%u:                   ; Target for the exit branch
endm
main:
    li t0, 0
    li t1, 10
    while_lt t0, t1
        addi t0, t0, 1
    endwhile
