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

    // Initialize the global assembler state (Clear buffers and set section to .text)
    init_assembler_total();

    // PASS 1: Build the Symbol Table
    // This populates addresses for both .text and .data labels
  
    init_assembler_pass();
    process_pass(in, false, argv[2]);
    dump_symbol_table();
    // Rewind for the second pass
    rewind(in);

    // PASS 2: Encode instructions and data into internal buffers
  
    init_assembler_pass();
    process_pass(in, true, argv[2]);
    dump_symbol_table();
    // FINAL STEP: Write the accumulated buffers to the output file
    // This merges the .text and .data sections into the final hex file
    save_binary(argv[2]);

    fclose(in);
    printf("Assembly complete. Symbols found: %d\n", symbol_count);
    return 0;
}
