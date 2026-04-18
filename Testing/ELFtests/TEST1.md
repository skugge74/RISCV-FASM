Try this experiment:

    Move your kstring.s and kstdio.s logic into a single file called kstdlib.s.

    Assemble it: riscv-fasm -f elf kstdlib.s -o kstdlib.o.

    Create a main.s that just has _start and calls kprintf.

    Assemble it: riscv-fasm -f elf main.s -o main.o.

    Link them manually: riscv64-unknown-elf-ld main.o kstdlib.o -o final.elf.
