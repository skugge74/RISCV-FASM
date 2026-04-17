#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "assembler.h"
#include "preprocessor.h"

// ==========================================
// GLOBAL STATE
// ==========================================
Symbol     symbol_table[MAX_SYMBOLS];
int        symbol_count      = 0;
char       current_parent[64] = "global";
Assembler  as_state;
bool       compile           = true;
int        current_line      = 0;
const char *current_file     = "unknown";
int        current_pass      = 0;
extern bool output_elf;

uint32_t anon_labels[MAX_SYMBOLS];
int      anon_count          = 0;
uint32_t pass1_anon_labels[MAX_SYMBOLS];
int      pass1_anon_count    = 0;

Elf32_Rela relocation_table[MAX_RELOCS];
int        relocation_count  = 0;

// ==========================================
// UTILITIES
// ==========================================
void panic(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "\033[1;31m[ERROR] %s:%d (Pass %d): \033[0m",
            current_file, current_line, current_pass);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    compile = false;
}

char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void simplify_punct(char *str) {
    char buffer[MAX_LINE_LEN * 2];
    int j = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] == ',')      buffer[j++] = ' ';
        else if (str[i] == '=') { buffer[j++] = ' '; buffer[j++] = '='; buffer[j++] = ' '; }
        else                    buffer[j++] = str[i];
    }
    buffer[j] = '\0';
    strcpy(str, buffer);
}

bool split_line(const char *str, char *ins, char args[MAX_ARGS][128], int *arg_count) {
    *ins = '\0'; *arg_count = 0;
    int idx = 0, parens = 0;
    while (isspace((unsigned char)*str)) str++;
    if (!*str || *str == ';' || *str == '#') return false;

    while (*str && !isspace((unsigned char)*str) && *str != ',' && idx < 63)
        ins[idx++] = *str++;
    ins[idx] = '\0';

    while (*str && *arg_count < MAX_ARGS) {
        while (isspace((unsigned char)*str) || *str == ',') str++;
        if (!*str || *str == ';' || *str == '#') break;
        idx = 0;
        while (*str && (parens > 0 || (!isspace((unsigned char)*str) && *str != ','))) {
            if (*str == '(') parens++;
            if (*str == ')') parens--;
            if (idx < 127) args[*arg_count][idx++] = *str;
            str++;
        }
        args[*arg_count][idx] = '\0';
        (*arg_count)++;
    }
    return strlen(ins) > 0;
}

// ==========================================
// SYMBOL & REGISTER HANDLING
// ==========================================
void init_assembler_total() {
    memset(&as_state, 0, sizeof(Assembler));
    as_state.origin = 0x80000000;
    symbol_count    = 0;
    relocation_count = 0;
}

void init_assembler_pass() {
    current_line = 0;
    as_state.size = 0;
    as_state.current_section = SECTION_TEXT;
    anon_count = 0;
    unique_id_counter = 0;
}

void add_relocation(uint32_t offset, const char *name, int type) {
    if (relocation_count >= MAX_RELOCS) return;

    int sym_idx = -1;
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            sym_idx = i + 1;  // ELF symbol indices are 1-based (0 = STN_UNDEF)
            break;
        }
    }
    if (sym_idx == -1) {
        // Register as undefined extern so the linker can resolve it
        add_symbol(name, 0xFFFFFFFF);
        sym_idx = symbol_count;  // add_symbol incremented this
    }

    relocation_table[relocation_count].r_offset = offset;
    relocation_table[relocation_count].r_info   = ELF32_R_INFO(sym_idx, type);
    relocation_table[relocation_count].r_addend = 0;
    relocation_count++;
}

void add_symbol(const char *name, uint32_t offset) {
    if (symbol_count >= MAX_SYMBOLS) return;

    // Anonymous label shorthand — stored separately
    if (strcmp(name, "@@") == 0) {
        if (anon_count < MAX_SYMBOLS)
            anon_labels[anon_count++] = as_state.origin + offset;
        return;
    }

    char full_name[128];
    if (name[0] == '.') snprintf(full_name, 128, "%s%s", current_parent, name);
    else {
        if (offset != 0xFFFFFFFF) strncpy(current_parent, name, 63);
        strcpy(full_name, name);
    }

    // Auto-promote _start to global so the linker can find it
    uint8_t auto_flags = 0;
    if (strcmp(full_name, "_start") == 0)
        auto_flags |= SYM_FLAG_GLOBAL;

    // Merge with a pre-registered entry (.global / .extern declared before label)
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, full_name) == 0) {
            if (offset != 0xFFFFFFFF)
                symbol_table[i].address = as_state.origin + offset;
            symbol_table[i].flags |= auto_flags;
            return;
        }
    }

    // New entry
    strcpy(symbol_table[symbol_count].name, full_name);
    symbol_table[symbol_count].address = (offset == 0xFFFFFFFF)
                                         ? 0xFFFFFFFF
                                         : (as_state.origin + offset);
    symbol_table[symbol_count].flags   = auto_flags;
    symbol_count++;
}

int find_symbol(const char *name) {
    for (int i = 0; i < symbol_count; i++)
        if (strcmp(symbol_table[i].name, name) == 0)
            return (int)symbol_table[i].address;

    if (name[0] == '.') {
        char full[128];
        snprintf(full, 128, "%s%s", current_parent, name);
        for (int i = 0; i < symbol_count; i++)
            if (strcmp(symbol_table[i].name, full) == 0)
                return (int)symbol_table[i].address;
    }
    return -1;
}

void add_or_update_variable(const char *name, int value) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].address = value;
            return;
        }
    }
    strncpy(symbol_table[symbol_count].name, name, 127);
    symbol_table[symbol_count].address = value;
    symbol_table[symbol_count].flags   = 0;
    symbol_count++;
}

