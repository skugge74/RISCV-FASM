#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define PARSE_ERROR INT_MAX
#define MAX_SYMBOLS 1024
#define MAX_LINE_LEN 256


typedef struct {
    char name[128];
    uint32_t address;
} Symbol;



extern Symbol symbol_table[MAX_SYMBOLS];
extern int symbol_count;
extern char current_parent[64];


uint32_t encode_R_type(int opcode, int funct3, int funct7, int rs1, int rs2, int rdst);
uint32_t encode_I_type(int opcode, int funct3, int imm, int rs1, int rdst);
uint32_t encode_S_type(int opcode, int funct3, int rsrc, int imm, int rdst);
uint32_t encode_B_type(int opcode, int funct3, int rs1, int rs2, int imm);
uint32_t encode_J_type(int opcode, int rdest, int imm);
uint32_t encode_instruction(char* name, int a1, int a2, int a3);

void add_symbol(const char* name, uint32_t addr);
int find_symbol(const char* name);
int resolve_val(const char* arg, uint32_t current_pc);
int find_macro(const char *name);
void dump_symbol_table();

void simplify_punct(char *str);
bool split_line(const char *str, char *ins, char *arg1, char *arg2, char *arg3, char *arg4);
bool parse_arg(const char *arg, int *out_val);

void process_pass(FILE *fp, FILE *fp_out, bool write_mode);
void process_instruction(char *line, uint32_t *pc, FILE *fp_out, bool write_mode);
#endif
