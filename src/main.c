#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "assembler.h"

int main(int argc, char **argv) {
    if (argc != 3) { printf("Usage: %s <in.s> <out.hex>\n", argv[0]); return 1; }
    FILE *in = fopen(argv[1], "r"), *out = fopen(argv[2], "w");
    if (!in || !out) { perror("Error"); return 1; }

    process_pass(in, NULL, false);
    rewind(in);
    process_pass(in, out, true);

    fclose(in); fclose(out);
    printf("Assembly complete. Symbols found: %d\n", symbol_count);
    return 0;
}
