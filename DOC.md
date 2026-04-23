# Kdex RISC-V FASM Documentation

## Overview

Kdex (RISC-V FASM) is a macro-enhanced RISC-V assembler and toolchain designed to bridge low-level hardware control with high-level programming abstractions. It supports both flat binary output for bare-metal environments and relocatable ELF object generation for integration with the GNU toolchain.

---

## Design Philosophy

Kdex is built around three core principles:

### 1. Control without abstraction loss

Kdex exposes raw RISC-V instructions while allowing structured programming constructs (loops, conditionals, structs) without sacrificing predictable machine-level behavior.

### 2. Dual execution targets

* **Flat binary mode**: deterministic output for embedded systems and bootloaders.
* **ELF mode**: GNU-compatible object files with full relocation support.

### 3. Compile-time intelligence

A macro engine and preprocessor enable compile-time evaluation, scoped labels, and structured memory modeling.

---

## Build System

### Requirements

* GCC or Clang
* RISC-V toolchain (`riscv64-unknown-elf-gcc`)
* QEMU (for execution testing)

### Build Commands

```bash
make
```

Builds the assembler executable.

```bash
make run FILE=example.s
```

Assembles and runs a program in QEMU.

```bash
make dump FILE=example.s
```

Outputs symbol table and binary layout.

````

---

## CLI Usage

```bash
./riscv-fasm [options] input.s
````

### Options

| Flag    | Description            |
| ------- | ---------------------- |
| -f elf  | Output ELF object file |
| -f flat | Output raw binary      |
| -o file | Output filename        |
| -q      | Quiet mode             |

---

## Compilation Modes

### Flat Mode

Produces raw machine code starting at `.org` address.

Use cases:

* Bootloaders
* Embedded firmware
* QEMU bare execution

### ELF Mode

Produces relocatable `.o` files compatible with GNU linker.

Features:

* Symbol table generation
* RISC-V relocation types (HI20, LO12, CALL, JAL)
* External symbol linking

---

## Language Overview

## Instructions

Kdex supports standard RISC-V RV32I instructions.

Example:

```asm
li t0, 10
li t1, 20
add t2, t0, t1
```

---

## Control Flow

### Conditionals

```asm
if eq, t0, t1
    print_str msg
endif
```

Supports:

* eq
* ne
* lt
* ge

---

## Labels

### Anonymous Labels

```asm
@@:
    addi t0, t0, 1
    bne t0, t1, @b
@@:
```

* `@b` → previous label
* `@f` → next label

---

## Macros

Kdex supports variadic macro expansion and scoped labels.

Example:

```asm
macro INC reg
    addi reg, reg, 1
endmacro
```

---

## Struct System

Kdex provides compile-time memory layout generation.

```asm
struct Packet
    field ID, 4
    array DATA, 128
    field CRC, 4
endstruct
```

### Usage

```asm
.data
my_packet: .space Packet_SIZE
```

---

## Memory Directives

| Directive | Description            |
| --------- | ---------------------- |
| .text     | Code section           |
| .data     | Initialized data       |
| .bss      | Zero-initialized data  |
| .word     | 32-bit value           |
| .byte     | 8-bit value            |
| .asciz    | Null-terminated string |
| .space    | Reserve memory         |

---

## Expressions

Kdex supports compile-time evaluation:

```asm
.equ SIZE, 1024 * 2
li a0, SIZE + 16
```

Supports:

* arithmetic
* constants
* character literals

---

## Standard Library (kstdlib)

Kdex includes a minimal syscall wrapper layer:

### Modules

* kstdio → output functions
* kstring → memory utilities
* kfile → file operations

---

## Relocation System (ELF mode)

Kdex handles:

* R_RISCV_HI20
* R_RISCV_LO12_I
* R_RISCV_CALL
* R_RISCV_JAL

Supports expressions like:

```asm
la t0, label + 8
```

---

## Development Roadmap

### Completed

* ELF relocation engine
* Flat binary output
* Macro system
* Struct system
* Preprocessor improvements

### In Progress

* Heap memory (kmalloc/kfree)
* CLI argument parsing

### Future Goals

* Self-hosting compiler
* Debug tracing system
* Advanced optimizer pass

---

## Architecture Notes

Kdex is structured into:

1. Lexer
2. Parser
3. Macro expander
4. Instruction encoder
5. ELF relocation engine
6. Output backend

---

## License

(Add your license here)

---

## Notes

Kdex is designed as both an educational tool and a practical assembler for RISC-V development workflows.