bool parse_arg(const char *arg, int *out_val) {
    if (!arg || !*arg) return false;
    char buffer[128];
    strncpy(buffer, arg, 127);
    buffer[127] = '\0';
    char *ptr = trim_whitespace(buffer);

    if (ptr[0] == 'x' && isdigit(ptr[1])) {
        int n = atoi(ptr + 1);
        if (n < 0 || n > 31) return false;
        *out_val = n; return true;
    }
    if (!strcmp(ptr, "zero")) { *out_val =  0; return true; }
    if (!strcmp(ptr, "ra"))   { *out_val =  1; return true; }
    if (!strcmp(ptr, "sp"))   { *out_val =  2; return true; }
    if (!strcmp(ptr, "gp"))   { *out_val =  3; return true; }
    if (!strcmp(ptr, "tp"))   { *out_val =  4; return true; }
    if (!strcmp(ptr, "fp") || !strcmp(ptr, "s0")) { *out_val = 8; return true; }
    if (!strcmp(ptr, "s1"))   { *out_val =  9; return true; }
    if (ptr[0] == 't' && isdigit(ptr[1])) {
        int n = atoi(ptr + 1);
        if (n < 0 || n > 6) return false;
        *out_val = (n <= 2) ? (n + 5) : (n + 25); return true;
    }
    if (ptr[0] == 's' && isdigit(ptr[1])) {
        int n = atoi(ptr + 1);
        if (n < 0 || n > 11) return false;
        *out_val = (n == 0) ? 8 : (n == 1 ? 9 : n + 16); return true;
    }
    if (ptr[0] == 'a' && isdigit(ptr[1])) {
        int n = atoi(ptr + 1);
        if (n < 0 || n > 7) return false;
        *out_val = n + 10; return true;
    }
    char *endptr;
    long val = strtol(ptr, &endptr, 0);
    if (endptr != ptr && *endptr == '\0') { *out_val = (int)val; return true; }
    return false;
}

int try_parse_reg(const char *arg, int *out_val) {
    if (!arg || !*arg || isdigit((unsigned char)arg[0])) return 0;
    if (parse_arg(arg, out_val)) return 1;
    return 0;
}

int resolve_val(const char *name, uint32_t current_offset, bool is_relative) {
    uint32_t absolute_pc = as_state.origin + current_offset;

    if (name[0] == '@') {
        int target = -1;
        if (name[1] == 'f') {
            for (int i = 0; i < pass1_anon_count; i++)
                if (pass1_anon_labels[i] > absolute_pc) { target = pass1_anon_labels[i]; break; }
        } else {
            for (int i = pass1_anon_count - 1; i >= 0; i--)
                if (pass1_anon_labels[i] < absolute_pc) { target = pass1_anon_labels[i]; break; }
        }
        if (target != -1)
            return is_relative ? (int)((int64_t)target - (int64_t)absolute_pc) : target;
        return 0;
    }

    if (strcmp(name, "$") == 0)
        return is_relative ? 0 : (int)absolute_pc;
    if (strcmp(name, "$$") == 0)
        return is_relative ? (int)(as_state.origin - absolute_pc) : (int)as_state.origin;

    int val;
    if (parse_arg(name, &val)) return val;

    int addr = find_symbol(name);
    if (addr != -1 && (uint32_t)addr != 0xFFFFFFFF)
        return is_relative ? (addr - (int32_t)absolute_pc) : addr;

    // In pass 2: only panic for symbols that ELF cannot resolve externally
    if (current_pass == 2) {
        if (!output_elf || strchr(name, '.') != NULL) {
            panic("Undefined symbol: '%s'", name);
        }
    }
    return 0;
}

int eval_math(char *expr, uint32_t current_offset, bool is_relative) {
    char buffer[128];
    strncpy(buffer, expr, 127);
    buffer[127] = '\0';
    char *str = trim_whitespace(buffer);
    if (!*str) return 0;

    int parens = 0, split_idx = -1, lowest_prec = 999;
    for (int i = 0; str[i]; i++) {
        if      (str[i] == '(') parens++;
        else if (str[i] == ')') parens--;
        else if (parens == 0) {
            int prec = 999;
            if      (str[i] == '<' && str[i+1] == '<') prec = 4;
            else if (str[i] == '>' && str[i+1] == '>') prec = 4;
            else if (str[i] == '|')                     prec = 1;
            else if (str[i] == '^')                     prec = 2;
            else if (str[i] == '&')                     prec = 3;
            else if (str[i] == '+' || str[i] == '-')    prec = 5;
            else if (str[i] == '*' || str[i] == '/')    prec = 6;
            if (prec <= lowest_prec && prec < 999) { lowest_prec = prec; split_idx = i; }
        }
    }

    if (split_idx != -1) {
        int  op_len  = (str[split_idx] == '<' || str[split_idx] == '>') ? 2 : 1;
        char op_char = str[split_idx];
        str[split_idx] = '\0';
        int left  = eval_math(str,                   current_offset, false);
        int right = eval_math(str + split_idx + op_len, current_offset, false);
        int result = 0;
        if (op_len == 2) {
            if (op_char == '<') result = left << right;
            else                result = left >> right;
        } else {
            switch (op_char) {
                case '|': result = left | right; break;
                case '^': result = left ^ right; break;
                case '&': result = left & right; break;
                case '+': result = left + right; break;
                case '-': result = left - right; break;
                case '*': result = left * right; break;
                case '/': result = (right != 0) ? (left / right) : 0; break;
            }
        }
        return is_relative
               ? (int)(result - (int32_t)(as_state.origin + current_offset))
               : result;
    }

    if (str[0] == '(' && str[strlen(str) - 1] == ')') {
        str[strlen(str) - 1] = '\0';
        return eval_math(str + 1, current_offset, is_relative);
    }
    return resolve_val(str, current_offset, is_relative);
}

