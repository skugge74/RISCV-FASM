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
// SECTION MEMORY HELPERS
// ==========================================
uint32_t get_current_offset() {
    if (as_state.current_section == SECTION_TEXT) return as_state.text_size;
    if (as_state.current_section == SECTION_DATA) return as_state.data_size;
    if (as_state.current_section == SECTION_BSS)  return as_state.bss_size;
    return 0;
}

uint32_t get_virtual_address() {
    // FIXED: Absolutely NO ELF checks here. 
    // Always use Absolute Addresses internally so math and Flat Binaries work seamlessly.
    // The subtraction to Section-Relative happens safely in save_elf().
    uint32_t base = as_state.origin;
    if (as_state.current_section == SECTION_TEXT) return base + as_state.text_size;
    if (as_state.current_section == SECTION_DATA) return base + as_state.final_text_size + as_state.data_size;
    if (as_state.current_section == SECTION_BSS)  return base + as_state.final_text_size + as_state.final_data_size + as_state.bss_size;
    return base;
}

void emit_bytes(const void* src, uint32_t len, bool write_mode) {
    if (!write_mode) {
        if (as_state.current_section == SECTION_TEXT) as_state.text_size += len;
        else if (as_state.current_section == SECTION_DATA) as_state.data_size += len;
        else if (as_state.current_section == SECTION_BSS)  as_state.bss_size += len;
        return;
    }
    
    if (as_state.current_section == SECTION_TEXT) {
        if (src) memcpy(&as_state.text[as_state.text_size], src, len);
        else memset(&as_state.text[as_state.text_size], 0, len);
        as_state.text_size += len;
    } else if (as_state.current_section == SECTION_DATA) {
        if (src) memcpy(&as_state.data[as_state.data_size], src, len);
        else memset(&as_state.data[as_state.data_size], 0, len);
        as_state.data_size += len;
    }
}

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

void rejoin_split_expressions(char args[MAX_ARGS][128], int *arg_count) {
    for (int i = 0; i < *arg_count - 1; i++) {
        char *current = args[i];
        char *next = args[i + 1];
        if (strlen(current) == 0 || strlen(next) == 0) continue;

        if (current[0] == '\'' || current[0] == '"') continue;
        if (next[0]    == '\'' || next[0]    == '"') continue; 

        bool c_is_op = (!strcmp(current, "+") || !strcmp(current, "-") || !strcmp(current, "*") || !strcmp(current, "/") || !strcmp(current, "<<") || !strcmp(current, ">>") || !strcmp(current, "&") || !strcmp(current, "|") || !strcmp(current, "^"));
        bool n_is_op = (!strcmp(next, "+") || !strcmp(next, "-") || !strcmp(next, "*") || !strcmp(next, "/") || !strcmp(next, "<<") || !strcmp(next, ">>") || !strcmp(next, "&") || !strcmp(next, "|") || !strcmp(next, "^"));
        
        char last = current[strlen(current) - 1];
        bool c_ends_op = (last == '+' || last == '-' || last == '*' || last == '/' || last == '<' || last == '>' || last == '&' || last == '|' || last == '^');

        if (c_is_op || n_is_op || c_ends_op) {
            strncat(current, next, 127 - strlen(current));
            for (int j = i + 1; j < *arg_count - 1; j++) {
                strcpy(args[j], args[j + 1]);
            }
            (*arg_count)--;
            i--; 
        }
    }
}

void simplify_punct(char *str) {
    char buffer[MAX_LINE_LEN * 2];
    int j = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '=') { buffer[j++] = ' '; buffer[j++] = '='; buffer[j++] = ' '; }
        else               buffer[j++] = str[i];
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
        bool in_str = false;
        int parens = 0;

        while (*str) {
            if (*str == '"' && !in_str) { in_str = true; args[*arg_count][idx++] = *str++; continue; }
            if (*str == '"' &&  in_str) { in_str = false; args[*arg_count][idx++] = *str++; continue; }
            if (in_str) { if (idx < 127) args[*arg_count][idx++] = *str++; continue; }

            if (*str == '\'') {
                if (idx < 127) args[*arg_count][idx++] = *str++; 
                if (*str == '\\' && idx < 127) args[*arg_count][idx++] = *str++; 
                if (*str && idx < 127) args[*arg_count][idx++] = *str++; 
                if (*str == '\'' && idx < 127) args[*arg_count][idx++] = *str++; 
                continue;
            }

            if (*str == '(') parens++;
            if (*str == ')') parens--;
            if (parens == 0 && (isspace((unsigned char)*str) || *str == ',')) break;

            if (idx < 127) args[*arg_count][idx++] = *str;
            str++;
        }
        args[*arg_count][idx] = '\0';
        if (idx > 0) (*arg_count)++;
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
    as_state.text_size = 0;
    as_state.data_size = 0;
    as_state.bss_size = 0;
    as_state.current_section = SECTION_TEXT;
    anon_count = 0;
    unique_id_counter = 0;
}

void add_relocation(uint32_t offset, const char *name, int type, int32_t addend) {
    if (relocation_count >= MAX_RELOCS) return;
    int sym_idx = -1;
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) { sym_idx = i + 1; break; }
    }
    if (sym_idx == -1) { add_symbol(name, 0xFFFFFFFF); sym_idx = symbol_count; }

    relocation_table[relocation_count].r_offset = offset;
    relocation_table[relocation_count].r_info   = ELF32_R_INFO(sym_idx, type);
    relocation_table[relocation_count].r_addend = addend;
    relocation_count++;
}

