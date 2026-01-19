.org 0x80000000
    j _start

.include "macros/data.s"   ; Your print macros
.include "macros/struct.s"        ; Your struct/array macros
.include "macros/array.s"        ; Your struct/array macros

; ==========================================
; STRUCT DEFINITION
; ==========================================
struct Packet
    field P_ID,    4     ; Offset 0
    array P_DATA,  10    ; Offset 4  (Reserves 10 bytes: 4 to 13)
    field P_CRC,   4     ; Offset 14 (4 + 10)
endstruct Packet         ; Total Size = 18 (14 + 4)

; ==========================================
; CODE
; ==========================================
.text
.align 4
_start:
    init_uart

    ; --- 1. Test Array Offset ---
    ; P_DATA should be at offset 4
    print_str msg_data
    li t0, P_DATA
    print_int_reg t0     ; Expected: 4
    print_str ln

    ; --- 2. Test Field After Array ---
    ; P_CRC should be at offset 14 (4 + 10)
    print_str msg_crc
    li t1, P_CRC
    print_int_reg t1     ; Expected: 14
    print_str ln

    ; --- 3. Test Total Size ---
    ; Packet_SIZE should be 18 (14 + 4)
    print_str msg_size
    li t2, Packet_SIZE
    print_int_reg t2     ; Expected: 18
    print_str ln

    halt

.data
ln:       .asciz "\n"
msg_data: .asciz "Array Offset (Expect 4): "
msg_crc:  .asciz "CRC Offset   (Expect 14): "
msg_size: .asciz "Total Size   (Expect 18): "