// ==========================================
// ENCODING
// ==========================================
uint32_t encode_R_type(int op, int f3, int f7, int rs1, int rs2, int rd) {
    return (op & 0x7F) | ((rd & 0x1F) << 7) | ((f3 & 0x7) << 12)
         | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | ((f7 & 0x7F) << 25);
}
uint32_t encode_I_type(int op, int f3, int imm, int rs1, int rd) {
    return (op & 0x7F) | ((rd & 0x1F) << 7) | ((f3 & 0x7) << 12)
         | ((rs1 & 0x1F) << 15) | ((imm & 0xFFF) << 20);
}
uint32_t encode_U_type(int op, int rd, int imm) {
    return (op & 0x7F) | ((rd & 0x1F) << 7) | (imm & 0xFFFFF000);
}
uint32_t encode_S_type(int op, int f3, int rs2, int imm, int rs1) {
    return (op & 0x7F) | ((imm & 0x1F) << 7) | ((f3 & 0x7) << 12)
         | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | (((imm >> 5) & 0x7F) << 25);
}
uint32_t encode_B_type(int op, int f3, int rs1, int rs2, int imm) {
    uint32_t u = (uint32_t)imm;
    return (op & 0x7F) | (((u >> 11) & 0x1) << 7) | (((u >> 1) & 0xF) << 8)
         | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20)
         | (((u >> 5) & 0x3F) << 25) | (((u >> 12) & 0x1) << 31);
}
uint32_t encode_J_type(int op, int rd, int imm) {
    uint32_t u = (uint32_t)imm;
    return (op & 0x7F) | ((rd & 0x1F) << 7) | (((u >> 12) & 0xFF) << 12)
         | (((u >> 11) & 0x1) << 20) | (((u >> 1) & 0x3FF) << 21)
         | (((u >> 20) & 0x1) << 31);
}

uint32_t encode_instruction(char *name, int a1, int a2, int a3) {
    // R-type
    if (!strcmp(name, "add"))   return encode_R_type(0x33, 0x0, 0x00, a2, a3, a1);
    if (!strcmp(name, "sub"))   return encode_R_type(0x33, 0x0, 0x20, a2, a3, a1);
    if (!strcmp(name, "sll"))   return encode_R_type(0x33, 0x1, 0x00, a2, a3, a1);
    if (!strcmp(name, "slt"))   return encode_R_type(0x33, 0x2, 0x00, a2, a3, a1);
    if (!strcmp(name, "sltu"))  return encode_R_type(0x33, 0x3, 0x00, a2, a3, a1);
    if (!strcmp(name, "xor"))   return encode_R_type(0x33, 0x4, 0x00, a2, a3, a1);
    if (!strcmp(name, "srl"))   return encode_R_type(0x33, 0x5, 0x00, a2, a3, a1);
    if (!strcmp(name, "sra"))   return encode_R_type(0x33, 0x5, 0x20, a2, a3, a1);
    if (!strcmp(name, "or"))    return encode_R_type(0x33, 0x6, 0x00, a2, a3, a1);
    if (!strcmp(name, "and"))   return encode_R_type(0x33, 0x7, 0x00, a2, a3, a1);
    // M extension
    if (!strcmp(name, "mul"))   return encode_R_type(0x33, 0x0, 0x01, a2, a3, a1);
    if (!strcmp(name, "div"))   return encode_R_type(0x33, 0x4, 0x01, a2, a3, a1);
    if (!strcmp(name, "divu"))  return encode_R_type(0x33, 0x5, 0x01, a2, a3, a1);
    if (!strcmp(name, "rem"))   return encode_R_type(0x33, 0x6, 0x01, a2, a3, a1);
    if (!strcmp(name, "remu"))  return encode_R_type(0x33, 0x7, 0x01, a2, a3, a1);
    // I-type ALU
    if (!strcmp(name, "addi"))  return encode_I_type(0x13, 0x0, a3, a2, a1);
    if (!strcmp(name, "slti"))  return encode_I_type(0x13, 0x2, a3, a2, a1);
    if (!strcmp(name, "sltiu")) return encode_I_type(0x13, 0x3, a3, a2, a1);
    if (!strcmp(name, "xori"))  return encode_I_type(0x13, 0x4, a3, a2, a1);
    if (!strcmp(name, "ori"))   return encode_I_type(0x13, 0x6, a3, a2, a1);
    if (!strcmp(name, "andi"))  return encode_I_type(0x13, 0x7, a3, a2, a1);
    if (!strcmp(name, "slli"))  return encode_I_type(0x13, 0x1, a3 & 0x1F,            a2, a1);
    if (!strcmp(name, "srli"))  return encode_I_type(0x13, 0x5, a3 & 0x1F,            a2, a1);
    if (!strcmp(name, "srai"))  return encode_I_type(0x13, 0x5, (a3 & 0x1F) | 0x400,  a2, a1);
    // Loads
    if (!strcmp(name, "lb"))    return encode_I_type(0x03, 0x0, a2, a3, a1);
    if (!strcmp(name, "lh"))    return encode_I_type(0x03, 0x1, a2, a3, a1);
    if (!strcmp(name, "lw"))    return encode_I_type(0x03, 0x2, a2, a3, a1);
    if (!strcmp(name, "lbu"))   return encode_I_type(0x03, 0x4, a2, a3, a1);
    if (!strcmp(name, "lhu"))   return encode_I_type(0x03, 0x5, a2, a3, a1);
    // CSR
    if (!strcmp(name, "csrrw")) return encode_I_type(0x73, 0x1, a2, a3, a1);
    if (!strcmp(name, "csrrs")) return encode_I_type(0x73, 0x2, a2, a3, a1);
    if (!strcmp(name, "csrrc")) return encode_I_type(0x73, 0x3, a2, a3, a1);
    if (!strcmp(name, "csrr"))  return encode_I_type(0x73, 0x2, a2,  0, a1);
    if (!strcmp(name, "csrw"))  return encode_I_type(0x73, 0x1, a1, a2,  0);
    if (!strcmp(name, "csrc"))  return encode_I_type(0x73, 0x3, a1, a2,  0);
    if (!strcmp(name, "csrs"))  return encode_I_type(0x73, 0x2, a1, a2,  0);
    // Stores
    if (!strcmp(name, "sb"))    return encode_S_type(0x23, 0x0, a1, a2, a3);
    if (!strcmp(name, "sh"))    return encode_S_type(0x23, 0x1, a1, a2, a3);
    if (!strcmp(name, "sw"))    return encode_S_type(0x23, 0x2, a1, a2, a3);
    // Branches
    if (!strcmp(name, "beq"))   return encode_B_type(0x63, 0x0, a1, a2, a3);
    if (!strcmp(name, "bne"))   return encode_B_type(0x63, 0x1, a1, a2, a3);
    if (!strcmp(name, "blt"))   return encode_B_type(0x63, 0x4, a1, a2, a3);
    if (!strcmp(name, "bge"))   return encode_B_type(0x63, 0x5, a1, a2, a3);
    if (!strcmp(name, "bltu"))  return encode_B_type(0x63, 0x6, a1, a2, a3);
    if (!strcmp(name, "bgeu"))  return encode_B_type(0x63, 0x7, a1, a2, a3);
    if (!strcmp(name, "beqz"))  return encode_B_type(0x63, 0x0, a1,  0, a3);
    if (!strcmp(name, "bnez"))  return encode_B_type(0x63, 0x1, a1,  0, a3);
    if (!strcmp(name, "bgt"))   return encode_B_type(0x63, 0x4, a2, a1, a3);
    if (!strcmp(name, "bgtz"))  return encode_B_type(0x63, 0x4,  0, a1, a3);
    if (!strcmp(name, "blez"))  return encode_B_type(0x63, 0x5,  0, a1, a3);
    if (!strcmp(name, "ble"))   return encode_B_type(0x63, 0x5, a2, a1, a3);
    if (!strcmp(name, "bgtu"))  return encode_B_type(0x63, 0x6, a2, a1, a3);
    if (!strcmp(name, "bleu"))  return encode_B_type(0x63, 0x7, a2, a1, a3);
    // Jumps
    if (!strcmp(name, "jal"))   return encode_J_type(0x6F, a1, a2);
    if (!strcmp(name, "jalr"))  return encode_I_type(0x67, 0x0, a3, a2, a1);
    // Upper immediates
    if (!strcmp(name, "lui"))   return encode_U_type(0x37, a1, a2);
    if (!strcmp(name, "auipc")) return encode_U_type(0x17, a1, a2);
    // System
    if (!strcmp(name, "ecall"))  return 0x00000073;
    if (!strcmp(name, "ebreak")) return 0x00100073;
    if (!strcmp(name, "mret"))   return 0x30200073;
    if (!strcmp(name, "sret"))   return 0x10200073;
    if (!strcmp(name, "wfi"))    return 0x10500073;
    // Pseudo-ops encoded directly
    if (!strcmp(name, "li"))    return encode_I_type(0x13, 0x0, a2, 0, a1);
    if (!strcmp(name, "mv"))    return encode_I_type(0x13, 0x0,  0, a2, a1);
    if (!strcmp(name, "j"))     return encode_J_type(0x6F, 0, a2);
    if (!strcmp(name, "ret"))   return encode_I_type(0x67, 0x0, 0, 1, 0);
    if (!strcmp(name, "nop"))   return encode_I_type(0x13, 0x0, 0, 0, 0);
    return 0;
}

