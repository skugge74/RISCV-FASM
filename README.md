# Kdex RISC-V High-Level Assembler

A robust, custom-built Assembler for RISC-V.
This project bridges the gap between raw assembly and high-level logic,allowing you to write complex loops and conditionals using a powerful Stack-Based Macro System while retaining bit-perfect control over the hardware.

It generates raw 32-bit little-endian binary images (`.bin`) ready for bare-metal execution on RISC-V processors or emulators (QEMU).
## FEATURES

1) High-Level Macro System
- Nested Logic: Supports infinite nesting of macros (e.g., loops inside loops) using an internal stack.
- Scoped Labels: Automatically handles unique label generation (e.g., `.loop_%u`) to prevent naming collisions in recursive calls.
- Arguments: Pass registers or immediates using `%1`, `%2`, etc.

2) Recursive Math Engine
- Expression Parsing: Supports complex arithmetic natively. `li t0, 10 + 5 * 2` Evaluates to `20`.
- Location Counters: 
    - `$`: Current Address (Dynamic, moves with every instruction).
    - `$$`: Section Origin (Static).
- Relative Logic: Calculates branch offsets automatically (beq x0, x0, $+8).
- CSR Support: Full support for Control Status Registers (csrr, csrw) for OS development.
- Smart Pseudo-Ops: Automatically expands li into lui + addi for large numbers (> 12 bits).
- Multiple Inclusion: manage large projects with `.include "file.s"`.
- #TODO: Recursive Inclusion
- Scoped Symbol Table
    - Handles Global Labels (main:, _start:) and Local Labels (.loop:) separately.
    - Prevents collisions by attaching local labels to their parent context.
 
## BUILD
| Command                | Description |
| --------------         | --------------- |
| `make`                 | Builds the `riscv-assembler` executable | 
| `make run FILE=test.s` | Assembles and runs in `QUEMU` a specific assembly file. |
| `make dump FILE=test.s`| Dumps the symbol table and binary layout for debugging. |

## Macro Language
This assembler treats macros as high-level constructs. You can define custom control structures that compile down to optimized assembly.

### Defining a macro
Use `macro` and `endm`. Use `%n` for arguments and `%u` for unique IDs.

```assembly
macro print_int %1
    mv a0, %1        # Move argument 1 to a0
    li a7, 1         # Syscall ID for print
    ecall
endm

macro halt
  .halt_%u:
    j .halt_%u
endm
```

### The Control-Flow Stack 

The assembler utilizes a Logic Stack to handle nesting. Below is an implementation of a high-level repeat loop that does not require a dedicated register for counting.

Implementation:
```assembly

; Usage: repeat 10
macro repeat %1
    addi sp, sp, -4        
    li   t0, %1            
    sw   t0, 0(sp)         
    
    .rep_start_%u:         
        lw   t0, 0(sp)     
        beq  t0, zero, .rep_exit_%u
endm

; Usage: endrepeat
macro endrepeat
        lw   t0, 0(sp)    
        addi t0, t0, -1  
        sw   t0, 0(sp)  
        j .rep_start_%u    
        
    .rep_exit_%u:
        addi sp, sp, 4    
endm
```
usage example
```assembly
; Print msg 10 times
    repeat 10
        print_str msg
    endrepeat
.data 
msg: .asciz "Hi"
```
# Directives & Math

### Directives

Standard RISC-V directives are supported for organizing memory.
- `.text` / `.data`: Switch sections.
- `.org 0x80000000`: Set the origin address.
- `.asciz "String"`: Null-terminated string.
- `.word`, `.byte`, `.half`: Raw data insertion.
- `.space 1024`: Reserve zeroed memory.
- `.align 4`: Align the next data/instruction to a 4-byte boundary.

### Address arithmetic
```assembly
; Load the address of the label 'my_data' + 4 bytes
    li t0, my_data + 4
    
    ; Skip the next 2 instructions (Relative Jump)
    beq x0, x0, $ + 8
```

### System Instructions (CSRs)
Perfect for writing Kernels, Trap Handlers, and Drivers.
```assembly
; Read the Hardware Thread ID (mhartid)
    csrr t0, 0xF14
    
    ; Write to Scratch Register (mscratch)
    li t1, 0xDEADBEEF
    csrw 0x340, t1
```

