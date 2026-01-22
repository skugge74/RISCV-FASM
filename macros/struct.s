; ==========================================================
; STRUCT DEFINITION MACROS
; ==========================================================

; 1. Start a new structure
; Usage: struct Monster
macro struct %1
    STRUCT_PTR = 0           ; Reset the global offset counter
endm

; 2. Define a field
; Usage: field Name, Size
macro field %1, %2
    %1 = STRUCT_PTR          ; Define the Offset Constant (e.g., M_HP = 4)
    STRUCT_PTR = STRUCT_PTR + %2 ; Increment the pointer
endm

; 3. End the structure
; Usage: endstruct Monster
macro endstruct %1
    %1_SIZE = STRUCT_PTR     ; Save the total size (e.g., Monster_SIZE = 12)
endm


; ==========================================================
; STRUCT ACCESS MACROS (READ/WRITE)
; ==========================================================
; Note: Because our custom assembler does text substitution,
; we pass the 'Type' as a suffix: w (word), h (half), b (byte).
;
; This allows 's%4' to expand directly to 'sw', 'sh', or 'sb'.
; ==========================================================

; ----------------------------------------------------------
; SET (Write to Struct)
; Usage: struct_set Instance, Field, Value, Type
; Example: struct_set player, M_ATK, 500, w  -> Generates: sw ...
;          struct_set player, M_ID,  5,   b  -> Generates: sb ...
; ----------------------------------------------------------
macro struct_set %1, %2, %3, %4
    la t0, %1           ; Load Base Address of Instance (e.g., player)
    li t1, %3           ; Load Immediate Value into Temp
    
    ; Magic Substitution: s%4 becomes sw, sh, or sb
    s%4 t1, %2(t0)      ; Store: s[Type] val, Offset(base)
endm

; ----------------------------------------------------------
; GET (Read from Struct)
; Usage: struct_get Instance, Field, DestReg, Type
; Example: struct_get player, M_ATK, a0, w   -> Generates: lw ...
;          struct_get player, M_HP,  t2, h   -> Generates: lh ...
; ----------------------------------------------------------
macro struct_get %1, %2, %3, %4
    la t0, %1           ; Load Base Address of Instance
    
    ; Magic Substitution: l%4 becomes lw, lh, or lb
    l%4 %3, %2(t0)      ; Load: l[Type] dest, Offset(base)
endm
