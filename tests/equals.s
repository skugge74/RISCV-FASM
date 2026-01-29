.org 0x80000000
    j _start

.include "./macros/data.s"

; ==========================================
; SIMULATING A C STRUCT
; struct Player {
;    int hp;  // 0
;    int mp;  // 4
;    int x;   // 8
;    int y;   // 12
; }
; ==========================================

CURRENT_OFFSET = 0          ; Start at 0

PLAYER_HP = CURRENT_OFFSET  ; HP is at 0
CURRENT_OFFSET = CURRENT_OFFSET + 4   ; Bump offset

PLAYER_MP = CURRENT_OFFSET  ; MP is at 4
CURRENT_OFFSET = CURRENT_OFFSET + 4   ; Bump offset

; --- TEST: Messy Syntax (Spaces & Comments) ---
PLAYER_X = CURRENT_OFFSET             ; X is at 8
CURRENT_OFFSET = CURRENT_OFFSET + 4   ; Bump offset (Messy comment check)

PLAYER_Y = CURRENT_OFFSET             ; Y is at 12
CURRENT_OFFSET = CURRENT_OFFSET + 4   ; Final size is 16

PLAYER_SIZE = CURRENT_OFFSET          ; Save total size

; ==========================================
; CODE
; ==========================================
.text
_start:
    init_uart
    
    ; Should load 16 (0x10)
    li t0, PLAYER_SIZE
    print_int_reg t0
    print_str ln
    ; 2. Check individual offsets
    ; Should load 0
    li t1, PLAYER_HP
    print_int_reg t1
    print_str ln
 
    ; Should load 4
    li t2, PLAYER_MP
    print_int_reg t2
    print_str ln
 
    ; Should load 8
    li t3, PLAYER_X
    print_int_reg t3
    print_str ln
 
    ; Should load 12
    li t4, PLAYER_Y
    print_int_reg t4
    print_str ln
 
    qemu_off
