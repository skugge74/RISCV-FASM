.org 0x80000000
j _start

.include "./macros/data.s"
.include "./macros/cond_loop.s"


;  --- 2. CODE SECTION ---
.text
_start:
    li sp, 0x80100000   # Initialize Stack
    li s1, 0x10000000   # UART Base
repeat 10
  print_str msg    
endrepeat
qemu_off
# --- 3. DATA SECTION ---
.data
.align 4
msg:
    .asciz "Hello, RISC-V FASM!\n"
