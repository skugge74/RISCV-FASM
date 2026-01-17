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
la s0, %1   
  .print_loop_%u:
    lb s2, 0(s0)
    beqz s2, .print_done_%u
    sb s2, 0(s1)
    addi s0, s0, 1
    j .print_loop_%u
  .print_done_%u:
endm
