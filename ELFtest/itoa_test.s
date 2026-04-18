.include "../macros/stdlib/kstring.s"
.include "../macros/stdlib/kstdio.s"

.text
.global _start
_start:
    # 1. Print the '1'
    print_int_imm 1
    
    # 2. Print the colon
    # Using a literal ASCII colon (58)
    print_char ','  

    # 3. Print the '123'
    print_int_imm 123
    
    # 4. Print the '456'
    li s1, 456
    print_int_reg s1

    # 5. Final Newline
    print_char '\n'

    exit 0