void add_symbol(const char *name, uint32_t raw_offset) {
    if (symbol_count >= MAX_SYMBOLS) return;
    if (strcmp(name, "@@") == 0) {
        if (anon_count < MAX_SYMBOLS) anon_labels[anon_count++] = get_virtual_address();
        return;
    }
    char full_name[128];
    if (name[0] == '.') snprintf(full_name, 128, "%s%s", current_parent, name);
    else {
        if (raw_offset != 0xFFFFFFFF) strncpy(current_parent, name, 63);
        strcpy(full_name, name);
    }
    uint8_t auto_flags = 0;
    if (strcmp(full_name, "_start") == 0) auto_flags |= SYM_FLAG_GLOBAL;
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, full_name) == 0) {
            if (raw_offset != 0xFFFFFFFF) {
                symbol_table[i].address = get_virtual_address();
                symbol_table[i].section = as_state.current_section;
            }
            symbol_table[i].flags |= auto_flags;
            return;
        }
    }
    strcpy(symbol_table[symbol_count].name, full_name);
    symbol_table[symbol_count].address = (raw_offset == 0xFFFFFFFF) ? 0xFFFFFFFF : get_virtual_address();
    symbol_table[symbol_count].flags   = auto_flags;
    symbol_table[symbol_count].section = (raw_offset == 0xFFFFFFFF) ? SECTION_ABS : as_state.current_section;
    symbol_count++;
}

int find_symbol(const char *name) {
    for (int i = 0; i < symbol_count; i++)
        if (strcmp(symbol_table[i].name, name) == 0) return (int)symbol_table[i].address;
    if (name[0] == '.') {
        char full[128];
        snprintf(full, 128, "%s%s", current_parent, name);
        for (int i = 0; i < symbol_count; i++)
            if (strcmp(symbol_table[i].name, full) == 0) return (int)symbol_table[i].address;
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
    symbol_table[symbol_count].section = SECTION_ABS;
    symbol_count++;
}

bool parse_arg(const char *arg, int *out_val) {
    if (!arg || !*arg) return false;
    char buffer[128];
    strncpy(buffer, arg, 127); buffer[127] = '\0';
    char *ptr = trim_whitespace(buffer);

    if (ptr[0] == '\'') {
        if (ptr[1] == '\\' && ptr[3] == '\'') {
            if (ptr[2] == 'n') { *out_val = '\n'; return true; }
            if (ptr[2] == 't') { *out_val = '\t'; return true; }
            if (ptr[2] == 'r') { *out_val = '\r'; return true; }
            if (ptr[2] == '0') { *out_val = '\0'; return true; }
            if (ptr[2] == '\\') { *out_val = '\\'; return true; }
        } else if (ptr[2] == '\'') {
            *out_val = ptr[1]; return true;
        }
    }

    if (ptr[0] == 'x' && isdigit(ptr[1])) {
        int n = atoi(ptr + 1);
        if (n < 0 || n > 31) return false;
        *out_val = n; return true;
    }
    if (!strcmp(ptr, "zero")) { *out_val = 0; return true; }
    if (!strcmp(ptr, "ra"))   { *out_val = 1; return true; }
    if (!strcmp(ptr, "sp"))   { *out_val = 2; return true; }
    if (!strcmp(ptr, "gp"))   { *out_val = 3; return true; }
    if (!strcmp(ptr, "tp"))   { *out_val = 4; return true; }
    if (!strcmp(ptr, "fp") || !strcmp(ptr, "s0")) { *out_val = 8; return true; }
    if (!strcmp(ptr, "s1"))   { *out_val = 9; return true; }
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

int resolve_val(const char *name, uint32_t current_virtual_address, bool is_relative) {
    if (name[0] == '@') {
        int target = -1;
        if (name[1] == 'f') {
            for (int i = 0; i < pass1_anon_count; i++)
                if (pass1_anon_labels[i] > current_virtual_address) { target = pass1_anon_labels[i]; break; }
        } else {
            for (int i = pass1_anon_count - 1; i >= 0; i--)
                if (pass1_anon_labels[i] < current_virtual_address) { target = pass1_anon_labels[i]; break; }
        }
        if (target != -1) return is_relative ? (int)((int64_t)target - (int64_t)current_virtual_address) : target;
        return 0;
    }
    if (strcmp(name, "$") == 0) return is_relative ? 0 : (int)current_virtual_address;
    if (strcmp(name, "$$") == 0) return is_relative ? (int)(as_state.origin - current_virtual_address) : (int)as_state.origin;
    
    int val;
    if (parse_arg(name, &val)) return val;
    
    // FIXED: Properly resolve Local Labels by prepending the current parent!
    int sym_idx = -1;
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) { sym_idx = i; break; }
    }
    
    if (sym_idx == -1 && name[0] == '.') {
        char full[128]; snprintf(full, 128, "%s%s", current_parent, name);
        for (int i = 0; i < symbol_count; i++) {
            if (strcmp(symbol_table[i].name, full) == 0) { sym_idx = i; break; }
        }
    }

    if (sym_idx != -1 && symbol_table[sym_idx].address != 0xFFFFFFFF) {
        // Only blank out the instruction for Linker Relocation IF the symbol is a Memory Label.
        if (output_elf && name[0] != '.' && name[0] != '@' && symbol_table[sym_idx].section != SECTION_ABS) {
            return 0;
        }
        int addr = symbol_table[sym_idx].address;
        return is_relative ? (addr - (int32_t)current_virtual_address) : addr;
    }

    if (current_pass == 2 && (!output_elf || strchr(name, '.') != NULL)) {
        panic("Undefined symbol: '%s'", name);
    }
    return 0;
}

