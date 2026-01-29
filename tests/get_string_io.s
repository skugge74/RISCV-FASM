.org 0x80000000
    j _start

.include "macros/data.s"
.include "./macros/cond_loop.s"

.text
.align 4
_start:
    init_uart         
    print_str banner   

    ; === SHELL MAIN LOOP ===
    li s10, 1          ; Keep looping forever
    
    while ne, s10, zero
    
        print_str prompt
        
        ; Wait for user to type a command
        get_string input_buffer, 64
        
        ; Check "help"
        strcmp input_buffer, cmd_help, t0
        if eq, t0, zero
            print_str msg_help
        endif

        ; Check "status"
        strcmp input_buffer, cmd_status, t0
        if eq, t0, zero
            print_str msg_status
        endif

        ; Check "reboot"
        strcmp input_buffer, cmd_reboot, t0
        if eq, t0, zero
            print_str msg_reboot
            j _start    ; Jump to start to "reboot"
        endif

        ; Check "exit"
        strcmp input_buffer, cmd_exit, t0
        if eq, t0, zero
            print_str msg_bye
            qemu_off
        endif

    endwhile
    qemu_off

.data
.align 4
; Buffers
input_buffer: .space 64

; Strings
banner:     .asciz "\n=== Kdex RISC-V Shell v1.0 ===\n"
prompt:     .asciz "kdex> "
msg_help:   .asciz "Commands: help, status, reboot, exit\n"
msg_status: .asciz "System OK. Memory at 0x80000000.\n"
msg_reboot: .asciz "Rebooting...\n"
msg_bye:    .asciz "Shutting down.\n"

; Commands
cmd_help:   .asciz "help"
cmd_status: .asciz "status"
cmd_reboot: .asciz "reboot"
cmd_exit:   .asciz "exit"
