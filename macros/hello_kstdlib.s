.include "stdlib/kstdio.s"
.include "stdlib/kstring.s"
.include "stdlib/kfile.s"

.data
my_filename: .asciz "hello_kstdlib.s"
err_msg:     .asciz "ERROR: Could not open file!\n"
file_buffer: .space 2048

.text
.global _start
_start:
    # 1. Open the file (Macros are now safe!)
    la a0, my_filename
    open_file a0, O_RDONLY
    bltz a0, .error_handler   # If a0 < 0, file didn't open
    mv s0, a0                 # Save File Descriptor to s0

    # 2. Read the file into our buffer
    la a0, file_buffer
    li a1, 2048
    read_file s0, a0, a1
    mv s1, a0                 # Save bytes read to s1

    # 3. Print a welcome message and a newline
    la a0, my_filename
    call strlen
    mv a1, a0                 # Length in a1
    la a0, my_filename        # Pointer in a0
    print a0, a1

    print_char '\n'
    print_char '-'
    print_char '\n'

    # 4. Print the file contents
    la a0, file_buffer
    print a0, s1              # print buffer, bytes_read

    # 5. Close the file and Exit
    close_file s0
    exit 0

.error_handler:
    neg a0, a0        # Make the error code positive for bash
    li a7, 93         # exit
    ecall
