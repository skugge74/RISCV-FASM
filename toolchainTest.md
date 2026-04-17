# 1. Assemble your code into an ELF component
./riscv-fasm -f elf tests/print.s tests/print.o

# 2. Compile the C code with GCC
riscv64-linux-gnu-gcc -c tests/hello.c -o tests/hello.o

# 3. Link them together
riscv64-linux-gnu-gcc tests/hello.o tests/print.o -o final_program.elf -nostdlib
