.org 0x80000000
j _start

.include "../macros/data.s"  

; ==========================================================
; MACRO: SUM_6
; Arguments: 7 Total (1 Destination + 6 Values)
; Usage: SUM_6 dest, v1, v2, v3, v4, v5, v6
; ==========================================================
macro SUM_6 %1, %2, %3, %4, %5, %6, %7
    li %1, %2          ; Load v1
    addi %1, %1, %3    ; Add v2
    addi %1, %1, %4    ; Add v3
    addi %1, %1, %5    ; Add v4 (Old parser would stop here!)
    addi %1, %1, %6    ; Add v5 (New feature)
    addi %1, %1, %7    ; Add v6 (New feature)
endm

.text
.align 4
_start:
    ; Test Case: 10 + 20 + 30 + 40 + 50 + 60
    ; Expected Result: 210 (0xD2 in Hex)
    li sp, 0x80100000
    li s1, 0x10000000
    
    SUM_6 t0, 10, 20, 30, 40, 50, 60
    
    ; Verify the result
    ; If you don't have a print macro, we will just use a specific
    ; check that hangs if correct and loops if wrong.
    
    li t1, 210         ; Expected value
    beq t0, t1, .success
    
    ; Failure Case: Loop forever here
.fail:
    j .fail

.success:
  print_str success
    ; Success Case: Clean shutdown (QEMU magic)
    li t0, 0x100000
    li t1, 0x5555
    sw t1, 0(t0)
qemu_off
.data
success: .asciz "succes ! \n"