// ==========================================
// DIRECTIVES
// ==========================================
void handle_directive(char *directive, char *args_str, char args[MAX_ARGS][128],
                      int arg_count, bool write_mode) {

    // ---- .include -------------------------------------------------------
    if (!strcmp(directive, ".include")) {
        char filename[128];
        sscanf(args_str, " \"%[^\"]\"", filename);
        FILE *inc_fp = fopen(filename, "r");
        if (inc_fp) {
            const char *saved_file = current_file;
            int          saved_line = current_line;
            char line[MAX_LINE_LEN];
            current_file = filename; current_line = 0;
            while (fgets(line, sizeof(line), inc_fp)) {
                current_line++;
                line[strcspn(line, "\r\n")] = '\0';
                process_line(line, write_mode);
            }
            fclose(inc_fp);
            current_file = saved_file;
            current_line = saved_line;
        }
        return;
    }

    // ---- .global / .globl -----------------------------------------------
    if (!strcmp(directive, ".global") || !strcmp(directive, ".globl")) {
        if (arg_count < 1) return;
        // Try to flag an existing symbol
        for (int i = 0; i < symbol_count; i++) {
            if (strcmp(symbol_table[i].name, args[0]) == 0) {
                symbol_table[i].flags |= SYM_FLAG_GLOBAL;
                return;
            }
        }
        // Pre-register so the flag is set when the label is defined later
        strncpy(symbol_table[symbol_count].name, args[0], 127);
        symbol_table[symbol_count].address = 0xFFFFFFFF;
        symbol_table[symbol_count].flags   = SYM_FLAG_GLOBAL;
        symbol_count++;
        return;
    }

    // ---- .extern --------------------------------------------------------
    if (!strcmp(directive, ".extern")) {
        if (arg_count < 1) return;
        // Don't add duplicates
        for (int i = 0; i < symbol_count; i++)
            if (strcmp(symbol_table[i].name, args[0]) == 0) return;
        strncpy(symbol_table[symbol_count].name, args[0], 127);
        symbol_table[symbol_count].address = 0xFFFFFFFF;   // SHN_UNDEF
        symbol_table[symbol_count].flags   = SYM_FLAG_EXTERN;
        symbol_count++;
        return;
    }

    // ---- .equ / .set ----------------------------------------------------
    if (!strcmp(directive, ".equ") || !strcmp(directive, ".set")) {
        char *comma = strchr(args_str, ',');
        if (comma) {
            int val = eval_math(comma + 1, as_state.size, false);
            add_or_update_variable(args[0], val);
        } else if (arg_count >= 2) {
            int val = eval_math(args[1], as_state.size, false);
            add_or_update_variable(args[0], val);
        }
        return;
    }

    // ---- Section switches -----------------------------------------------
    if (!strcmp(directive, ".text")) { as_state.current_section = SECTION_TEXT; return; }
    if (!strcmp(directive, ".data")) { as_state.current_section = SECTION_DATA; return; }

    // ---- .org -----------------------------------------------------------
    if (!strcmp(directive, ".org")) {
        if (arg_count > 0)
            as_state.origin = (uint32_t)eval_math(args[0], as_state.size, false);
        return;
    }

    // ---- .word ----------------------------------------------------------
    if (!strcmp(directive, ".word")) {
        for (int i = 0; i < arg_count; i++) {
            while ((as_state.origin + as_state.size) % 4 != 0) {
                if (write_mode) as_state.image[as_state.size] = 0;
                as_state.size++;
            }
            if (write_mode) {
                uint32_t val = (uint32_t)eval_math(args[i], as_state.size, false);
                memcpy(&as_state.image[as_state.size], &val, 4);
            }
            as_state.size += 4;
        }
        return;
    }

    // ---- .half / .short -------------------------------------------------
    if (!strcmp(directive, ".half") || !strcmp(directive, ".short")) {
        for (int i = 0; i < arg_count; i++) {
            while ((as_state.origin + as_state.size) % 2 != 0) {
                if (write_mode) as_state.image[as_state.size] = 0;
                as_state.size++;
            }
            if (write_mode) {
                uint16_t val = (uint16_t)eval_math(args[i], as_state.size, false);
                memcpy(&as_state.image[as_state.size], &val, 2);
            }
            as_state.size += 2;
        }
        return;
    }

    // ---- .byte ----------------------------------------------------------
    if (!strcmp(directive, ".byte")) {
        char *ptr = args_str;
        while (*ptr) {
            while (isspace((unsigned char)*ptr)) ptr++;
            if (!*ptr) break;
            char *end = ptr;
            while (*end && !isspace((unsigned char)*end) && *end != ',') end++;
            char expr[128];
            int  len = (int)(end - ptr);
            if (len > 127) len = 127;
            strncpy(expr, ptr, len); expr[len] = '\0';
            uint8_t val = (uint8_t)eval_math(expr, as_state.size, false);
            if (write_mode) as_state.image[as_state.size] = val;
            as_state.size++;
            ptr = end;
            while (*ptr == ',' || isspace((unsigned char)*ptr)) ptr++;
        }
        return;
    }

    // ---- .fill ----------------------------------------------------------
    if (!strcmp(directive, ".fill")) {
        int repeat = (arg_count > 0) ? eval_math(args[0], as_state.size, false) : 0;
        int size   = (arg_count > 1) ? eval_math(args[1], as_state.size, false) : 1;
        int value  = (arg_count > 2) ? eval_math(args[2], as_state.size, false) : 0;
        for (int i = 0; i < repeat; i++) {
            if (write_mode) {
                uint32_t val = value;
                memcpy(&as_state.image[as_state.size], &val, size);
            }
            as_state.size += size;
        }
        return;
    }

    // ---- .space ---------------------------------------------------------
    if (!strcmp(directive, ".space")) {
        int num_bytes = (arg_count > 0) ? eval_math(args[0], as_state.size, false) : 0;
        for (int i = 0; i < num_bytes; i++) {
            if (write_mode) as_state.image[as_state.size] = 0;
            as_state.size++;
        }
        return;
    }

    // ---- .align ---------------------------------------------------------
    if (!strcmp(directive, ".align")) {
        int align_val = (arg_count > 0) ? eval_math(args[0], as_state.size, false) : 4;
        if (align_val <= 0) align_val = 4;
        while ((as_state.origin + as_state.size) % align_val != 0) {
            if (write_mode) as_state.image[as_state.size] = 0;
            as_state.size++;
        }
        return;
    }

    // ---- .asciz ---------------------------------------------------------
    if (!strcmp(directive, ".asciz")) {
        char *start = strchr(args_str, '"'), *end = strrchr(args_str, '"');
        if (start && end && start != end) {
            for (char *p = start + 1; p < end; p++) {
                if (*p == '\\' && *(p+1) == 'n') {
                    if (write_mode) as_state.image[as_state.size] = 0x0A;
                    p++;
                } else {
                    if (write_mode) as_state.image[as_state.size] = *p;
                }
                as_state.size++;
            }
            if (write_mode) as_state.image[as_state.size] = '\0';
            as_state.size++;
        }
        return;
    }
}

