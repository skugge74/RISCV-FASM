# A RISC-V macro-assembly language with compiler-like capabilities and ELF toolchain integration.

*RISC-V assembler and toolchain written entirely in C. It was built to bridge the gap between raw hardware control and high-level programming logic.*

__If you dont wanna read everything:__ custom RISC-V toolchain + runtime + mini standard library + linker + allocator + ABI + macro DSL

Unlike traditional bare-bones assemblers, it features a **Stack-Based Macro Engine**, a fully compliant **ELF Relocation Engine**, and its own **Standard Library** (kinda). It allows you to write complex loops, nested conditionals, and dynamic data structures while retaining bit-perfect control over the CPU.

Whether you are writing a bare-metal bootloader (`.bin`) or compiling an object file (`.o`) to link with a modern C/C++ codebase via the GNU Linker, Kdex handles the math, the memory, and the machine code.

---

## ✨ The Kdex Philosophy & Architecture

* **Dual-Mode Output:**
* **Relocatable ELF (`-f elf`):** Generates strict, industry-standard object files. Features a smart relocation engine (`R_RISCV_HI20`, `LO12_I`, `CALL`, `JAL`) that perfectly connects your assembly to `gcc` and `ld` without OS segmentation faults. It natively handles complex relocation math (e.g., `la t0, my_buffer + 2048`).
* **Flat Binary (`-f flat`):** Generates pure, headerless machine code blobs for embedded microcontrollers, bootloaders, or raw QEMU execution based on a strict `.org` origin.
* **High-Level Macro System:** Infinite logic nesting (loops within loops), variadic arguments (`%n`, `%#`), and automatic scoped labels (`.loop_%u`) to prevent naming collisions.
* **The Kdex Standard Library (`kstdlib`):** Ships with safe, register-preserving wrappers for Linux syscalls, including `kstdio` (printing), `kfile` (file I/O), and `kstring` (memory operations).
* **Quality of Life Preprocessor:** Compile-time mutable variables (`=`), recursive file inclusion (`.include`), and a smart lexer that natively understands character literals (`'a'`, `'\n'`).

---

## 🛠️ Build & Requirements

### Requirements
* `make` & standard C compiler (`gcc`/`clang`)
* `riscv64-unknown-elf-gcc` / `riscv64-linux-gnu-objdump` (for ELF linking/debugging)
* `qemu-riscv32` (for execution)

### Commands

| Command | Description |
| --- | --- |
| `make` | Builds the `riscv-fasm` executable from source. |
| `make run FILE=test.s` | Assembles (flat) and executes a file in QEMU. |
| `make dump FILE=test.s` | Dumps the symbol table and binary layout for debugging. |

### CLI Interface

| Flag | Long Flag | Description |
| --- | --- | --- |
| `-q` | `--quiet` | Silence the UI banner/summary for CI/CD pipelines. |
| `-f` | `--format` | Set output format (`elf` or `flat`). |
| `-o` | `--output` | Specify output filename. |

**Example (C-Interop Workflow):**
```bash
./riscv-fasm -f elf math.s -o math.o
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -nostdlib main.c math.o -o program
```

---

## 📝 Syntax & Language Features

### The Control-Flow Stack
Using the internal Logic Stack, you can implement high-level loops and conditionals without dedicating hardware registers for counting.

```assembly
    li t0, 20
    li t1, 20
    
    if eq, t0, t1
        print_str match_msg
    endif 
```

### Anonymous Labels (`@@`, `@f`, `@b`)
For tiny local jumps (skip logic or short loops), use the Anonymous Label System to avoid cluttering your symbol table.

```assembly
    mv t0, a0         
@@:                   # Anchor
    lb t1, 0(t0)      
    beqz t1, @f       # Jump forward to the next @@
    addi t0, t0, 1    
    j @b              # Jump backward to the previous @@
@@:                   
    sub a0, t0, a0    
    ret
```

### Data Modeling & Structs
Kdex uses a mutable variable system to manage memory layouts automatically.

```assembly
struct Packet
    field ID, 4          ; ID = 0
    array DATA, 128      ; DATA = 4
    field CRC, 4         ; CRC = 132
endstruct Packet         ; Packet_SIZE = 136

.data
    my_packet: .space Packet_SIZE
```

### Memory Directives & Smart Lexing
* **Sections:** `.text` (R/X), `.data` (R/W), `.bss`.
* **Data Types:** `.word`, `.half`, `.byte`, `.asciz`, `.space`, `.fill`.
* **Visibility:** `.global`, `.extern` for the GNU Linker.
* **Smart Parsing:** Natively evaluates math and character literals inline.

```assembly
.equ BUFFER_SIZE, 1024 * 2
li a0, BUFFER_SIZE + 16      # Math is evaluated at compile-time
print_char '\n'              # Lexer safely handles escapes and chars
```

---

## 🚀 Development Roadmap & TODOs

### Completed Milestones
* [x] **Relocatable ELF Output:** Transitioned from a simple flat-binary assembler to a true GNU-compliant toolchain component.
* [x] **Section Separation:** Safe `.text` and `.data` isolation preventing OS segmentation faults.
* [x] **Relocation Math:** Natively handles addends in ELF mode (e.g., `la t0, my_var + 8`).
* [x] **Advanced Lexer:** Taught `parse_arg` how to safely read character literals (`'a'`, `'\n'`) and prevent "ghost memory" parsing bugs via zero-initialized buffers.
* [x] **Recursive Inclusion:** Modularized the standard library by allowing files to include files that include files.
* [x] **Pseudo-Instruction Expansion:** Added `bltz`, `bgez`, and `neg` to round out the base integer instruction set.

### Next Steps
* [ ] **Documentation**: compile-time vs runtime rules, macro expansion order, relocation model, struct memory layout rules
* [ ] **Debug mode**: --dump-macros, --dump-relocations, --trace-expansion
