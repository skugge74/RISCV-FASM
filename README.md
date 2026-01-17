# MACROS

This project is a High-Level Assembler. It allows you to write RISC-V code using high-level logic such as loops and conditionals
without losing the control of writing raw assembly.

- Fixed-Width Encoding: It transforms text instructions into 32-bit little-endian binary words that can run directly on a RISC-V processor or emulator.

- Scoped Symbol Table: It handles global labels (like main:) and local labels (like .loop:) using a parent-context system, preventing naming collisions.

- The Control-Flow Stack: This assembler can handle nested macros.

`make`	                    Builds the riscv-assembler executable.
`make run`	                Uses the default macros/repeat.s, assembles, and starts QEMU.
`make run FILE=filename`	Assembles and runs a specific file.
`make dump FILE=filename`   Dumps the specific file


==========================================================
        RISC-V HIGH-LEVEL MACRO DOCUMENTATION
==========================================================
This documentation covers the stack-based control flow 
macros implemented in the Kdex RISC-V Assembler.

# MACRO CONVENTION

``` asm
macro name_of_macro %1, %2, %3
    ; ... instruction templates ...
endm
```
- `%1, %2, %3`: These are placeholders for registers or values you pass when calling the macro.

- `.label_%u`: these are labels internal to the macro that allow nested macros

# LOOP MACRO

; --- Register-less Repeat Macro ---
; Usage: repeat 10
macro repeat %1
    addi sp, sp, -4        ; Allocate stack frame for counter
    li   t0, %1            ; Load literal count
    sw   t0, 0(sp)         ; Push to stack
    .rep_start_%u:
        lw   t0, 0(sp)     ; Fetch current state
        beq  t0, zero, .rep_exit_%u
endm
; Usage: endrepeat
macro endrepeat
        lw   t0, 0(sp)     ; Load counter for math
        addi t0, t0, -1    ; Decrement
        sw   t0, 0(sp)     ; Update stack
        j .rep_start_%u    ; Loop
    .rep_exit_%u:
        addi sp, sp, 4     ; Deallocate stack frame (Safety cleanup)
endm
