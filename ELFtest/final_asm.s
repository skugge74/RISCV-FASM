# final_asm.s
.global process_data
.global multipliers
.extern main

.include "macros.s"

# --- WRITABLE DATA SECTION ---
.data
.align 4
multipliers:
    .word 2
    .word 5    # We want this one! (multipliers + 4)
    .word 10

# --- EXECUTABLE CODE SECTION ---
.text
.global _start
_start:
    call main       # Call the C main function
    mv a0, a0       # The exit code is already in a0 from main!
    li a7, 93       # Syscall: exit
    ecall           # Tell QEMU to shut down safely
.align 4
process_data:
    # C passes arguments in a0 and a1
    # a0 = integer array pointer
    # a1 = array length

    # 1. Use MACRO to sum the array into a2
    sum_array a0 a1 a2

    # 2. RELOCATION MATH TEST: Load the 2nd multiplier directly
    # The GNU Linker will calculate this offset for us!
    la t0, multipliers+ 4
    lw t1, 0(t0)

    # 3. Multiply the sum (a2) by our multiplier (t1)
    mul a0, a2, t1

    # 4. ANONYMOUS LABEL TEST: Skip the nop
    beqz a0, @f
    nop
@@:
    # Return to C! (Result is in a0)
    ret
