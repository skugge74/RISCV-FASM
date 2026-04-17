.text
.align 4

_start:
    # 1. Load an external global variable (requires 2 relocations: HI20, LO12_I)
    la a0, external_variable

    # 2. Call an external C function (requires 2 relocations: HI20, LO12_I)
    call printf

    # 3. Direct jump to an external label (requires 1 relocation: JAL)
    j exit_program

    # 4. Local label (Should NOT generate relocations, should be encoded instantly)
.local_loop:
    addi a0, a0, 1
    j .local_loop
