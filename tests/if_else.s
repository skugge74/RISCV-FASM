.org 0x80000000
j _start

.include "./macros/data.s"

; Usage: if condition, reg1, reg2, label_name
macro if %1 %2 %3 %4
    ; 1. Check condition (e.g., beq t0, t1, .body)
    b%1 %2, %3, .if_body_%4
    
    ; 2. If false, jump over the body to the end
    j .if_end_%4
    
    ; 3. Start of the body
    .if_body_%4:
endm

; Usage: endif label_name
macro endif %1
    ; 4. Define the target for the jump in step 2
    .if_end_%1:
endm


.include "./macros/data.s"  ; Assuming your print macros are here

.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
 
    li t0, 20
    li t1, 10  ; They are equal
    
    max t2, t0, t1
    print_int_reg t2

    ; --- Test 1: Should Print ---
    if eq, t0, t1, check_1
        print_str match_msg
    endif check_1

    ; --- Test 2: Should NOT Print ---
    if ne, t0, t1, check_2
        print_str no_match_msg
    endif check_2
print_str success_msg
    halt

.data
match_msg: .asciz "Matched!\n"
no_match_msg: .asciz "Not matched\n"
success_msg: "succes\n"
