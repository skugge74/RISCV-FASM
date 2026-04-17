#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <elf.h>

// ==========================================
// CONSTANTS
// ==========================================
#define MAX_SYMBOLS   1024
#define MAX_LINE_LEN  256
#define MAX_BIN_SIZE  65536
#define MAX_ARGS      16
#define MAX_RELOCS    1024

// Symbol flags
#define SYM_FLAG_GLOBAL  (1 << 0)
#define SYM_FLAG_EXTERN  (1 << 1)

// RISC-V ELF relocation types
#define R_RISCV_32        1
#define R_RISCV_JAL       17
#define R_RISCV_CALL      18 
#define R_RISCV_HI20      26
#define R_RISCV_LO12_I    27
#define R_RISCV_LO12_S    28

// ==========================================
// TYPES
// ==========================================
typedef enum {
    SECTION_ABS = 0,
    SECTION_TEXT,
    SECTION_DATA,
    SECTION_BSS
} SectionType;

typedef struct {
    uint8_t     text[MAX_BIN_SIZE];
    uint32_t    text_size;
    
    uint8_t     data[MAX_BIN_SIZE];
    uint32_t    data_size;

    uint32_t    bss_size;

    // Locked in at the end of Pass 1 for absolute virtual addressing
    uint32_t    final_text_size; 
    uint32_t    final_data_size; // <-- The missing piece!

    uint32_t    origin;          // Hardware base address (from .org)
    SectionType current_section;
} Assembler;

typedef struct {
    char        name[128];
    uint32_t    address;
    uint8_t     flags;
    SectionType section;         // Tracks where the symbol lives for the ELF Linker
} Symbol;

// ==========================================
// GLOBAL STATE
// ==========================================
extern Assembler  as_state;
extern Symbol     symbol_table[MAX_SYMBOLS];
extern int        symbol_count;
extern char       current_parent[64];
extern bool       compile;
extern int        current_line;
extern const char *current_file;
extern int        current_pass;

extern uint32_t   anon_labels[MAX_SYMBOLS];
extern int        anon_count;
extern uint32_t   pass1_anon_labels[MAX_SYMBOLS];
extern int        pass1_anon_count;

extern Elf32_Rela relocation_table[MAX_RELOCS];
extern int        relocation_count;

// ==========================================
// FUNCTION PROTOTYPES
// ==========================================
uint32_t encode_R_type(int op, int f3, int f7, int rs1, int rs2, int rd);
uint32_t encode_I_type(int op, int f3, int imm, int rs1, int rd);
uint32_t encode_U_type(int op, int rd, int imm);
uint32_t encode_S_type(int op, int f3, int rs2, int imm, int rs1);
uint32_t encode_B_type(int op, int f3, int rs1, int rs2, int imm);
uint32_t encode_J_type(int op, int rd, int imm);
uint32_t encode_instruction(char *name, int a1, int a2, int a3);

void add_symbol(const char *name, uint32_t offset);
int  find_symbol(const char *name);
void add_or_update_variable(const char *name, int value);
void add_relocation(uint32_t offset, const char *name, int type);

char *trim_whitespace(char *str);
void  simplify_punct(char *str);
bool  split_line(const char *str, char *ins, char args[MAX_ARGS][128], int *arg_count);
bool  parse_arg(const char *arg, int *out_val);
int   try_parse_reg(const char *arg, int *out_val);
int   resolve_val(const char *name, uint32_t current_offset, bool is_relative);
int   eval_math(char *expr, uint32_t current_offset, bool is_relative);

void init_assembler_total();
void init_assembler_pass();
void process_pass(FILE *fp, bool write_mode, const char *filename);
void process_line(char *line, bool write_mode);
void handle_directive(char *directive, char *args_str, char args[MAX_ARGS][128], int arg_count, bool write_mode);

void save_binary(const char *filename);
void save_elf(const char *filename);
void dump_symbol_table();
void panic(const char *fmt, ...);

#endif // ASSEMBLER_H
