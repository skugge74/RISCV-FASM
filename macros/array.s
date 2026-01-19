; Usage: array buffer_name, size_in_bytes
macro array %1, %2
    %1 = STRUCT_PTR              ; Define the start of the array
    STRUCT_PTR = STRUCT_PTR + %2 ; Skip 'Size' bytes forward
endm

macro endstruct %1
    %1_SIZE = STRUCT_PTR
endm
