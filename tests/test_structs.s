.org 0x80000000
    j _start

.include "./macros/data.s"   
.include "./macros/struct.s" 

; --- Define a "Monster" Struct ---
struct Monster
    field M_ID,   1    ; Offset 0
    field M_HP,   2    ; Offset 4
    field M_ATK,  4    ; Offset 8
    field M_POS,  10    ; Offset 12
endstruct Monster      ; Size 17

.text
.align 4
_start:
    init_uart

    ; 1. Print Total Size
    li t0, Monster_SIZE
    print_int_reg t0    ; Should print 17
    print_str ln

    ; 2. Print Offsets
    li t1, M_ATK        ; Should be 8
    print_int_reg t1
    print_str ln
    
    ; 3. Print Offsets
    li t1, M_POS        ; Should be 12
    print_int_reg t1
    print_str ln
    halt

.data
ln: .asciz "\n"
