;Note: array is the same macro as field
; Usage: array buffer_name, size_in_bytes
macro array %1, %2
    %1 = STRUCT_PTR              ; Define the start of the array
    STRUCT_PTR = STRUCT_PTR + %2 ; Skip 'Size' bytes forward
endm

macro endarray %1
    %1_SIZE = STRUCT_PTR
endm

; ==========================================================
; BYTE ARRAY ACCESS (For char/byte buffers)
; ==========================================================

; ----------------------------------------------------------
; ARRAY GET BYTE
; Usage: array_get_b Instance, Field, IndexReg, DestReg
; Example: array_get_b player, P_DATA, t1, t2
; ----------------------------------------------------------
macro array_get_b %1, %2, %3, %4
    la t6, %1           ; 1. Load Base Address into t6 (Safe Temp)
    add t6, t6, %3      ; 2. Add the Index (Base + Index)
    lb %4, %2(t6)       ; 3. Load Byte from (Base+Index) + Offset
endm

; ----------------------------------------------------------
; ARRAY SET BYTE
; Usage: array_set_b Instance, Field, IndexReg, ValueReg
; Example: array_set_b player, P_DATA, t1, t2
; ----------------------------------------------------------
macro array_set_b %1, %2, %3, %4
    la t6, %1           ; 1. Load Base Address into t6
    add t6, t6, %3      ; 2. Add the Index
    sb %4, %2(t6)       ; 3. Store Byte at (Base+Index) + Offset
endm
