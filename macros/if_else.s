; --- Transparent If-Equal ---
macro if_eq %1, %2
    addi sp, sp, -8        ; [0:Backup1, 4:Backup2]
    sw   %1, 0(sp)
    sw   %2, 4(sp)
    
    lw   t0, 0(sp)
    lw   t1, 4(sp)
    ; If NOT equal, jump to the ELSE block
    bne  t0, t1, .if_false_%u
    
    ; --- TRUE PATH ---
    lw   %1, 0(sp)         ; Restore user registers
    lw   %2, 4(sp)
    addi sp, sp, 8         ; Clean stack for the True code
endm

; --- Else ---
macro else
    ; If we reached the end of the TRUE block, skip the ELSE block
    j .if_end_%u           
    
    .if_false_%u:          ; Target for the initial BNE jump
    addi sp, sp, 8         ; Clean stack for the False code
endm

; --- Endif ---
macro endif
    .if_false_%u:          ; Target if no 'else' exists
    .if_end_%u:            ; Final target for skipping 'else'
endm

main:
    li t0, 10
    li t1, 20

    if_eq t0, t1
        addi t6, zero, 1
        ; This code won't run
        ;repeat 5
        ;    addi a0, a0, 1
        ;endrepeat
    else
      addi t6, zero, 2
        ; This code WILL run
        ;for_range 0, 3
        ;    addi a1, a1, 1
        ;endfor
    endif

exit:
    ret
