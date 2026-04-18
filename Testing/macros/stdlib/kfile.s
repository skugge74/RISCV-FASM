# ==========================================
# Kdex File I/O (kfile.s)
# ==========================================
.equ SYS_OPENAT, 56
.equ SYS_CLOSE,  57
.equ AT_FDCWD,  -100

.equ O_RDONLY, 0
.equ O_WRONLY, 1
.equ O_CREAT,  64

# Open a file relative to current directory
# Usage: open_file [filename_address], [flags]
macro open_file %1 %2
    mv a1, %1         # MOVE USER ARG FIRST
    li a2, %2         # MOVE USER ARG FIRST
    li a0, AT_FDCWD   # THEN safely overwrite a0
    li a3, 0666       # Default permissions
    li a7, SYS_OPENAT
    ecall
endm

# Read from an open file into a memory buffer
# Usage: read_file [file_descriptor], [buffer_address], [bytes_to_read]
macro read_file %1 %2 %3
    mv a2, %3         # Move length
    mv a1, %2         # Move buffer
    mv a0, %1         # Move FD last (in case %2 or %3 relied on a0)
    li a7, SYS_READ   # (Requires kstdio.s to be included for SYS_READ)
    ecall
endm

# Close an open file
# Usage: close_file [file_descriptor]
macro close_file %1
    mv a0, %1
    li a7, SYS_CLOSE
    ecall
endm
