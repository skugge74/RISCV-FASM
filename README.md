# Kdex RISC-V High-Level Assembler

A robust, custom-built Assembler for RISC-V.
This project bridges the gap between raw assembly and high-level logic,allowing the writing of complex loops and use complex data structures using a powerful Stack-Based Macro System while retaining bit-perfect control over the hardware.

It generates raw 32-bit little-endian binary images (`.bin`) ready for bare-metal execution on RISC-V processors or emulators (QEMU).
## FEATURES

1) **High-Level Macro System**
    - Nested Logic: Supports infinite nesting of macros (e.g., loops inside loops) using an internal stack.
    - Scoped Labels: Automatically handles unique label generation (e.g., `.loop_%u`) to prevent naming collisions in recursive calls.
    - Arguments: paramater handling using `%1`, `%2`, etc.

2) **Recursive Math Engine**
    - Expression Parsing: Supports complex arithmetic natively. `li t0, 10 + 5 * 2 << 3` Evaluates to ` t0 <- 160`.
    - Location Counters: 
        - `$`: Current Address (Dynamic, moves with every instruction).
        - `$$`: Section Origin (Static).
3) **Stateful Preprocessor & Data Modeling**
    - Mutable Variables: Introduces compile-time variables (`=`) that can be reassigned (e.g., `OFFSET = OFFSET + 4`), enabling dynamic calculation of constants during assembly.

    - C-Style Structs: Native support for defining complex data structures (`struct`, `field`, `endstruct`) with automatic offset management.

    - Memory Layout Engine: Automatically calculates total object sizes (`Struct_SIZE`) and handles array reservations (`array buffer, 128`), eliminating manual offset math.

    - Zero-Overhead Abstraction: All calculations happen at compile time, resolving to hardcoded constants in the binary with no runtime performance penalty. 

4) **Other things**
    - CSR Support: Full support for Control Status Registers (csrr, csrw) for OS development.
    - Multiple Inclusion: manage large projects with `.include "file.s"`.
5) TODO
    - Recursive Inclusion
    - dynamic number of args for macros
    - add @@, @f, @d for anonymous labels handling
    
## BUILD
**REQUIREMENTS:**
- riscv64-linux-gnu-objdump
- qemu-system-riscv32
- make 

| Command                | Description |
| --------------         | --------------- |
| `make`                 | Builds the `riscv-fasm` executable | 
| `make run FILE=test.s` | Assembles and runs in `QUEMU` a specific assembly file. |
| `make dump FILE=test.s`| Dumps the symbol table and binary layout for debugging using `riscv64-linux-gnu-objdump`. |

## Macro Language
This assembler treats macros as high-level constructs. You can define custom control structures that compile down to optimized assembly.

### Defining a macro
Use `macro` and `endm`. Use `%n` (where `n` is a number) for arguments and `%u` for unique IDs.

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
## Compile-Time Variables & Mutable State
The Variable System (`=`) operates entirely at compile-time. Unlike labels, these variables are mutable.

### Syntax and behaviour

Use the `=` operator to create or update a variable. The value is calculated immediately and stored in the symbol table.

```assembly
OFFSET = 1                ; Define variable 'OFFSET' with value 1
OFFSET = OFFSET + 4       ; Update 'OFFSET' (1 + 4 = 5)
li t0, OFFSET*2           ; t0 <- 10 (5*2)
MAX_VAL = 10 * 2          ; Math is evaluated immediately (20)
OFFSET = OFFSET + MAX_VAL ; OFFSET <- 25 (5+20)
li t1, OFFSET             ; t1 <- 25
```

### struct & array macro definition 
```assembly
; Usage: struct Packet
macro struct %1
    STRUCT_PTR = 0           ; Reset the global offset counter
endm

; Usage: field Name, Size
macro field %1, %2
    %1 = STRUCT_PTR          ; Define the Offset Constant (e.g., M_CRC = 4)
    STRUCT_PTR = STRUCT_PTR + %2 ; Increment the pointer
endm

; Usage: endstruct Packet
macro endstruct %1
    %1_SIZE = STRUCT_PTR     ; Save the total size (e.g., Packet_SIZE = 12)
endm


macro struct_set %1, %2, %3, %4
    la t0, %1           ; Load Base Address of Instance 
    li t1, %3           ; Load Immediate Value into Temp
    s%4 t1, %2(t0)      ; Store: s[Type] val, Offset(base)
endm

macro struct_get %1, %2, %3, %4
    la t0, %1           ; Load Base Address of Instance
    l%4 %3, %2(t0)      ; Load: l[Type] dest, Offset(base)

macro array %1, %2
    %1 = STRUCT_PTR              ; Define the start of the array
    STRUCT_PTR = STRUCT_PTR + %2 ; Skip 'Size' bytes forward
endm

macro array_get_b %1, %2, %3, %4
    la t6, %1           ;  Load Base Address into t6 (Safe Temp)
    add t6, t6, %3      ;  Add the Index (Base + Index)
    lb %4, %2(t6)       ;  Load Byte from (Base+Index) + Offset
endm

 macro array_set_b %1, %2, %3, %4
    la t6, %1           ;  Load Base Address into t6
    add t6, t6, %3      ;  Add the Index
    sb %4, %2(t6)       ;  Store Byte at (Base+Index) + Offset
endm

```

### struct & array Usage example
```assembly
struct Packet
    field ID, 4         ; ID = 0
    array DATA, 128     ; DATA = 4
    field CRC, 4        ; CRC = 132 (4 + 128)
endstruct Packet        ; Packet_SIZE = 136


_start:
    struct_set my_packet, ID,  1,   b   
    struct_set my_packet, CRC, 999, w
    for_range i, 0, 128
        
        ; Load Index 'i' into t1
        la t0, i
        lw t1, 0(t0)
        
        ; Load Value to write (i) 
        mv t2, t1  
        
        ; COMMAND: P_DATA[t1] = t2
        ; array_set_b Instance, Field, IndexReg, ValueReg
        array_set_b player, P_DATA, t1, t2
        
    endfor_range i
    
    struct_get my_packet, CRC, t0, w    
    
    li t1, 10
    ; COMMAND: t2 = P_DATA[10]
    array_get_b my_packet, DATA, t1, t2
    
    print_int_reg t0    ; Prints 999
    print_str ln
.data:
my_packet: .space Packet_SIZE  ; Allocate 136 bytes 
```

## Directives & Math

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

