; --- Truly Transparent Repeat Macro ---
macro repeat %1
    addi sp, sp, -8        ; Space for [Counter] and [Backup_T0]
    sw   t0, 4(sp)         ; Step 1: Backup user's t0
    li   t0, %1            ; Load loop count
    sw   t0, 0(sp)         ; Step 2: Save counter to stack
    lw   t0, 4(sp)         ; Step 3: Restore user's t0 for their code
    .rep_start_%u:
        addi sp, sp, -4    ; Create temp space to peek at counter
        sw   t0, 0(sp)     ; Backup user's t0 AGAIN
        lw   t0, 4(sp)     ; Load counter (now at offset 4)
        beq  t0, zero, .rep_exit_%u
        lw   t0, 0(sp)     ; Restore user's t0 for the loop body
        addi sp, sp, 4     ; Clean temp space
endm

macro endrepeat
    addi sp, sp, -4        ; Temp backup to do math
    sw   t0, 0(sp)
    lw   t0, 4(sp)         ; Load counter
    addi t0, t0, -1
    sw   t0, 4(sp)         ; Save counter
    lw   t0, 0(sp)         ; Restore user's t0
    addi sp, sp, 4
    j .rep_start_%u
    .rep_exit_%u:
        lw   t0, 0(sp)     ; Final restore of t0 from the very start
        addi sp, sp, 8     ; Clean up total macro stack frame
endm

main:
 repeat 10
    addi t1, t1, 1
 endrepeat
exit:
  addi a0, 93
  addi a1, 0
  ecall
