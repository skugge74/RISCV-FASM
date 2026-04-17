#include <stdio.h>
#include <stdlib.h>
#include "assembler.h"

int main(int argc, char **argv) {
    if (argc != 3) { 
        printf("Usage: %s <input.s> <output.hex>\n", argv[0]); 
        return 1; 
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) { 
        perror("Error opening input file"); 
        return 1; 
    }

    init_assembler_total();

    // --- PASS 1 ---
    init_assembler_pass();
    process_pass(in, false, argv[1]); 
    
    rewind(in);

    // --- PASS 2 ---
    init_assembler_pass();
    process_pass(in, true, argv[1]);

    // --- CLEANUP & EXIT ---
    if (!compile) {
        // If compilation failed, close the file and return an error code
        fclose(in);
        return 1; 
    }

    // Only dump symbols and save the binary if compilation succeeded
    dump_symbol_table();
    save_binary(argv[2]);

    fclose(in);
    printf("Assembly complete. Symbols found: %d\n", symbol_count);
    return 0;
}
