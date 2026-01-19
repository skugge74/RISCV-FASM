; ==========================================================
; STRUCT MACROS
; ==========================================================

; Initialize a new structure
macro struct %1
    STRUCT_PTR = 0           ; Reset the global counter
endm

; Define a field
; Usage: field Name, Size
macro field %1, %2
    %1 = STRUCT_PTR          ; 1. Define the Constant (e.g. M_HP = 4)
    STRUCT_PTR = STRUCT_PTR + %2 ; 2. Update the Variable (4 + 4 = 8)
endm

; Finalize the structure
macro endstruct %1
    %1_SIZE = STRUCT_PTR     ; 3. Save the final total size
endm
