.include "../macros/stdlib/kstdio.s"
.include "../macros/stdlib/kstring.s"
.include "../macros/stdlib/kargs.s"

.data
msg_argc: .asciz "Arguments found: "
msg_argv: .asciz "First argument: "
newline:  .asciz "\n"

.text
.global _start
_start:
    # 1. Capture the stack state immediately
    call k_init_args

    # 2. Print "Arguments found: "
    print_str msg_argc
    
    # Get argc (e.g., 2)
    get_argc
    addi a0, a0, 48     # Convert 2 to '2' (ASCII 50)
    
    # --- MANUAL STACK PRINTING ---
    # We push the char to the stack so we have a pointer to it
    addi sp, sp, -16    # Allocate stack space
    sb a0, 0(sp)        # Store the ASCII char ('2') at 0(sp)
    
    li a0, 1            # stdout
    mv a1, sp           # a1 = POINTER to the char (the stack address)
    li a2, 1            # length = 1
    li a7, 64           # sys_write
    ecall
    
    addi sp, sp, 16     # Restore stack
    # ----------------------------

    print_str newline

    # 3. Check if we have user arguments (argc > 1)
    get_argc
    li t0, 1
    ble a0, t0, .no_args

    # 4. Get and print argv[1]
    print_str msg_argv
    li t0, 1
    get_argv t0         # a0 = pointer to "hello"
    
    mv s0, a0
    call strlen
    mv a1, a0           # length
    mv a0, s0           # pointer
    print a0, a1        # syscall write
    print_str newline

    exit 0

.no_args:
    exit 0