int eval_math(char *expr, uint32_t current_virtual_address, bool is_relative) {
    char buffer[128]; strncpy(buffer, expr, 127); buffer[127] = '\0';
    char *str = trim_whitespace(buffer); if (!*str) return 0;
    if (str[0] == '\'') {
        return resolve_val(str, current_virtual_address, is_relative);
    }
    int parens = 0, split_idx = -1, lowest_prec = 999;
    for (int i = 0; str[i]; i++) {
        if      (str[i] == '(') parens++;
        else if (str[i] == ')') parens--;
        else if (parens == 0) {
            int prec = 999;
            if      (str[i] == '<' && str[i+1] == '<') prec = 4;
            else if (str[i] == '>' && str[i+1] == '>') prec = 4;
            else if (str[i] == '|')                    prec = 1;
            else if (str[i] == '^')                    prec = 2;
            else if (str[i] == '&')                    prec = 3;
            else if (str[i] == '+' || str[i] == '-')   prec = 5;
            else if (str[i] == '*' || str[i] == '/')   prec = 6;
            if (prec <= lowest_prec && prec < 999) { lowest_prec = prec; split_idx = i; }
        }
    }
    if (split_idx != -1) {
        int  op_len  = (str[split_idx] == '<' || str[split_idx] == '>') ? 2 : 1;
        char op_char = str[split_idx];
        str[split_idx] = '\0';
        int left  = eval_math(str, current_virtual_address, false);
        int right = eval_math(str + split_idx + op_len, current_virtual_address, false);
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
        return is_relative ? (int)(result - (int32_t)current_virtual_address) : result;
    }
    if (str[0] == '(' && str[strlen(str) - 1] == ')') {
        str[strlen(str) - 1] = '\0';
        return eval_math(str + 1, current_virtual_address, is_relative);
    }
    return resolve_val(str, current_virtual_address, is_relative);
}

// ==========================================
// ENCODING
// ==========================================
uint32_t encode_R_type(int op, int f3, int f7, int rs1, int rs2, int rd) { return (op & 0x7F) | ((rd & 0x1F) << 7) | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | ((f7 & 0x7F) << 25); }
uint32_t encode_I_type(int op, int f3, int imm, int rs1, int rd) { return (op & 0x7F) | ((rd & 0x1F) << 7) | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((imm & 0xFFF) << 20); }
uint32_t encode_U_type(int op, int rd, int imm) { return (op & 0x7F) | ((rd & 0x1F) << 7) | (imm & 0xFFFFF000); }
uint32_t encode_S_type(int op, int f3, int rs2, int imm, int rs1) { return (op & 0x7F) | ((imm & 0x1F) << 7) | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | (((imm >> 5) & 0x7F) << 25); }
uint32_t encode_B_type(int op, int f3, int rs1, int rs2, int imm) { uint32_t u = (uint32_t)imm; return (op & 0x7F) | (((u >> 11) & 0x1) << 7) | (((u >> 1) & 0xF) << 8) | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | (((u >> 5) & 0x3F) << 25) | (((u >> 12) & 0x1) << 31); }
uint32_t encode_J_type(int op, int rd, int imm) { uint32_t u = (uint32_t)imm; return (op & 0x7F) | ((rd & 0x1F) << 7) | (((u >> 12) & 0xFF) << 12) | (((u >> 11) & 0x1) << 20) | (((u >> 1) & 0x3FF) << 21) | (((u >> 20) & 0x1) << 31); }

