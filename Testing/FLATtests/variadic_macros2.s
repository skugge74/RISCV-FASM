.org 0x80000000
j _start

.include "../macros/data.s"

# ==========================================
# THE ADVANCED VARIADIC MACRO
# ==========================================
macro define_packet %1, %*
    .word %#          # 1. Store total argument count
    .word %1          # 2. Store the "Packet ID" (First argument)
    .byte %* # 3. Dump ALL arguments as raw bytes 
    .align 4          # 4. Re-align for the next struct
endm

.text
_start:
    li sp, 0x80100000
    li s1, 0x10000000 
    # ------------------------------------------------
    # 1. Test Argument Counting (%#)
    # ------------------------------------------------
    print_str msg_count
    la s2, my_packet
    lw t0, 0(s2)      # Load arg count (Should be 6)
    print_int_reg t0
    print_str nl
    
    # ------------------------------------------------
    # 2. Test Positional Extraction (%1)
    # ------------------------------------------------
    print_str msg_magic
    lw t1, 4(s2)      # Load Packet ID (Should be 0x42)
    print_hex_reg t1
    print_str nl
    
    # ------------------------------------------------
    # 3. Test Variadic Dump (%*) & Multi-byte parsing
    # ------------------------------------------------
    print_str msg_bytes
    # The bytes start at offset 8.
    # The arguments are: 0x42, 0x10, 0x20, 0x30, 0x40, 0x99
    # We want to read the LAST byte (0x99).
    # Offset = 8 (header size) + 5 (index of 6th element) = 13
    lbu t2, 13(s2)
    print_hex_reg t2
    print_str nl

    qemu_off

.data
.align 4
my_packet:
    # We pass 6 total arguments. 
    # Packet ID is 0x42.
    define_packet 0x42, 0x10, 0x20, 0x30, 0x40, 0x99

msg_count: .asciz "Arg Count: "
msg_magic: .asciz "Packet ID: "
msg_bytes: .asciz "Last Byte: "
nl: .asciz "\n"
