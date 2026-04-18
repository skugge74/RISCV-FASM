.org 0x80000000
    j _start

.include "../macros/data.s"
.include "../macros/cond_loop.s"
.include "../macros/struct.s"
.include "../macros/array.s"

struct Packet
    field P_ID,    4
    array P_DATA,  10 
    field P_CRC,   4
endstruct Packet

.text
.align 4
_start:
    init_uart

    ; ==========================================
    ; 1. WRITE LOOP (Fill Array)
    ; ==========================================
    print_str msg_write

    ; for_range i, 0, 10
    for_range i, 0, 10
        
        ; Load Index 'i' into t1
        la t0, i
        lw t1, 0(t0)
        
        ; Load Value to write (we will just use 'i' as the value)
        mv t2, t1  
        
        ; COMMAND: P_DATA[t1] = t2
        ; array_set_b Instance, Field, IndexReg, ValueReg
        array_set_b player, P_DATA, t1, t2
        
    endfor_range i
    
    print_str ln

    ; ==========================================
    ; 2. READ LOOP (Print Array)
    ; ==========================================
    print_str msg_read

    for_range i, 0, 10
        
        ; Load Index 'i' into t1
        la t0, i
        lw t1, 0(t0)
        
        ; COMMAND: t2 = P_DATA[t1]
        ; array_get_b Instance, Field, IndexReg, DestReg
        array_get_b player, P_DATA, t1, t2
        
        print_int_reg t2
        print_str space
        
    endfor_range i

    print_str ln
    qemu_off

.data
ln:        .asciz "\n"
space:     .asciz " "
msg_write: .asciz "Writing 0..9 to array...\n"
msg_read:  .asciz "Reading back: "
i:         .word 0
player:    .space Packet_SIZE