uint32_t encode_instruction(char *name, int a1, int a2, int a3) {
    if (!strcmp(name, "add"))   return encode_R_type(0x33, 0x0, 0x00, a2, a3, a1);
    if (!strcmp(name, "sub"))   return encode_R_type(0x33, 0x0, 0x20, a2, a3, a1);
    if (!strcmp(name, "neg"))   return encode_R_type(0x33, 0x0, 0x20, 0, a2, a1);
    if (!strcmp(name, "sll"))   return encode_R_type(0x33, 0x1, 0x00, a2, a3, a1);
    if (!strcmp(name, "slt"))   return encode_R_type(0x33, 0x2, 0x00, a2, a3, a1);
    if (!strcmp(name, "sltu"))  return encode_R_type(0x33, 0x3, 0x00, a2, a3, a1);
    if (!strcmp(name, "xor"))   return encode_R_type(0x33, 0x4, 0x00, a2, a3, a1);
    if (!strcmp(name, "srl"))   return encode_R_type(0x33, 0x5, 0x00, a2, a3, a1);
    if (!strcmp(name, "sra"))   return encode_R_type(0x33, 0x5, 0x20, a2, a3, a1);
    if (!strcmp(name, "or"))    return encode_R_type(0x33, 0x6, 0x00, a2, a3, a1);
    if (!strcmp(name, "and"))   return encode_R_type(0x33, 0x7, 0x00, a2, a3, a1);
    if (!strcmp(name, "mul"))     return encode_R_type(0x33, 0x0, 0x01, a2, a3, a1);
    if (!strcmp(name, "div"))     return encode_R_type(0x33, 0x4, 0x01, a2, a3, a1);
    if (!strcmp(name, "divu"))   return encode_R_type(0x33, 0x5, 0x01, a2, a3, a1);
    if (!strcmp(name, "rem"))     return encode_R_type(0x33, 0x6, 0x01, a2, a3, a1);
    if (!strcmp(name, "remu"))   return encode_R_type(0x33, 0x7, 0x01, a2, a3, a1);
    if (!strcmp(name, "addi"))  return encode_I_type(0x13, 0x0, a3, a2, a1);
    if (!strcmp(name, "slti"))  return encode_I_type(0x13, 0x2, a3, a2, a1);
    if (!strcmp(name, "sltiu")) return encode_I_type(0x13, 0x3, a3, a2, a1);
    if (!strcmp(name, "xori"))  return encode_I_type(0x13, 0x4, a3, a2, a1);
    if (!strcmp(name, "ori"))   return encode_I_type(0x13, 0x6, a3, a2, a1);
    if (!strcmp(name, "andi"))  return encode_I_type(0x13, 0x7, a3, a2, a1);
    if (!strcmp(name, "slli"))  return encode_I_type(0x13, 0x1, a3 & 0x1F, a2, a1);
    if (!strcmp(name, "srli"))  return encode_I_type(0x13, 0x5, a3 & 0x1F, a2, a1);
    if (!strcmp(name, "srai"))  return encode_I_type(0x13, 0x5, (a3 & 0x1F) | 0x400, a2, a1);
    if (!strcmp(name, "lb"))    return encode_I_type(0x03, 0x0, a2, a3, a1);
    if (!strcmp(name, "lh"))    return encode_I_type(0x03, 0x1, a2, a3, a1);
    if (!strcmp(name, "lw"))    return encode_I_type(0x03, 0x2, a2, a3, a1);
    if (!strcmp(name, "lbu"))   return encode_I_type(0x03, 0x4, a2, a3, a1);
    if (!strcmp(name, "lhu"))   return encode_I_type(0x03, 0x5, a2, a3, a1);
    if (!strcmp(name, "csrrw")) return encode_I_type(0x73, 0x1, a2, a3, a1);
    if (!strcmp(name, "csrrs")) return encode_I_type(0x73, 0x2, a2, a3, a1);
    if (!strcmp(name, "csrrc")) return encode_I_type(0x73, 0x3, a2, a3, a1);
    if (!strcmp(name, "csrr"))  return encode_I_type(0x73, 0x2, a2, 0, a1);
    if (!strcmp(name, "csrw"))  return encode_I_type(0x73, 0x1, a1, a2, 0);
    if (!strcmp(name, "csrc"))  return encode_I_type(0x73, 0x3, a1, a2, 0);
    if (!strcmp(name, "csrs"))  return encode_I_type(0x73, 0x2, a1, a2, 0);
    if (!strcmp(name, "sb"))    return encode_S_type(0x23, 0x0, a1, a2, a3);
    if (!strcmp(name, "sh"))    return encode_S_type(0x23, 0x1, a1, a2, a3);
    if (!strcmp(name, "sw"))    return encode_S_type(0x23, 0x2, a1, a2, a3);
    if (!strcmp(name, "beq"))   return encode_B_type(0x63, 0x0, a1, a2, a3);
    if (!strcmp(name, "bne"))   return encode_B_type(0x63, 0x1, a1, a2, a3);
    if (!strcmp(name, "blt"))   return encode_B_type(0x63, 0x4, a1, a2, a3);
    if (!strcmp(name, "bge"))   return encode_B_type(0x63, 0x5, a1, a2, a3);
    if (!strcmp(name, "bltu"))  return encode_B_type(0x63, 0x6, a1, a2, a3);
    if (!strcmp(name, "bgeu"))  return encode_B_type(0x63, 0x7, a1, a2, a3);
    if (!strcmp(name, "beqz"))  return encode_B_type(0x63, 0x0, a1, 0, a3);
    if (!strcmp(name, "bnez"))  return encode_B_type(0x63, 0x1, a1, 0, a3);
    if (!strcmp(name, "bgt"))   return encode_B_type(0x63, 0x4, a2, a1, a3);
    if (!strcmp(name, "bgtz"))  return encode_B_type(0x63, 0x4, 0, a1, a3);
    if (!strcmp(name, "blez"))  return encode_B_type(0x63, 0x5, 0, a1, a3);
    
    // FIXED: Branch pseudo-op logic
    if (!strcmp(name, "bltz"))  return encode_B_type(0x63, 0x4, a1, 0, a3);
    if (!strcmp(name, "bgez"))  return encode_B_type(0x63, 0x5, a1, 0, a3);
    
    if (!strcmp(name, "ble"))   return encode_B_type(0x63, 0x5, a2, a1, a3);
    if (!strcmp(name, "bgtu"))  return encode_B_type(0x63, 0x6, a2, a1, a3);
    if (!strcmp(name, "bleu"))  return encode_B_type(0x63, 0x7, a2, a1, a3);
    if (!strcmp(name, "jal"))   return encode_J_type(0x6F, a1, a2);
    if (!strcmp(name, "jalr"))  return encode_I_type(0x67, 0x0, a3, a2, a1);
    if (!strcmp(name, "lui"))   return encode_U_type(0x37, a1, a2);
    if (!strcmp(name, "auipc")) return encode_U_type(0x17, a1, a2);
    if (!strcmp(name, "ecall"))  return 0x00000073;
    if (!strcmp(name, "ebreak")) return 0x00100073;
    if (!strcmp(name, "mret"))   return 0x30200073;
    if (!strcmp(name, "sret"))   return 0x10200073;
    if (!strcmp(name, "wfi"))    return 0x10500073;
    if (!strcmp(name, "li"))    return encode_I_type(0x13, 0x0, a2, 0, a1);
    if (!strcmp(name, "mv"))    return encode_I_type(0x13, 0x0, 0, a2, a1);
    if (!strcmp(name, "j"))     return encode_J_type(0x6F, 0, a2);
    if (!strcmp(name, "ret"))   return encode_I_type(0x67, 0x0, 0, 1, 0);
    if (!strcmp(name, "nop"))   return encode_I_type(0x13, 0x0, 0, 0, 0);
    return 0;
}