// ==========================================
// PASS DRIVER
// ==========================================
void process_pass(FILE *fp, bool write_mode, const char *filename) {
    current_pass = write_mode ? 2 : 1;
    current_file = filename;
    current_line = 0;
    char line[MAX_LINE_LEN];
    rewind(fp);
    defining_macro = false;
    anon_count = 0;
    strcpy(current_parent, "global");

    while (fgets(line, sizeof(line), fp)) {
        current_line++;
        line[strcspn(line, "\r\n")] = '\0';
        process_line(line, write_mode);
    }
    if (!write_mode) {
        memcpy(pass1_anon_labels, anon_labels, sizeof(anon_labels));
        pass1_anon_count = anon_count;
    }
}

// ==========================================
// LINE PROCESSING
// ==========================================
void process_line(char *line, bool write_mode) {
    // Strip comments (respect strings)
    bool in_string = false;
    for (int i = 0; line[i]; i++) {
        if (line[i] == '"') in_string = !in_string;
        if (!in_string && line[i] == ';') { line[i] = '\0'; break; }
        if (!in_string && line[i] == '#') {
            if (i == 0 || line[i-1] != '%') { line[i] = '\0'; break; }
        }
    }

    char work[MAX_LINE_LEN];
    strcpy(work, line);
    simplify_punct(work);

    char ins[64], args[MAX_ARGS][128];
    int  arg_count = 0;
    if (!split_line(work, ins, args, &arg_count)) return;

    // ---- Macro definition bookkeeping -----------------------------------
    if (!strcmp(ins, "macro")) {
        defining_macro = true;
        strcpy(macro_lib[macro_lib_count].name, args[0]);
        macro_lib[macro_lib_count].line_count = 0;
        return;
    }
    if (!strcmp(ins, "endm")) {
        defining_macro = false;
        macro_lib_count++;
        return;
    }
    if (defining_macro) {
        strcpy(macro_lib[macro_lib_count].lines[macro_lib[macro_lib_count].line_count++], line);
        return;
    }

    // ---- Macro invocation -----------------------------------------------
    int m_idx = find_macro(ins);
    if (m_idx != -1) {
        int current_id;
        if (strncmp(ins, "end", 3) == 0) {
            current_id = pop_id(ins + 3);
        } else {
            current_id = ++unique_id_counter;
            char end_search[128];
            snprintf(end_search, sizeof(end_search), "end%s", ins);
            if (find_macro(end_search) != -1) push_id(current_id, ins);
        }
        for (int i = 0; i < macro_lib[m_idx].line_count; i++) {
            char expanded[MAX_LINE_LEN];
            strcpy(expanded, macro_lib[m_idx].lines[i]);
            substitute_args_with_id(expanded, args, arg_count, current_id);
            process_line(expanded, write_mode);
        }
        return;
    }

    // ---- Label definition -----------------------------------------------
    if (ins[strlen(ins) - 1] == ':') {
        ins[strlen(ins) - 1] = '\0';
        if (current_pass == 1) add_symbol(ins, as_state.size);
        if (ins[0] != '.') strncpy(current_parent, ins, 63);
        if (arg_count == 0) return;
        // Instruction on same line as label
        strcpy(ins, args[0]);
        for (int i = 0; i < arg_count - 1; i++) strcpy(args[i], args[i+1]);
        arg_count--;
    }

    // ---- Directive ------------------------------------------------------
    if (ins[0] == '.') {
        char *ptr = strstr(line, ins) + strlen(ins);
        handle_directive(ins, ptr, args, arg_count, write_mode);
        return;
    }

    // ---- Compile-time variable assignment (name = expr) -----------------
    if (arg_count > 0 && strcmp(args[0], "=") == 0) {
        char *eq_pos = strchr(work, '=');
        if (eq_pos) {
            char *math_expr = eq_pos + 1;
            char *comment   = strpbrk(math_expr, ";#");
            if (comment) *comment = '\0';
            while (*math_expr == ' ' || *math_expr == '\t') math_expr++;
            int val = eval_math(math_expr, as_state.size, false);
            add_or_update_variable(ins, val);
        }
        return;
    }

    // ---- Instruction encoding -------------------------------------------
    int v1 = 0, v2 = 0, v3 = 0;

    // -- la / call (two-instruction pseudo-ops) ---------------------------
    if (!strcmp(ins, "la") || !strcmp(ins, "call")) {
        bool is_call = !strcmp(ins, "call");
        char *target = is_call ? args[0] : args[1];

        if (!is_call) {
            if (try_parse_reg(args[0], &v1) == 0)
                v1 = eval_math(args[0], as_state.size, false);
        }

        if (write_mode) {
            int addr = find_symbol(target);
            bool needs_reloc = (addr == -1)
                             && output_elf
                             && strchr(target, '.') == NULL
                             && target[0] != '@';
            if (needs_reloc) {
                if (is_call) {
                    // Single R_RISCV_CALL covers the auipc+jalr pair
                    add_relocation(as_state.size, target, R_RISCV_CALL);
                } else {
                    add_relocation(as_state.size,     target, R_RISCV_HI20);
                    add_relocation(as_state.size + 4, target, R_RISCV_LO12_I);
                }
            }

            uint32_t val   = (uint32_t)eval_math(target, as_state.size, is_call);
            uint32_t lower = val & 0xFFF;
            uint32_t upper = (lower & 0x800) ? (val >> 12) + 1 : (val >> 12);

            // call  → auipc ra, upper  +  jalr ra, ra, lower
            // la rd → lui  rd, upper  +  addi rd, rd, lower
            uint32_t bin1 = is_call
                ? encode_U_type(0x17, 1,  upper << 12)         // auipc ra
                : encode_U_type(0x37, v1, upper << 12);         // lui   rd
            uint32_t bin2 = is_call
                ? encode_I_type(0x67, 0x0, (int)lower, 1,  1)  // jalr ra, ra, lo
                : encode_I_type(0x13, 0x0, (int)lower, v1, v1); // addi rd, rd, lo

            memcpy(&as_state.image[as_state.size],     &bin1, 4);
            memcpy(&as_state.image[as_state.size + 4], &bin2, 4);
        }
        as_state.size += 8;
        return;
    }

    // -- li (load immediate — may expand to two instructions) -------------
    if (!strcmp(ins, "li")) {
        if (try_parse_reg(args[0], &v1) == 0)
            v1 = eval_math(args[0], as_state.size, false);
        int val = eval_math(args[1], as_state.size, false);

        if (val < -2048 || val > 2047) {
            if (write_mode) {
                uint32_t u_val = (uint32_t)val;
                uint32_t lower = u_val & 0xFFF;
                uint32_t upper = (lower & 0x800) ? (u_val >> 12) + 1 : (u_val >> 12);
                uint32_t bin_lui  = encode_U_type(0x37, v1, upper << 12);
                uint32_t bin_addi = encode_I_type(0x13, 0x0, (int)lower, v1, v1);
                memcpy(&as_state.image[as_state.size],     &bin_lui,  4);
                memcpy(&as_state.image[as_state.size + 4], &bin_addi, 4);
            }
            as_state.size += 8;
        } else {
            if (write_mode) {
                uint32_t bin = encode_I_type(0x13, 0x0, val, 0, v1);
                memcpy(&as_state.image[as_state.size], &bin, 4);
            }
            as_state.size += 4;
        }
        return;
    }

    // -- j / jal ----------------------------------------------------------
    if (!strcmp(ins, "j") || !strcmp(ins, "jal")) {
        char *target_name = (!strcmp(ins, "jal")) ? args[1] : args[0];
        if (!strcmp(ins, "jal")) {
            if (try_parse_reg(args[0], &v1) == 0)
                v1 = eval_math(args[0], as_state.size, false);
        } else {
            v1 = 0;
        }

        if (write_mode) {
            int addr = find_symbol(target_name);
            if (addr == -1 && output_elf
                    && target_name[0] != '@'
                    && strchr(target_name, '.') == NULL) {
                add_relocation(as_state.size, target_name, R_RISCV_JAL);
            }
        }
        v2 = eval_math(target_name, as_state.size, true);
        uint32_t bin = encode_instruction(ins, v1, v2, v3);
        if (write_mode) memcpy(&as_state.image[as_state.size], &bin, 4);
        as_state.size += 4;
        return;
    }

    // -- Branch instructions ----------------------------------------------
    if (ins[0] == 'b') {
        if (try_parse_reg(args[0], &v1) == 0)
            v1 = eval_math(args[0], as_state.size, false);

        char *target_name;
        if (!strcmp(ins, "beqz") || !strcmp(ins, "bnez")
                || !strcmp(ins, "bgtz") || !strcmp(ins, "blez")) {
            target_name = args[1];
        } else {
            if (try_parse_reg(args[1], &v2) == 0)
                v2 = eval_math(args[1], as_state.size, false);
            target_name = args[2];
        }
        v3 = eval_math(target_name, as_state.size, true);
        uint32_t bin = encode_instruction(ins, v1, v2, v3);
        if (write_mode) memcpy(&as_state.image[as_state.size], &bin, 4);
        as_state.size += 4;
        return;
    }

    // -- General instructions ---------------------------------------------
    if (arg_count > 0) {
        if (try_parse_reg(args[0], &v1) == 0)
            v1 = eval_math(args[0], as_state.size, false);
    }
    if (arg_count > 1) {
        if (strchr(args[1], '(')) {
            // offset(base) addressing
            char *paren   = strchr(args[1], '(');
            *paren        = '\0';
            char *reg_end = strchr(paren + 1, ')');
            if (reg_end) *reg_end = '\0';
            v2 = eval_math(args[1], as_state.size, false);
            if (try_parse_reg(paren + 1, &v3) == 0)
                v3 = eval_math(paren + 1, as_state.size, false);
        } else {
            if (try_parse_reg(args[1], &v2) == 0)
                v2 = eval_math(args[1], as_state.size, false);
        }
    }
    if (arg_count > 2) {
        if (try_parse_reg(args[2], &v3) == 0)
            v3 = eval_math(args[2], as_state.size, false);
    }

    uint32_t bin = encode_instruction(ins, v1, v2, v3);
    if (bin != 0) {
        if (write_mode) memcpy(&as_state.image[as_state.size], &bin, 4);
        as_state.size += 4;
    }
}

