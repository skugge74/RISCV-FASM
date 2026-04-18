
# ==========================================
# Kdex Standard I/O (kstdio.s)
# ==========================================
.equ STDOUT, 1
.equ SYS_WRITE, 64
.equ SYS_EXIT, 93

# Usage: exit 0
macro exit %1
    li a0, %1
    li a7, SYS_EXIT
    ecall
endm

# Usage: print [buffer_address], [length]
macro print %1, %2
    mv a2, %2          # a2 = length
    mv a1, %1          # a1 = pointer to buffer
    li a0, STDOUT      # a0 = file descriptor
    li a7, SYS_WRITE   # a7 = syscall number
    ecall
endm

macro print_char %1
    addi sp, sp, -16
    sw   t0, 4(sp)       # Save t0 so we don't break previous logic
    sw   a0, 8(sp)       # Save a0
    sw   a1, 12(sp)      # Save a1
    
    li   t0, %1          # Load the char (comma, newline, etc)
    sb   t0, 0(sp)       # Buffer it on stack
    
    # Manually trigger the syscall to avoid macro-in-macro confusion
    li   a0, 1           # stdout
    mv   a1, sp          # buffer pointer
    li   a2, 1           # length
    li   a7, 64          # sys_write
    ecall
    
    lw   a1, 12(sp)      # Restore everything
    lw   a0, 8(sp)
    lw   t0, 4(sp)
    addi sp, sp, 16
endm

# Usage: print_int_reg t0
macro print_int_reg %1
    addi sp, sp, -32
    mv   a0, %1        # Input value
    mv   a1, sp        # Buffer (stack)
    call kitoa         # Returns string pointer in a0
    
    mv   s0, a0        # Save string pointer
    call strlen        # Length returned in a0
    
    print s0, a0       # Print using the saved pointer and length
    addi sp, sp, 32
endm

# Usage: print_int_imm 1337
macro print_int_imm %1
    addi sp, sp, -32
    li   a0, %1        # Input immediate
    mv   a1, sp
    call kitoa
    
    mv   s0, a0
    call strlen
    print s0, a0
    addi sp, sp, 32
endm

.global kprintf
kprintf:
    addi sp, sp, -80     # Full frame: 7 args (28) + 4 saved regs (16) + scratch (36)
    sw   ra,  76(sp)
    sw   s0,  72(sp)     # s0 = format string pointer
    sw   s1,  68(sp)     # s1 = FIXED arg bank base pointer
    sw   s2,  64(sp)     # s2 = current char
    sw   s3,  60(sp)     # s3 = arg index

    # Store arguments into a FIXED location in our stack frame
    sw   a1,  0(sp)
    sw   a2,  4(sp)
    sw   a3,  8(sp)
    sw   a4,  12(sp)
    sw   a5,  16(sp)
    sw   a6,  20(sp)
    sw   a7,  24(sp)

    mv   s0, a0          # s0 = format string
    mv   s1, sp          # s1 = fixed base of arg bank (never changes)
    li   s3, 0           # s3 = arg index

.kprintf_loop:
    lb   s2, 0(s0)
    beqz s2, .kprintf_done

    li   t0, '%'
    beq  s2, t0, .kprintf_spec

    # Print raw character via stack scratch area (use our own frame, above args)
    addi t0, sp, 28      # scratch area above arg bank
    sb   s2, 0(t0)
    li   a0, 1
    mv   a1, t0
    li   a2, 1
    li   a7, 64
    ecall
    j    .kprintf_next

.kprintf_spec:
    addi s0, s0, 1       # skip '%'
    lb   s2, 0(s0)       # get specifier

    # Load arg: s1 + (s3 * 4)
    slli t0, s3,  2
    add  t0, t0, s1
    lw   a0, 0(t0)       # a0 = current argument
    addi s3, s3, 1       # advance arg index

    li   t1, 'd'
    beq  s2, t1, .kprintf_int
    li   t1, 's'
    beq  s2, t1, .kprintf_str
    li   t1, 'c'
    beq  s2, t1, .kprintf_char
    j    .kprintf_next

.kprintf_int:
    # Use scratch space at sp+32 as itoa buffer (safe, above args+scratch)
    addi a1, sp, 32
    call kitoa           # returns string pointer in a0
    mv   t5, a0
    call strlen
    mv   a2, a0
    mv   a1, t5
    li   a0, 1
    li   a7, 64
    ecall
    j    .kprintf_next

.kprintf_str:
    mv   t5, a0          # save string pointer
    call strlen
    mv   a2, a0
    mv   a1, t5
    li   a0, 1
    li   a7, 64
    ecall
    j    .kprintf_next

.kprintf_char:
    addi t0, sp, 28      # scratch area
    sb   a0, 0(t0)
    li   a0, 1
    mv   a1, t0
    li   a2, 1
    li   a7, 64
    ecall
    j    .kprintf_next

.kprintf_next:
    addi s0, s0, 1
    j    .kprintf_loop

.kprintf_done:
    lw   s3,  60(sp)
    lw   s2,  64(sp)
    lw   s1,  68(sp)
    lw   s0,  72(sp)
    lw   ra,  76(sp)
    addi sp, sp, 80
    ret
