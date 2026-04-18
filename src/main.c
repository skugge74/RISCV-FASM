#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "assembler.h"

// Professional UI Colors
#define CLR_RESET  "\033[0m"
#define CLR_HEADER "\033[1;36m"
#define CLR_OK     "\033[1;32m"
#define CLR_ERROR  "\033[1;31m"
#define CLR_INFO   "\033[1;34m"
#define CLR_BOLD   "\033[1m"

bool quiet_mode = false;

void print_banner() {
    if (quiet_mode) return;
    printf(CLR_HEADER "┌────────────────────────────────────────────────────────┐\n");
    printf("│  " CLR_BOLD "RISC-V-FASM" CLR_RESET CLR_HEADER " - Toolchain Architecture                  │\n");
    printf("│  Target: RV32I | Version 1.1                           │\n");
    printf("└────────────────────────────────────────────────────────┘\n" CLR_RESET);
}

    bool output_elf = false;
int main(int argc, char **argv) {
    clock_t start_time = clock();
    char *input_file = NULL;
    char *output_file = NULL;

    // --- SMART ARGUMENT PARSING ---
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            quiet_mode = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                if (strcmp(argv[i+1], "elf") == 0) output_elf = true;
                i++;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[i+1];
                i++;
            }
        } else {
            input_file = argv[i];
        }
    }

    print_banner();

    if (!input_file) {
        printf(CLR_ERROR "Error: No input file specified.\n" CLR_RESET);
        printf("Usage: riscv-fasm [-q] [-f flat|elf] [-o output] <input.s>\n");
        return 1;
    }

    // --- AUTOMATIC FILENAME GENERATION ---
    char auto_out[256];
    if (!output_file) {
        strncpy(auto_out, input_file, 250);
        char *dot = strrchr(auto_out, '.');
        if (dot) *dot = '\0';
        strcat(auto_out, output_elf ? ".o" : ".bin");
        output_file = auto_out;
    }

    // --- FILE EXECUTION ---
    if (!quiet_mode) printf(CLR_INFO "[1/3]" CLR_RESET " Pre-processing: %s\n", input_file);
    FILE *in = fopen(input_file, "r");
    if (!in) { 
        printf(CLR_ERROR "IO Error: Could not open %s\n" CLR_RESET, input_file); 
        return 1; 
    }
    
    init_assembler_total();
    init_assembler_pass();
    process_pass(in, false, input_file); 
    
    if (!quiet_mode) printf(CLR_INFO "[2/3]" CLR_RESET " Encoding Instructions...\n");
    rewind(in);
    init_assembler_pass();
    process_pass(in, true, input_file);
    fclose(in);

    if (!compile) {
        if (!quiet_mode) printf("\n" CLR_ERROR "✖ BUILD FAILED" CLR_RESET "\n");
        return 1;
    }

    if (!quiet_mode) printf(CLR_INFO "[3/3]" CLR_RESET " Writing %s to %s...\n", 
           output_elf ? "ELF" : "Binary", output_file);
           
    if (output_elf) save_elf(output_file);
    else save_binary(output_file);

    // --- FINAL SUMMARY ---
    if (!quiet_mode) {
        double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
        printf("\n" CLR_OK "✔ Build Successful" CLR_RESET "\n");
        printf("──────────────────────────────────────────────────────────\n");
        printf("  %-12s %s\n", CLR_BOLD "Output:" CLR_RESET, output_file);
        printf("  %-12s %s\n", CLR_BOLD "Format:" CLR_RESET, output_elf ? "ELF (Relocatable)" : "Flat Binary");
printf("  %-12s %d bytes\n", CLR_BOLD "Size:" CLR_RESET, as_state.text_size + as_state.data_size);
        printf("  %-12s %d bytes\n", CLR_BOLD "Size:" CLR_RESET, as_state.text_size + as_state.data_size);
        printf("  %-12s %.4fs\n", CLR_BOLD "Time:" CLR_RESET, elapsed);
        printf("──────────────────────────────────────────────────────────\n\n");
    }

    return 0;
}
