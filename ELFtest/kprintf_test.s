.include "../macros/stdlib/kstring.s"
.include "../macros/stdlib/kstdio.s"

.data
fmt: .asciz "User %d: %s has %d coins.\n"
name: .asciz "Kdex"

.text
.global _start
_start:
    la a0, fmt        # Argument 0: Format
    li a1, 1          # Argument 1: %d
    la a2, name       # Argument 2: %s
    li a3, 500        # Argument 3: %d
    
    call kprintf

    exit 0
