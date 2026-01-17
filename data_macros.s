.org 0x80000000

# --- 1. MACRO DEFINITIONS (Global) ---
macro halt
  .halt_%u:
    j .halt_%u
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
la s0, %1   # Pointer reset inside loop
  .print_loop_%u:
    lb s2, 0(s0)
    beqz s2, .print_done_%u
    sb s2, 0(s1)
    addi s0, s0, 1
    j .print_loop_%u
  .print_done_%u:
endm

# --- 2. CODE SECTION ---
.text
_start:
    li sp, 0x80100000   # Initialize Stack
    li s1, 0x10000000   # UART Base

repeat 10
  print_str msg    
endrepeat

halt

# --- 3. DATA SECTION ---
.data
.align 4
msg:
    .asciz "Hello, RISC-V FASM!\n"
