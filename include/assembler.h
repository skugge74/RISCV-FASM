#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#define MAX_SYMBOLS 1024
#define MAX_LINE_LEN 256
#define MAX_BIN_SIZE 131072 

typedef enum { SECTION_TEXT, SECTION_DATA } SectionType;

typedef struct {
    uint8_t image[MAX_BIN_SIZE]; 
    uint32_t size;               // Current byte offset in the flat file
    uint32_t origin;             // Hardware base address (from .org)
    SectionType current_section;
} Assembler;

typedef struct {
    char name[128];
    uint32_t address;
} Symbol;

extern Assembler as_state;
extern Symbol symbol_table[MAX_SYMBOLS];
extern int symbol_count;
extern char current_parent[64];

// Prototypes
uint32_t encode_R_type(int op, int f3, int f7, int rs1, int rs2, int rd);
uint32_t encode_I_type(int op, int f3, int imm, int rs1, int rd);
uint32_t encode_U_type(int op, int rd, int imm);
uint32_t encode_S_type(int op, int f3, int rs2, int imm, int rs1);
uint32_t encode_B_type(int op, int f3, int rs1, int rs2, int imm);
uint32_t encode_J_type(int op, int rd, int imm);
uint32_t encode_instruction(char* name, int a1, int a2, int a3);
void add_symbol(const char* name, uint32_t offset);
int find_symbol(const char* name);
int resolve_val(const char* name, uint32_t current_offset, bool is_relative);
void simplify_punct(char *str);
bool split_line(const char *str, char *ins, char *arg1, char *arg2, char *arg3, char *arg4);
bool parse_arg(const char *arg, int *out_val);
void process_pass(FILE *fp, bool write_mode);
void process_instruction(char *line, bool write_mode);
void handle_directive(char *directive, char *args, bool write_mode);
void init_assembler_pass();
void init_assembler_total();
void save_binary(const char* filename);

#endif
