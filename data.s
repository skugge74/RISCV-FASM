.org 0x80000000

.text
_start:
    la s0, msg          # Pointer to string
    li s1, 0x10000000   # UART Base Address

print_loop:
    lb s2, 0(s0)        # Load next character
    beqz s2, halt       # If character is 0 (null terminator), we are done
    sb s2, 0(s1)        # Write character to UART (this prints it!)
    addi s0, s0, 1      # Move to next character
    j print_loop

halt:
    j halt              # Infinite loop to keep the CPU from crashing

.align 4
msg:
    .asciz "Hello, RISC-V FASM!\n"