// ==========================================
// OUTPUT
// ==========================================
void save_binary(const char *filename) {
    if (!compile) return;
    FILE *f = fopen(filename, "wb");
    if (f) { fwrite(as_state.image, 1, as_state.size, f); fclose(f); }
}

void dump_symbol_table() {
    if (!compile) return;
    for (int i = 0; i < symbol_count; i++) {
        printf("%-30s | 0x%08X | flags=0x%02X\n",
               symbol_table[i].name,
               symbol_table[i].address,
               symbol_table[i].flags);
    }
}

void save_elf(const char *filename) {
    if (!compile) return;
    FILE *f = fopen(filename, "wb");
    if (!f) { panic("Could not open file for writing: %s", filename); return; }

    // Section name string table
    char shstrtab[] = "\0.text\0.shstrtab\0.symtab\0.strtab\0.rela.text\0";
    int  shstrtab_size = sizeof(shstrtab);

    // Build symbol string table and ELF symbol array
    char *strtab = calloc(1, 65536);
    int   strtab_size = 1;     // index 0 is always the null byte

    Elf32_Sym symtab[MAX_SYMBOLS + 1];
    memset(symtab, 0, sizeof(symtab));
    int sym_count = 1;         // index 0 is STN_UNDEF

    // We need to track how our internal symbols map to the new sorted ELF indices
    int elf_sym_map[MAX_SYMBOLS];
    memset(elf_sym_map, 0, sizeof(elf_sym_map));

    // --- ELF RULE: All LOCAL symbols must come first ---
    for (int i = 0; i < symbol_count; i++) {
        if (!(symbol_table[i].flags & (SYM_FLAG_GLOBAL | SYM_FLAG_EXTERN))) {
            symtab[sym_count].st_name = strtab_size;
            strcpy(&strtab[strtab_size], symbol_table[i].name);
            strtab_size += strlen(symbol_table[i].name) + 1;

            if (symbol_table[i].address == 0xFFFFFFFF) {
                symtab[sym_count].st_shndx = SHN_UNDEF;
                symtab[sym_count].st_value = 0;
            } else {
                symtab[sym_count].st_value = symbol_table[i].address - as_state.origin;
                symtab[sym_count].st_shndx = 1; // .text
            }
            symtab[sym_count].st_info = ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE);
            elf_sym_map[i] = sym_count;
            sym_count++;
        }
    }

    // Record the index where GLOBAL symbols begin!
    int local_sym_count = sym_count; 

    // --- Write GLOBAL and EXTERN symbols ---
    for (int i = 0; i < symbol_count; i++) {
        if (symbol_table[i].flags & (SYM_FLAG_GLOBAL | SYM_FLAG_EXTERN)) {
            symtab[sym_count].st_name = strtab_size;
            strcpy(&strtab[strtab_size], symbol_table[i].name);
            strtab_size += strlen(symbol_table[i].name) + 1;

            if (symbol_table[i].address == 0xFFFFFFFF) {
                symtab[sym_count].st_shndx = SHN_UNDEF;
                symtab[sym_count].st_value = 0;
            } else {
                symtab[sym_count].st_value = symbol_table[i].address - as_state.origin;
                symtab[sym_count].st_shndx = 1; // .text
            }
            symtab[sym_count].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE);
            elf_sym_map[i] = sym_count;
            sym_count++;
        }
    }

    // Re-map relocations to use the new sorted symbol indices
    Elf32_Rela patched_relocs[MAX_RELOCS];
    for (int i = 0; i < relocation_count; i++) {
        patched_relocs[i] = relocation_table[i];
        int old_idx = ELF32_R_SYM(relocation_table[i].r_info) - 1;
        int new_idx = elf_sym_map[old_idx];
        int type    = ELF32_R_TYPE(relocation_table[i].r_info);
        patched_relocs[i].r_info = ELF32_R_INFO(new_idx, type);
    }

    // Calculate section offsets in the file
    int num_sections = 6;
    uint32_t off_shdr    = sizeof(Elf32_Ehdr);
    uint32_t off_text    = off_shdr    + (num_sections * sizeof(Elf32_Shdr));
    uint32_t off_shstrtab = off_text   + as_state.size;
    uint32_t off_strtab  = off_shstrtab + shstrtab_size;
    uint32_t off_symtab  = off_strtab  + strtab_size;
    uint32_t off_rela    = off_symtab  + (sym_count * sizeof(Elf32_Sym));
    int      rela_size   = relocation_count * sizeof(Elf32_Rela);

    // ELF header
    Elf32_Ehdr ehdr; memset(&ehdr, 0, sizeof(Elf32_Ehdr));
    memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
    ehdr.e_ident[EI_CLASS]   = ELFCLASS32;
    ehdr.e_ident[EI_DATA]    = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_type      = ET_REL;
    ehdr.e_machine   = EM_RISCV;
    ehdr.e_version   = EV_CURRENT;
    ehdr.e_ehsize    = sizeof(Elf32_Ehdr);
    ehdr.e_shentsize = sizeof(Elf32_Shdr);
    ehdr.e_shnum     = num_sections;
    ehdr.e_shoff     = off_shdr;
    ehdr.e_shstrndx  = 2;

    // Section headers
    Elf32_Shdr shdr[6]; memset(shdr, 0, sizeof(shdr));

    shdr[1].sh_name      = 1;
    shdr[1].sh_type      = SHT_PROGBITS;
    shdr[1].sh_flags     = SHF_ALLOC | SHF_EXECINSTR;
    shdr[1].sh_addr      = as_state.origin;
    shdr[1].sh_offset    = off_text;
    shdr[1].sh_size      = as_state.size;
    shdr[1].sh_addralign = 4;
    
    shdr[2].sh_name      = 7;
    shdr[2].sh_type      = SHT_STRTAB;
    shdr[2].sh_offset    = off_shstrtab;
    shdr[2].sh_size      = shstrtab_size;
    shdr[2].sh_addralign = 1;
    
    shdr[3].sh_name      = 17;
    shdr[3].sh_type      = SHT_SYMTAB;
    shdr[3].sh_offset    = off_symtab;
    shdr[3].sh_size      = sym_count * sizeof(Elf32_Sym);
    shdr[3].sh_link      = 4;                 
    shdr[3].sh_info      = local_sym_count;  // <--- THE FIX! First Global Index!
    shdr[3].sh_entsize   = sizeof(Elf32_Sym);
    shdr[3].sh_addralign = 4;
    
    shdr[4].sh_name      = 25;
    shdr[4].sh_type      = SHT_STRTAB;
    shdr[4].sh_offset    = off_strtab;
    shdr[4].sh_size      = strtab_size;
    shdr[4].sh_addralign = 1;
    
    shdr[5].sh_name      = 33;
    shdr[5].sh_type      = SHT_RELA;
    shdr[5].sh_offset    = off_rela;
    shdr[5].sh_size      = rela_size;
    shdr[5].sh_link      = 3;   
    shdr[5].sh_info      = 1;   
    shdr[5].sh_entsize   = sizeof(Elf32_Rela);
    shdr[5].sh_addralign = 4;

    // Write file
    fwrite(&ehdr,           1, sizeof(Elf32_Ehdr),            f);
    fwrite(shdr,            1, sizeof(shdr),                  f);
    fwrite(as_state.image,  1, as_state.size,                 f);
    fwrite(shstrtab,        1, shstrtab_size,                 f);
    fwrite(strtab,          1, strtab_size,                   f);
    fwrite(symtab,          1, sym_count * sizeof(Elf32_Sym), f);
    
    // Write patched relocations!
    fwrite(patched_relocs,  1, rela_size,                     f);

    free(strtab);
    fclose(f);
}
