# Kdex RISC-V High-Level Assembler

A robust, custom-built assembler that bridges the gap between raw assembly and high-level logic. **Kdex** allows you to write complex loops and data structures using a powerful **Stack-Based Macro System** while retaining bit-perfect control over hardware.

It generates raw 32-bit little-endian binary images (`.bin`) optimized for bare-metal execution on RISC-V processors or QEMU.

---

## ✨ Key Features

* **High-Level Macro System:** * **Nested Logic:** Supports infinite nesting (loops within loops) via an internal logic stack.
* **Scoped Labels:** Automatic unique label generation (`.loop_%u`) prevents naming collisions.


* **Stateful Preprocessor:** * **Mutable Variables:** Compile-time variables (`=`) allow dynamic constant calculation.
* **Zero-Overhead Abstraction:** All logic resolves at compile-time to hardcoded constants.


* **Data Modeling:** C-style `struct` and `array` support with automatic offset management.
* **Recursive Math Engine:** Supports complex arithmetic natively (e.g., `li t0, 10 + 5 * 2 << 3`).
* **OS Development Ready:** Full support for Control Status Registers (CSRs) and multiple file inclusion.

---

## 🛠️ Build & Requirements

### Requirements

* `riscv64-linux-gnu-objdump`
* `qemu-system-riscv32`
* `make`

### Commands

| Command | Description |
| --- | --- |
| `make` | Builds the `riscv-fasm` executable. |
| `make run FILE=test.s` | Assembles and executes a file in QEMU. |
| `make dump FILE=test.s` | Dumps the symbol table and binary layout for debugging. |

---

## 📝 Macro Language & Logic

Macros in Kdex are treated as high-level constructs. Use `%n` for arguments and `%u` for unique IDs.

### Basic Definition

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

Using the internal Logic Stack, you can implement high-level loops without dedicating a specific register for counting.

```assembly
; Usage: repeat 10 ... endrepeat
macro repeat %1
    addi sp, sp, -4        
    li   t0, %1            
    sw   t0, 0(sp)         
    
    .rep_start_%u:         
        lw   t0, 0(sp)     
        beq  t0, zero, .rep_exit_%u
endm

macro endrepeat
        lw   t0, 0(sp)    
        addi t0, t0, -1  
        sw   t0, 0(sp)  
        j .rep_start_%u    
        
    .rep_exit_%u:
        addi sp, sp, 4    
endm

```

---

## 🏗️ Data Modeling (Structs & Arrays)

Kdex uses a mutable variable system to manage memory layouts automatically.

### Definitions

```assembly
struct Packet
    field ID, 4          ; ID = 0
    array DATA, 128      ; DATA = 4
    field CRC, 4         ; CRC = 132
endstruct Packet         ; Packet_SIZE = 136

```

### Usage Example

```assembly
_start:
    struct_set my_packet, ID, 1, b   
    struct_set my_packet, CRC, 999, w

    for_range i, 0, 128
        la t0, i
        lw t1, 0(t0)      ; Load Index 'i'
        mv t2, t1         ; Value to write
        array_set_b player, P_DATA, t1, t2
    endfor_range i

.data:
    my_packet: .space Packet_SIZE  ; Allocates 136 bytes

```

---

## ⚙️ Directives & System Instructions

### Memory Management

* `.text` / `.data`: Switch between sections.
* `.org 0x80000000`: Define origin address.
* `.align 4`: Align to 4-byte boundary.
* `.asciz`, `.word`, `.byte`, `.half`, `.space`.

### Address Arithmetic

```assembly
li t0, my_data + 4   ; Address of label + offset
beq x0, x0, $ + 8    ; Jump 2 instructions forward ($ = current address)

```

### CSR Support (Kernel/Driver Level)

```assembly
csrr t0, 0xF14       ; Read Hardware Thread ID (mhartid)
li t1, 0xDEADBEEF
csrw 0x340, t1       ; Write to mscratch

```

---

## 📅 TODO

* [ ] Recursive Inclusion
* [ ] Variadic macro arguments
* [ ] Anonymous label handling (`@@`, `@f`, `@b`)