// ==========================================
// DIRECTIVES
// ==========================================
void handle_directive(char *directive, char *args_str, char args[MAX_ARGS][128], int arg_count, bool write_mode) {
    if (!strcmp(directive, ".include")) {
        static int include_depth = 0;
        if (include_depth > 20) { 
            panic("Max include depth exceeded! Circular dependency detected in: %s", args_str); 
            return; 
        }
        char filename[128]; sscanf(args_str, " \"%[^\"]\"", filename);
        FILE *inc_fp = fopen(filename, "r");
        if (inc_fp) {
            include_depth++; 
            const char *saved_file = current_file; 
            int saved_line = current_line;
            char line[MAX_LINE_LEN]; current_file = filename; current_line = 0;
            while (fgets(line, sizeof(line), inc_fp)) {
                current_line++; line[strcspn(line, "\r\n")] = '\0';
                process_line(line, write_mode);
            }
            fclose(inc_fp); 
            current_file = saved_file; 
            current_line = saved_line;
            include_depth--; 
        } else {
            panic("Could not open included file: %s", filename);
        }
        return;
    }
    if (!strcmp(directive, ".global") || !strcmp(directive, ".globl")) {
        if (arg_count < 1) return;
        for (int i = 0; i < symbol_count; i++) {
            if (strcmp(symbol_table[i].name, args[0]) == 0) {
                symbol_table[i].flags |= SYM_FLAG_GLOBAL; return;
            }
        }
        strncpy(symbol_table[symbol_count].name, args[0], 127);
        symbol_table[symbol_count].address = 0xFFFFFFFF;
        symbol_table[symbol_count].flags   = SYM_FLAG_GLOBAL;
        symbol_table[symbol_count].section = SECTION_ABS;
        symbol_count++;
        return;
    }
    if (!strcmp(directive, ".extern")) {
        if (arg_count < 1) return;
        for (int i = 0; i < symbol_count; i++) if (strcmp(symbol_table[i].name, args[0]) == 0) return;
        strncpy(symbol_table[symbol_count].name, args[0], 127);
        symbol_table[symbol_count].address = 0xFFFFFFFF;
        symbol_table[symbol_count].flags   = SYM_FLAG_EXTERN;
        symbol_table[symbol_count].section = SECTION_ABS;
        symbol_count++;
        return;
    }
    if (!strcmp(directive, ".equ") || !strcmp(directive, ".set")) {
        char *comma = strchr(args_str, ',');
        if (comma) add_or_update_variable(args[0], eval_math(comma + 1, get_virtual_address(), false));
        else if (arg_count >= 2) add_or_update_variable(args[0], eval_math(args[1], get_virtual_address(), false));
        return;
    }
    if (!strcmp(directive, ".text")) { as_state.current_section = SECTION_TEXT; return; }
    if (!strcmp(directive, ".data")) { as_state.current_section = SECTION_DATA; return; }
    if (!strcmp(directive, ".bss"))  { as_state.current_section = SECTION_BSS; return; }
    if (!strcmp(directive, ".org")) {
        if (arg_count > 0) as_state.origin = (uint32_t)eval_math(args[0], get_virtual_address(), false);
        return;
    }
    if (!strcmp(directive, ".word")) {
        for (int i = 0; i < arg_count; i++) {
            while (get_virtual_address() % 4 != 0) emit_bytes(NULL, 1, write_mode);
            uint32_t val = (uint32_t)eval_math(args[i], get_virtual_address(), false);
            emit_bytes(&val, 4, write_mode);
        }
        return;
    }
    if (!strcmp(directive, ".half") || !strcmp(directive, ".short")) {
        for (int i = 0; i < arg_count; i++) {
            while (get_virtual_address() % 2 != 0) emit_bytes(NULL, 1, write_mode);
            uint16_t val = (uint16_t)eval_math(args[i], get_virtual_address(), false);
            emit_bytes(&val, 2, write_mode);
        }
        return;
    }
    if (!strcmp(directive, ".byte")) {
        char *ptr = args_str;
        while (*ptr) {
            while (isspace((unsigned char)*ptr)) ptr++;
            if (!*ptr) break;
            char *end = ptr; while (*end && !isspace((unsigned char)*end) && *end != ',') end++;
            char expr[128]; int len = (int)(end - ptr); if (len > 127) len = 127;
            strncpy(expr, ptr, len); expr[len] = '\0';
            uint8_t val = (uint8_t)eval_math(expr, get_virtual_address(), false);
            emit_bytes(&val, 1, write_mode);
            ptr = end; while (*ptr == ',' || isspace((unsigned char)*ptr)) ptr++;
        }
        return;
    }
    if (!strcmp(directive, ".fill")) {
        int repeat = (arg_count > 0) ? eval_math(args[0], get_virtual_address(), false) : 0;
        int size   = (arg_count > 1) ? eval_math(args[1], get_virtual_address(), false) : 1;
        int value  = (arg_count > 2) ? eval_math(args[2], get_virtual_address(), false) : 0;
        for (int i = 0; i < repeat; i++) {
            uint32_t val = value; emit_bytes(&val, size, write_mode);
        }
        return;
    }
    if (!strcmp(directive, ".space")) {
        int num_bytes = (arg_count > 0) ? eval_math(args[0], get_virtual_address(), false) : 0;
        for (int i = 0; i < num_bytes; i++) emit_bytes(NULL, 1, write_mode);
        return;
    }
    if (!strcmp(directive, ".align")) {
        int align_val = (arg_count > 0) ? eval_math(args[0], get_virtual_address(), false) : 4;
        if (align_val <= 0) align_val = 4;
        while (get_virtual_address() % align_val != 0) emit_bytes(NULL, 1, write_mode);
        return;
    }
    if (!strcmp(directive, ".asciz")) {
        char *start = strchr(args_str, '"'), *end = strrchr(args_str, '"');
        if (start && end && start != end) {
            for (char *p = start + 1; p < end; p++) {
                if (*p == '\\' && *(p+1) == 'n') {
                    char nl = '\n'; emit_bytes(&nl, 1, write_mode); p++;
                } else emit_bytes(p, 1, write_mode);
            }
            emit_bytes(NULL, 1, write_mode); 
        }
        return;
    }
}

// ==========================================
// PASS DRIVER
// ==========================================
void process_pass(FILE *fp, bool write_mode, const char *filename) {
    current_pass = write_mode ? 2 : 1;
    current_file = filename; current_line = 0;
    char line[MAX_LINE_LEN]; rewind(fp);
    defining_macro = false; anon_count = 0;
    strcpy(current_parent, "global");
    as_state.text_size = 0;
    as_state.data_size = 0;
    while (fgets(line, sizeof(line), fp)) {
        current_line++; line[strcspn(line, "\r\n")] = '\0';
        process_line(line, write_mode);
    }
    if (!write_mode) {
        memcpy(pass1_anon_labels, anon_labels, sizeof(anon_labels));
        pass1_anon_count = anon_count;
        as_state.final_text_size = as_state.text_size;
        as_state.final_data_size = as_state.data_size;
        for (int i = 0; i < symbol_count; i++) {
            if (symbol_table[i].address == 0xFFFFFFFF || symbol_table[i].section == SECTION_ABS) continue;
            if (symbol_table[i].section == SECTION_DATA) {
                symbol_table[i].address += as_state.final_text_size;
            }
        }
    }
}

