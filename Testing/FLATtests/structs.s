.org 0x80000000
    j _start

.include "../macros/data.s"   
.include "../macros/struct.s" 

; --- Define a "Monster" Struct ---
; Note: Order matters for alignment 
struct Monster
    field M_ATK,  4    ; Offset 0
    field M_HP,   2    ; Offset 4
    field M_ID,   1    ; Offset 6
    field M_POS,  10   ; Offset 7
endstruct Monster      ; Size 17

.text
.align 4
_start:
    init_uart

    ; --- WRITE (Set) ---
    ; Usage: struct_set Instance, Field, Value, Type(b/h/w)
    
    struct_set player, M_ID,  1,   b   ; b -> sb
    struct_set player, M_HP,  100, h   ; h -> sh
    struct_set player, M_ATK, 999, w   ; w -> sw

    ; --- READ (Get) ---
    ; Usage: struct_get Instance, Field, DestReg, Type(b/h/w)
    
    struct_get player, M_ATK, t0, w    
    
    print_int_reg t0    ; Prints 999
    print_str ln
    qemu_off

.data
ln: .asciz "\n"
player: .space Monster_SIZE
