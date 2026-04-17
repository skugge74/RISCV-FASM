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
    process_pass(in, false, argv[1]); // Note: Pass input filename here
    
    // FREEZE the labels found in Pass 1 for use in Pass 2
    pass1_anon_count = anon_count;
    memcpy(pass1_anon_labels, anon_labels, sizeof(anon_labels));
    
    rewind(in);

    // --- PASS 2 ---
    init_assembler_pass();
    process_pass(in, true, argv[1]);




  

  
    dump_symbol_table();
    if (!compile)exit(1);
    save_binary(argv[2]);

    fclose(in);
    printf("Assembly complete. Symbols found: %d\n", symbol_count);
    return 0;
}