// ==========================================
// LINE PROCESSING
// ==========================================
void process_line(char *line, bool write_mode) {
    bool in_string = false;
    for (int i = 0; line[i]; i++) {
        if (line[i] == '"') in_string = !in_string;
        if (!in_string && line[i] == ';') { line[i] = '\0'; break; }
        if (!in_string && line[i] == '#') {
            if (i == 0 || line[i-1] != '%') { line[i] = '\0'; break; }
        }
    }
    char work[MAX_LINE_LEN]; strcpy(work, line); simplify_punct(work);
    
    char ins[64] = {0};
    char args[MAX_ARGS][128] = {0}; 
    int arg_count = 0;

    if (!split_line(work, ins, args, &arg_count)) return;
    rejoin_split_expressions(args, &arg_count);

    if (!strcmp(ins, "macro")) {
        defining_macro = true; strcpy(macro_lib[macro_lib_count].name, args[0]);
        macro_lib[macro_lib_count].line_count = 0; return;
    }
    if (!strcmp(ins, "endm")) { defining_macro = false; macro_lib_count++; return; }
    if (defining_macro) { strcpy(macro_lib[macro_lib_count].lines[macro_lib[macro_lib_count].line_count++], line); return; }

    int m_idx = find_macro(ins);
    if (m_idx != -1) {
        int current_id;
        if (strncmp(ins, "end", 3) == 0) current_id = pop_id(ins + 3);
        else {
            current_id = ++unique_id_counter; char end_search[128];
            snprintf(end_search, sizeof(end_search), "end%s", ins);
            if (find_macro(end_search) != -1) push_id(current_id, ins);
        }
        for (int i = 0; i < macro_lib[m_idx].line_count; i++) {
            char expanded[MAX_LINE_LEN]; strcpy(expanded, macro_lib[m_idx].lines[i]);
            substitute_args_with_id(expanded, args, arg_count, current_id);
            process_line(expanded, write_mode);
        }
        return;
    }
    if (ins[strlen(ins) - 1] == ':') {
        ins[strlen(ins) - 1] = '\0';
        if (current_pass == 1) add_symbol(ins, 0); 
        if (ins[0] != '.') strncpy(current_parent, ins, 63);
        if (arg_count == 0) return;
        strcpy(ins, args[0]); for (int i = 0; i < arg_count - 1; i++) strcpy(args[i], args[i+1]); arg_count--;
    }
    if (ins[0] == '.') {
        char *ptr = strstr(line, ins) + strlen(ins); handle_directive(ins, ptr, args, arg_count, write_mode); return;
    }
    if (arg_count > 0 && strcmp(args[0], "=") == 0) {
        char *eq_pos = strchr(work, '=');
        if (eq_pos) {
            char *math_expr = eq_pos + 1; char *comment = strpbrk(math_expr, ";#"); if (comment) *comment = '\0';
            while (*math_expr == ' ' || *math_expr == '\t') math_expr++;
            add_or_update_variable(ins, eval_math(math_expr, get_virtual_address(), false));
        }
        return;
    }
    int v1 = 0, v2 = 0, v3 = 0;
    if (!strcmp(ins, "la") || !strcmp(ins, "call")) {
        bool is_call = !strcmp(ins, "call");
        char *target = is_call ? args[0] : args[1];

        if (!is_call && try_parse_reg(args[0], &v1) == 0) v1 = eval_math(args[0], get_virtual_address(), false);

        if (write_mode) {
            char base_sym[128]; strcpy(base_sym, target);
            int32_t addend = 0;
            char *math_op = strpbrk(base_sym, "+-");
            if (math_op) {
                addend = eval_math(math_op, get_virtual_address(), false);
                *math_op = '\0'; 
                trim_whitespace(base_sym);
            }

            int sym_idx = -1;
            for (int i = 0; i < symbol_count; i++) {
                if (strcmp(symbol_table[i].name, base_sym) == 0) { sym_idx = i; break; }
            }

            bool needs_reloc = output_elf && base_sym[0] != '@' &&
                               (sym_idx == -1 || 
                               (symbol_table[sym_idx].flags & (SYM_FLAG_GLOBAL | SYM_FLAG_EXTERN)) || 
                               symbol_table[sym_idx].section != SECTION_ABS);

            if (needs_reloc) {
                if (is_call) add_relocation(get_current_offset(), base_sym, R_RISCV_CALL, addend);
                else {
                    add_relocation(get_current_offset(),     base_sym, R_RISCV_HI20, addend);
                    add_relocation(get_current_offset() + 4, base_sym, R_RISCV_LO12_I, addend);
                }
                
                uint32_t b1 = is_call ? encode_U_type(0x17, 1,  0) : encode_U_type(0x37, v1, 0);
                uint32_t b2 = is_call ? encode_I_type(0x67, 0x0, 0, 1, 1) : encode_I_type(0x13, 0x0, 0, v1, v1);
                emit_bytes(&b1, 4, true); emit_bytes(&b2, 4, true);
            } else {
                uint32_t val   = (uint32_t)eval_math(target, get_virtual_address(), is_call);
                uint32_t lower = val & 0xFFF;
                uint32_t upper = (lower & 0x800) ? (val >> 12) + 1 : (val >> 12);
                uint32_t b1 = is_call ? encode_U_type(0x17, 1,  upper << 12) : encode_U_type(0x37, v1, upper << 12);
                uint32_t b2 = is_call ? encode_I_type(0x67, 0x0, (int)lower, 1,  1) : encode_I_type(0x13, 0x0, (int)lower, v1, v1);
                emit_bytes(&b1, 4, true); emit_bytes(&b2, 4, true);
            }
            return;
        }
        emit_bytes(NULL, 8, false); return;
    }

    if (!strcmp(ins, "li")) {
        if (try_parse_reg(args[0], &v1) == 0) v1 = eval_math(args[0], get_virtual_address(), false);
        int val = eval_math(args[1], get_virtual_address(), false);
        if (val < -2048 || val > 2047) {
            if (write_mode) {
                uint32_t u_val = (uint32_t)val;
                uint32_t lower = u_val & 0xFFF;
                uint32_t upper = (lower & 0x800) ? (u_val >> 12) + 1 : (u_val >> 12);
                uint32_t bin_lui  = encode_U_type(0x37, v1, upper << 12);
                uint32_t bin_addi = encode_I_type(0x13, 0x0, (int)lower, v1, v1);
                emit_bytes(&bin_lui, 4, true); emit_bytes(&bin_addi, 4, true);
            } else emit_bytes(NULL, 8, false);
        } else {
            if (write_mode) {
                uint32_t bin = encode_I_type(0x13, 0x0, val, 0, v1); emit_bytes(&bin, 4, true);
            } else emit_bytes(NULL, 4, false);
        }
        return;
    }

    if (!strcmp(ins, "j") || !strcmp(ins, "jal")) {
        char *target_name = (!strcmp(ins, "jal")) ? args[1] : args[0];
        if (!strcmp(ins, "jal") && try_parse_reg(args[0], &v1) == 0) v1 = eval_math(args[0], get_virtual_address(), false);
        else v1 = 0;
        
        if (write_mode) {
            char base_sym[128]; strcpy(base_sym, target_name);
            int32_t addend = 0;
            char *math_op = strpbrk(base_sym, "+-");
            if (math_op) {
                addend = eval_math(math_op, get_virtual_address(), false);
                *math_op = '\0'; trim_whitespace(base_sym);
            }
            if (output_elf && base_sym[0] != '@' && strchr(base_sym, '.') == NULL) {
                add_relocation(get_current_offset(), base_sym, R_RISCV_JAL, addend);
                v2 = 0; // Blank instruction value for linker
            } else {
                v2 = eval_math(target_name, get_virtual_address(), true);
            }
        } else {
            v2 = eval_math(target_name, get_virtual_address(), true);
        }
        uint32_t bin = encode_instruction(ins, v1, v2, v3);
        emit_bytes(&bin, 4, write_mode); return;
    }

    if (ins[0] == 'b') {
        if (try_parse_reg(args[0], &v1) == 0) v1 = eval_math(args[0], get_virtual_address(), false);
        char *target_name;
        if (!strcmp(ins, "beqz") || !strcmp(ins, "bnez") || !strcmp(ins, "bgtz") || !strcmp(ins, "blez") || !strcmp(ins, "bltz") || !strcmp(ins, "bgez")) {
            target_name = args[1];
        } else {
            if (try_parse_reg(args[1], &v2) == 0) v2 = eval_math(args[1], get_virtual_address(), false);
            target_name = args[2];
        }
        v3 = eval_math(target_name, get_virtual_address(), true);
        uint32_t bin = encode_instruction(ins, v1, v2, v3);
        emit_bytes(&bin, 4, write_mode); return;
    }
    
    // Normal Instruction Resolution
    if (arg_count > 0 && try_parse_reg(args[0], &v1) == 0) v1 = eval_math(args[0], get_virtual_address(), false);
    if (arg_count > 1) {
        if (strchr(args[1], '(')) {
            char *paren = strchr(args[1], '('); *paren = '\0';
            char *reg_end = strchr(paren + 1, ')'); if (reg_end) *reg_end = '\0';
            v2 = eval_math(args[1], get_virtual_address(), false);
            if (try_parse_reg(paren + 1, &v3) == 0) v3 = eval_math(paren + 1, get_virtual_address(), false);
        } else if (try_parse_reg(args[1], &v2) == 0) v2 = eval_math(args[1], get_virtual_address(), false);
    }
    if (arg_count > 2 && try_parse_reg(args[2], &v3) == 0) v3 = eval_math(args[2], get_virtual_address(), false);
    
    uint32_t bin = encode_instruction(ins, v1, v2, v3);
if (bin != 0) {
        emit_bytes(&bin, 4, write_mode);
    } else if (write_mode && strlen(ins) > 0) {
        panic("Unknown instruction: '%s'", ins);
    }
}

