# Kdex RISC-V High-Level Assembler

A robust, custom-built assembler that bridges the gap between raw assembly and high-level logic. **Kdex** allows you to write complex loops and data structures using a powerful **Stack-Based Macro System** while retaining bit-perfect control over hardware.

It generates raw 32-bit little-endian binary images (`.bin`) optimized for bare-metal execution on RISC-V processors or QEMU, as well as strictly compliant ELF object files (`.o`) for seamless C/C++ integration.

---

## ✨ Key Features

* **High-Level Macro System:** * **Nested Logic:** Supports infinite nesting (loops within loops) via an internal logic stack.
  * **Scoped Labels:** Automatic unique label generation (`.loop_%u`) prevents naming collisions.
  * **Variadic Arguments:** Macros can accept flexible argument counts using `%n` and `%#`.

* **Stateful Preprocessor:** * **Mutable Variables:** Compile-time variables (`=`) allow dynamic constant calculation.
  * **Zero-Overhead Abstraction:** All logic resolves at compile-time to hardcoded constants.

* **Section-Aware Memory Model:** * True structural separation of `.text` (Read/Execute), `.data` (Read/Write), and `.bss` sections. Ensures strict, OS-compliant memory permissions and prevents segmentation faults when linking with high-level languages.

* **Data Modeling:** C-style `struct` and `array` support with automatic offset management.
* **Recursive Math Engine:** Supports complex arithmetic natively (e.g., `li t0, 10 + 5 * 2 << 3`) with section-relative Program Counter (`$`) evaluation.
* **OS Development Ready:** Full support for Control Status Registers (CSRs) and multiple file inclusion.

---

## 🛠️ Build & Requirements

### Requirements

* `riscv64-linux-gnu-objdump` (or `riscv64-unknown-elf-gcc` for cross-compilation)
* `qemu-system-riscv32` / `qemu-riscv32`
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

* `.text` / `.data` / `.bss`: Safely switch between hardware memory sections.
* `.org 0x80000000`: Define hardware origin address for flat binaries.
* `.align 4`: Align to N-byte boundaries.
* `.asciz`, `.word`, `.byte`, `.half`, `.space`, `.fill`.
* `.global`, `.extern`: Define symbol visibility for the GNU Linker.

### Address Arithmetic

```assembly
li t0, my_data + 4   ; Address of label + offset
beq x0, x0, $ + 8    ; Jump 2 instructions forward ($ = section-relative current address)
```

### CSR Support (Kernel/Driver Level)

```assembly
csrr t0, 0xF14       ; Read Hardware Thread ID (mhartid)
li t1, 0xDEADBEEF
csrw 0x340, t1       ; Write to mscratch
```

---

## 🏗️ Binary & ELF Output Formats

Kdex supports two distinct output modes depending on your stage in the development lifecycle.

### 1. Relocatable ELF (`-f elf`)
**Purpose:** Industry-standard object files (`.o`) for linking with C/C++ or other assembly modules.
* **True Memory Separation:** Safely isolates `.text` (Read/Execute) and `.data` (Read/Write) to prevent OS Segmentation Faults.
* **Smart Relocations:** Automatically generates `R_RISCV_HI20`, `LO12_I`, and `CALL` relocations for cross-section and external global symbols.
* **Toolchain Friendly:** 100% compatible with `riscv64-unknown-elf-gcc` and standard GNU `ld`.
* **Standard Workflow:**
  ```bash
  ./riscv-fasm -f elf math.s -o math.o
  riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib main.c math.o -o program
  ```

### 2. Flat Binary (`-f flat`)
**Purpose:** Pure machine code blobs (`.bin`) for direct hardware execution.
* **Minimalist:** No headers or metadata; the file contains only instructions and data appended sequentially.
* **Origin-based:** Requires `.org` to calculate absolute jumps correctly.

---

## 🏷️ Advanced Labeling & Macros

### Anonymous Labels (`@@`, `@f`, `@b`)
For tiny local jumps where naming a label is a waste of time (e.g., skip logic or short loops), use the **Anonymous Label System**.

* `@@`: Defines an anonymous anchor.
* `@f`: Jumps **forward** to the next `@@`.
* `@b`: Jumps **backward** to the previous `@@`.

```assembly
    li t0, 5
@@:                   ; Anchor
    addi t0, t0, -1
    bnez t0, @b       ; Jump back to the @@ above
    
    beq a0, a1, @f    ; Jump forward to the next @@
    nop
@@:                   ; Anchor
```

### Variadic Macro Arguments (`...`)
Kdex supports macros that accept a flexible number of arguments. This is particularly useful for building data tables or multi-register save/restore logic.

* `%n`: Refers to a specific argument (e.g., `%1`, `%2`).
* `%#`: Returns the total number of arguments passed.

```assembly
; Example: Push multiple registers to the stack
macro push_all ...
    addi sp, sp, -(%# * 4)   ; Allocate space based on argument count
    for_range i, 1, %# + 1
        sw %{i}, ((i-1)*4)(sp)
    endfor_range
endm

; Usage:
push_all ra, s0, s1, t0      ; Automatically handles 4 registers
```

---

## 🛠️ CLI Interface (Quiet Mode)

For automation, CI/CD pipelines, or custom Python test suites, use **Quiet Mode** to suppress the UI banner and build summary.

| Flag | Long Flag | Description |
| --- | --- | --- |
| `-q` | `--quiet` | Silence all non-error output. |
| `-f` | `--format` | Set output format (`elf` or `flat`). |
| `-o` | `--output` | Specify output filename (defaults to input with new ext). |

```bash
# Silence the UI for scripted builds
./riscv-fasm -q -f elf tests/shell.s -o build/shell.o
```

---

### 🚀 Updated TODO
* [x] **Separate Sections:** Separate sections `.text` and `.data` to ensure correct R/W/X OS permissions and avoid segfaults during C-interop.
* [x] **Recursive Inclusion:** Allow files to include files that include files.
* [ ] **Self-Hosting:** Rewrite the assembler in Kdex Assembly.
* [x] **Relocation Math Problem** : Currently, the ELF writer handles simple relocations (like HI20 and LO12_I). But what if an instruction is something like la t0, my_var + 8?
