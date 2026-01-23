.org 0x80000000
    j _start

.include "macros/data.s"
.include "macros/cond_loop.s"

.text
.align 4
_start:
    init_uart

    li t0, 5        ; Counter = 5
    li t1, 0        ; Limit = 0
    li t2, 3        ; Trigger Value = 3

    print_str msg_start

    ; while (t0 > t1)  -> while gt, t0, t1
    while gt, t0, t1
        
        print_int_reg t0
        print_str space

        ; if (t0 == 3) -> if eq, t0, t2
        if eq, t0, t2
            print_str msg_trigger
        endif

        addi t0, t0, -1   ; Decrement
    endwhile

    print_str ln
    print_str msg_done
    halt

.data
ln:          .asciz "\n"
space:       .asciz " "
msg_start:   .asciz "Countdown: "
msg_trigger: .asciz " (Three!) "
msg_done:    .asciz "\nDone."