// ==========================================
// OUTPUT
// ==========================================
void save_binary(const char *filename) {
    if (!compile) return;
    FILE *f = fopen(filename, "wb");
    if (f) { fwrite(as_state.text, 1, as_state.text_size, f); fwrite(as_state.data, 1, as_state.data_size, f); fclose(f); }
}

void dump_symbol_table() {
    if (!compile) return;
    for (int i = 0; i < symbol_count; i++) printf("%-30s | 0x%08X | Sec=%d\n", symbol_table[i].name, symbol_table[i].address, symbol_table[i].section);
}

void save_elf(const char *filename) {
    if (!compile) return;
    FILE *f = fopen(filename, "wb");
    if (!f) return;

    char shstrtab[] = "\0.text\0.data\0.shstrtab\0.symtab\0.strtab\0.rela.text\0";
    int  shstrtab_size = sizeof(shstrtab);
    char *strtab = calloc(1, 65536);
    int strtab_size = 1; 

    Elf32_Sym symtab[MAX_SYMBOLS + 1]; 
    memset(symtab, 0, sizeof(symtab));
    int elf_sym_map[MAX_SYMBOLS]; 
    int sym_idx = 1; 

    // PASS A: Local Symbols (Non-globals)
    for (int i = 0; i < symbol_count; i++) {
        if (!(symbol_table[i].flags & (SYM_FLAG_GLOBAL | SYM_FLAG_EXTERN))) {
            symtab[sym_idx].st_name = strtab_size;
            strcpy(&strtab[strtab_size], symbol_table[i].name);
            strtab_size += strlen(symbol_table[i].name) + 1;

            if (symbol_table[i].address != 0xFFFFFFFF) {
                if (symbol_table[i].section == SECTION_ABS) {
                    symtab[sym_idx].st_value = symbol_table[i].address;
                    symtab[sym_idx].st_shndx = SHN_ABS;
                } else if (symbol_table[i].section == SECTION_DATA) {
                    symtab[sym_idx].st_value = symbol_table[i].address - (as_state.origin + as_state.final_text_size);
                    symtab[sym_idx].st_shndx = 2; 
                } else {
                    symtab[sym_idx].st_value = symbol_table[i].address - as_state.origin;
                    symtab[sym_idx].st_shndx = 1; 
                }
            }
            symtab[sym_idx].st_info = ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE);
            elf_sym_map[i] = sym_idx++;
        }
    }
    int local_sym_count = sym_idx;

    // PASS B: Global Symbols
    for (int i = 0; i < symbol_count; i++) {
        if (symbol_table[i].flags & (SYM_FLAG_GLOBAL | SYM_FLAG_EXTERN)) {
            symtab[sym_idx].st_name = strtab_size;
            strcpy(&strtab[strtab_size], symbol_table[i].name);
            strtab_size += strlen(symbol_table[i].name) + 1;

            if (symbol_table[i].address != 0xFFFFFFFF) {
                if (symbol_table[i].section == SECTION_ABS) {
                    symtab[sym_idx].st_value = symbol_table[i].address;
                    symtab[sym_idx].st_shndx = SHN_ABS;
                } else if (symbol_table[i].section == SECTION_DATA) {
                    symtab[sym_idx].st_value = symbol_table[i].address - (as_state.origin + as_state.final_text_size);
                    symtab[sym_idx].st_shndx = 2;
                } else {
                    symtab[sym_idx].st_value = symbol_table[i].address - as_state.origin;
                    symtab[sym_idx].st_shndx = 1;
                }
            } else {
                symtab[sym_idx].st_shndx = SHN_UNDEF;
            }
            
            int st_type = STT_NOTYPE;
            if (symbol_table[i].section == SECTION_DATA) st_type = STT_OBJECT;
            else if (symbol_table[i].section == SECTION_TEXT) st_type = STT_FUNC;
            
            symtab[sym_idx].st_info = ELF32_ST_INFO(STB_GLOBAL, st_type);
            elf_sym_map[i] = sym_idx++;
        }
    }

    Elf32_Rela patched[MAX_RELOCS];
    for (int i = 0; i < relocation_count; i++) {
        patched[i] = relocation_table[i];
        int old_sym_idx = ELF32_R_SYM(relocation_table[i].r_info) - 1;
        int new_sym_idx = elf_sym_map[old_sym_idx];
        patched[i].r_info = ELF32_R_INFO(new_sym_idx, ELF32_R_TYPE(relocation_table[i].r_info));
    }

    int num_sections = 7;
    uint32_t off_shdr     = sizeof(Elf32_Ehdr);
    uint32_t off_text     = off_shdr     + (num_sections * sizeof(Elf32_Shdr));
    uint32_t off_data     = off_text     + as_state.text_size;
    uint32_t off_shstrtab = off_data     + as_state.data_size;
    uint32_t off_strtab   = off_shstrtab + shstrtab_size;
    uint32_t off_symtab   = off_strtab   + strtab_size;
    uint32_t off_rela     = off_symtab   + (sym_idx * sizeof(Elf32_Sym));

    Elf32_Ehdr ehdr; 
    memset(&ehdr, 0, sizeof(ehdr));
    memcpy(ehdr.e_ident, ELFMAG, 4);
    ehdr.e_ident[4] = 1; ehdr.e_ident[5] = 1; ehdr.e_ident[6] = 1;
    ehdr.e_type = 1; ehdr.e_machine = 0xF3; ehdr.e_version = 1;
    ehdr.e_shoff = off_shdr; ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_shentsize = sizeof(Elf32_Shdr); ehdr.e_shnum = num_sections; ehdr.e_shstrndx = 3;

    Elf32_Shdr shdr[7];
    memset(shdr, 0, sizeof(shdr));
    shdr[1].sh_name = 1; shdr[1].sh_type = 1; shdr[1].sh_flags = 6;
    shdr[1].sh_offset = off_text; shdr[1].sh_size = as_state.text_size; shdr[1].sh_addralign = 4;
    shdr[2].sh_name = 7; shdr[2].sh_type = 1; shdr[2].sh_flags = 3;
    shdr[2].sh_offset = off_data; shdr[2].sh_size = as_state.data_size; shdr[2].sh_addralign = 4;
    shdr[3].sh_name = 13; shdr[3].sh_type = 3; 
    shdr[3].sh_offset = off_shstrtab; shdr[3].sh_size = shstrtab_size;
    shdr[4].sh_name = 23; shdr[4].sh_type = 2; shdr[4].sh_link = 5;
    shdr[4].sh_info = local_sym_count; shdr[4].sh_entsize = sizeof(Elf32_Sym);
    shdr[4].sh_offset = off_symtab; shdr[4].sh_size = sym_idx * sizeof(Elf32_Sym);
    shdr[5].sh_name = 31; shdr[5].sh_type = 3;
    shdr[5].sh_offset = off_strtab; shdr[5].sh_size = strtab_size;
    shdr[6].sh_name = 39; shdr[6].sh_type = 4; shdr[6].sh_link = 4; shdr[6].sh_info = 1;
    shdr[6].sh_entsize = sizeof(Elf32_Rela);
    shdr[6].sh_offset = off_rela; shdr[6].sh_size = relocation_count * sizeof(Elf32_Rela);

    fwrite(&ehdr, 1, sizeof(ehdr), f);
    fwrite(shdr, 1, sizeof(shdr), f);
    fwrite(as_state.text, 1, as_state.text_size, f);
    fwrite(as_state.data, 1, as_state.data_size, f);
    fwrite(shstrtab, 1, shstrtab_size, f);
    fwrite(strtab, 1, strtab_size, f);
    fwrite(symtab, 1, sym_idx * sizeof(Elf32_Sym), f);
    fwrite(patched, 1, relocation_count * sizeof(Elf32_Rela), f);

    free(strtab);
    fclose(f);
}
