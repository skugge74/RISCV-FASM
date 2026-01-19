.org 0x80000000
j _start

.include "./macros/data.s"

# --- Add the new get_char macro here (or put it in macros/data.s) ---
macro get_char %1
    li t5, 0x10000000
    .wait_loop_%u:
        lb t6, 5(t5)
        andi t6, t6, 1
        beqz t6, .wait_loop_%u
    lb %1, 0(t5)
endm

.text
_start:
    init_uart
    print_str msg_start

shell_loop:
    # 1. Wait for input
    get_char t0
    
    # 2. Check for Quit ('q')
    li t1, 113 
    beq t0, t1, exit_shell

    # 3. Check for Enter Key (Carriage Return - 13)
    li t1, 13
    beq t0, t1, handle_newline

    # 4. Check for Line Feed (10) - just in case
    li t1, 10
    beq t0, t1, handle_newline

    # 5. Normal Echo (Print the character)
    li s1, 0x10000000
    sb t0, 0(s1)
    j shell_loop

handle_newline:
    # Print the newline string instead of echoing the raw char
    print_str ln
    j shell_loop

exit_shell:
    print_str msg_bye
    halt
.data
msg_start: .asciz "Type something (press 'q' to quit): "
msg_bye:   .asciz "\nGoodbye!\n"
