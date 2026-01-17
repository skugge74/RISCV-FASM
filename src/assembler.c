#include "assembler.h"
#include "preprocessor.h"

Symbol symbol_table[MAX_SYMBOLS];
int symbol_count = 0;
char current_parent[64] = "global";

void dump_symbol_table() {
    printf("\n--- SYMBOL TABLE DUMP ---\n");
    printf("%-20s | %-10s\n", "Symbol Name", "Address");
    printf("-------------------------------------\n");
    for (int i = 0; i < symbol_count; i++) {
        printf("%-20s | 0x%08x\n", symbol_table[i].name, symbol_table[i].address);
    }
    printf("-------------------------------------\n");
}
int find_macro(const char *name) {
    for (int i = 0; i < macro_lib_count; i++) {
        if (strcmp(name, macro_lib[i].name) == 0) return i;
    }
    return -1;
}

void add_symbol(const char* name, uint32_t addr) {
    if (symbol_count >= MAX_SYMBOLS) return;
    char full_name[128];

    bool is_macro_label = (name[0] == '.' && (strchr(name, '_') != NULL || isdigit(name[1])));

    if (is_macro_label) {
        strncpy(full_name, name, 127);
    } 
    else if (name[0] == '.') {
        snprintf(full_name, sizeof(full_name), "%s%s", current_parent, name);
    } else {
        strncpy(current_parent, name, 63);
        strncpy(full_name, name, 127);
    }
    
    strcpy(symbol_table[symbol_count].name, full_name);
    symbol_table[symbol_count].address = addr;
    symbol_count++;
}

int find_symbol(const char *name) {
    bool is_macro = (name[0] == '.' && (strchr(name, '_') != NULL || isdigit(name[1])));

    if (is_macro) {
        for (int i = 0; i < symbol_count; i++) {
            if (strcmp(symbol_table[i].name, name) == 0) return symbol_table[i].address;
        }
        return -1; 
    }

    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) return symbol_table[i].address;
    }

    if (name[0] == '.') {
        char full[128];
        snprintf(full, 128, "%s%s", current_parent, name);
        for (int i = 0; i < symbol_count; i++) {
            if (strcmp(symbol_table[i].name, full) == 0) return symbol_table[i].address;
        }
    }

    return -1; 
}

int resolve_val(const char* arg, uint32_t current_pc) {
    int val;
    if (parse_arg(arg, &val)) {
        return val; 
    }

    int target_addr = find_symbol(arg);
    if (target_addr != -1) {
        return (int32_t)target_addr - (int32_t)current_pc;
    }

    return 0; 
}

uint32_t encode_R_type(int op, int f3, int f7, int rs1, int rs2, int rd) {
    return (op  & 0x7F) | 
           ((rd  & 0x1F) << 7) | 
           ((f3  & 0x7)  << 12) | 
           ((rs1 & 0x1F) << 15) | 
           ((rs2 & 0x1F) << 20) | 
           ((f7  & 0x7F) << 25);
}

uint32_t encode_I_type(int op, int f3, int imm, int rs1, int rd) {
    //  prevent sign extension.
    uint32_t u_op   = (uint32_t)op  & 0x7F;
    uint32_t u_rd   = (uint32_t)rd  & 0x1F;
    uint32_t u_f3   = (uint32_t)f3  & 0x7;
    uint32_t u_rs1  = (uint32_t)rs1 & 0x1F;
    uint32_t u_imm  = (uint32_t)imm & 0xFFF; 

    return u_op | (u_rd << 7) | (u_f3 << 12) | (u_rs1 << 15) | (u_imm << 20);
}

uint32_t encode_S_type(int op, int f3, int rs2, int imm, int rs1) {
    // imm[11:0] is split into [11:5] and [4:0]
    uint32_t imm_4_0  = (imm & 0x1F);       // Lower 5 bits
    uint32_t imm_11_5 = (imm >> 5) & 0x7F;  // Upper 7 bits
    
    return (op  & 0x7F) | 
           (imm_4_0 << 7) | 
           ((f3  & 0x7)  << 12) | 
           ((rs1 & 0x1F) << 15) | 
           ((rs2 & 0x1F) << 20) | 
           (imm_11_5 << 25);
}

uint32_t encode_B_type(int op, int f3, int rs1, int rs2, int imm) {
    // RISC-V B-type offsets must be even. Hardware ignores bit 0.
    // imm[12|11|10:5|4:1]
    uint32_t b1_4  = (imm >> 1)  & 0xF;    // bits 1,2,3,4
    uint32_t b5_10 = (imm >> 5)  & 0x3F;   // bits 5,6,7,8,9,10
    uint32_t b11   = (imm >> 11) & 0x1;    // bit 11
    uint32_t b12   = (imm >> 12) & 0x1;    // bit 12 (sign bit)

    return (op  & 0x7F) | 
           (b11 << 7) | 
           (b1_4 << 8) | 
           ((f3  & 0x7)  << 12) | 
           ((rs1 & 0x1F) << 15) | 
           ((rs2 & 0x1F) << 20) | 
           (b5_10 << 25) | 
           (b12 << 31);
}

uint32_t encode_J_type(int op, int rd, int imm) {
    // imm[20|10:1|11|19:12]
    uint32_t b12_19 = (imm >> 12) & 0xFF;   // bits 12-19
    uint32_t b11    = (imm >> 11) & 0x1;    // bit 11
    uint32_t b1_10  = (imm >> 1)  & 0x3FF;  // bits 1-10
    uint32_t b20    = (imm >> 20) & 0x1;    // bit 20 (sign bit)

    return (op  & 0x7F) | 
           ((rd & 0x1F) << 7) | 
           (b12_19 << 12) | 
           (b11 << 20) | 
           (b1_10 << 21) | 
           (b20 << 31);
}

uint32_t encode_U_type(int op, int rd, int imm) {
    return (op & 0x7F) | ((rd & 0x1F) << 7) | (imm & 0xFFFFF000);
}

uint32_t encode_instruction(char* name, int a1, int a2, int a3) {
    // --- R-Type: Register-Register Operations ---
    if (!strcmp(name, "add"))  return encode_R_type(0x33, 0x0, 0x00, a2, a3, a1);
    if (!strcmp(name, "sub"))  return encode_R_type(0x33, 0x0, 0x20, a2, a3, a1);
    if (!strcmp(name, "sll"))  return encode_R_type(0x33, 0x1, 0x00, a2, a3, a1);
    if (!strcmp(name, "slt"))  return encode_R_type(0x33, 0x2, 0x00, a2, a3, a1);
    if (!strcmp(name, "sltu")) return encode_R_type(0x33, 0x3, 0x00, a2, a3, a1);
    if (!strcmp(name, "xor"))  return encode_R_type(0x33, 0x4, 0x00, a2, a3, a1);
    if (!strcmp(name, "srl"))  return encode_R_type(0x33, 0x5, 0x00, a2, a3, a1);
    if (!strcmp(name, "sra"))  return encode_R_type(0x33, 0x5, 0x20, a2, a3, a1);
    if (!strcmp(name, "or"))   return encode_R_type(0x33, 0x6, 0x00, a2, a3, a1);
    if (!strcmp(name, "and"))  return encode_R_type(0x33, 0x7, 0x00, a2, a3, a1);

    // --- I-Type: Immediate Operations ---
    if (!strcmp(name, "addi"))  return encode_I_type(0x13, 0x0, a3, a2, a1);
    if (!strcmp(name, "slti"))  return encode_I_type(0x13, 0x2, a3, a2, a1);
    if (!strcmp(name, "sltiu")) return encode_I_type(0x13, 0x3, a3, a2, a1);
    if (!strcmp(name, "xori"))  return encode_I_type(0x13, 0x4, a3, a2, a1);
    if (!strcmp(name, "ori"))   return encode_I_type(0x13, 0x6, a3, a2, a1);
    if (!strcmp(name, "andi"))  return encode_I_type(0x13, 0x7, a3, a2, a1);
    if (!strcmp(name, "slli"))  return encode_I_type(0x13, 0x1, a3 & 0x1F, a2, a1);
    if (!strcmp(name, "srli"))  return encode_I_type(0x13, 0x5, a3 & 0x1F, a2, a1);
    if (!strcmp(name, "srai"))  return encode_I_type(0x13, 0x5, (a3 & 0x1F) | 0x400, a2, a1);

    // --- I-Type: Load Operations ---
    if (!strcmp(name, "lb"))  return encode_I_type(0x03, 0x0, a2, a3, a1); // Note: a2=imm, a3=rs1
    if (!strcmp(name, "lh"))  return encode_I_type(0x03, 0x1, a2, a3, a1);
    if (!strcmp(name, "lw"))  return encode_I_type(0x03, 0x2, a2, a3, a1);
    if (!strcmp(name, "lbu")) return encode_I_type(0x03, 0x4, a2, a3, a1);
    if (!strcmp(name, "lhu")) return encode_I_type(0x03, 0x5, a2, a3, a1);

    // --- S-Type: Store Operations ---
    if (!strcmp(name, "sb")) return encode_S_type(0x23, 0x0, a1, a2, a3); // rs2, imm, rs1
    if (!strcmp(name, "sh")) return encode_S_type(0x23, 0x1, a1, a2, a3);
    if (!strcmp(name, "sw")) return encode_S_type(0x23, 0x2, a1, a2, a3);

    // --- B-Type: Conditional Branches ---
    if (!strcmp(name, "beq"))  return encode_B_type(0x63, 0x0, a1, a2, a3);
    if (!strcmp(name, "bne"))  return encode_B_type(0x63, 0x1, a1, a2, a3);
    if (!strcmp(name, "blt"))  return encode_B_type(0x63, 0x4, a1, a2, a3);
    if (!strcmp(name, "bge"))  return encode_B_type(0x63, 0x5, a1, a2, a3);
    if (!strcmp(name, "bltu")) return encode_B_type(0x63, 0x6, a1, a2, a3);
    if (!strcmp(name, "bgeu")) return encode_B_type(0x63, 0x7, a1, a2, a3);

    // --- J-Type & I-Type (Jumps) ---
    if (!strcmp(name, "jal"))  return encode_J_type(0x6F, a1, a2);
    if (!strcmp(name, "jalr")) return encode_I_type(0x67, 0x0, a3, a2, a1);

    // --- U-Type: Upper Immediates ---
    if (!strcmp(name, "lui"))   return encode_U_type(0x37, a1, a2);
    if (!strcmp(name, "auipc")) return encode_U_type(0x17, a1, a2);

    // --- Common Pseudo-instructions (The "WOW" CV details) ---
    if (!strcmp(name, "li"))   return encode_I_type(0x13, 0x0, a2, 0, a1);  // addi rd, x0, imm
    if (!strcmp(name, "mv"))   return encode_I_type(0x13, 0x0, 0, a2, a1);  // addi rd, rs, 0
    if (!strcmp(name, "j"))    return encode_J_type(0x6F, 0, a1);           // jal x0, label
    if (!strcmp(name, "ret"))  return encode_I_type(0x67, 0x0, 0, 1, 0);    // jalr x0, x1, 0
    if (!strcmp(name, "nop"))  return encode_I_type(0x13, 0x0, 0, 0, 0);    // addi x0, x0, 0

    return 0; // Unknown instruction
}


bool parse_arg(const char *arg, int *out_val) {
    if (!arg || *arg == '\0') return false;
    while (isspace((unsigned char)*arg)) arg++;
    if (*arg == '\0') return false;

    if (*arg == 'x') { *out_val = atoi(arg + 1); return true; }
    if (!strncmp(arg, "zero", 4)) { *out_val = 0; return true; }
    if (!strncmp(arg, "ra", 2))   { *out_val = 1; return true; }
    if (!strncmp(arg, "sp", 2))   { *out_val = 2; return true; }
    
    if (*arg == 't') {
        int n = atoi(arg+1);
        *out_val = (n < 3) ? (n + 5) : (n + 25 - 3);
        return true;
    }

    if (isdigit(*arg) || (*arg == '-' && isdigit(*(arg + 1)))) {
        char *endptr;
        *out_val = (int)strtol(arg, &endptr, 0);
        return true;
    }

    return false; // It's a label
}

void simplify_punct(char *str) {
    char temp[MAX_LINE_LEN];
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == ',' || str[i] == '(' || str[i] == ')') {
            temp[j++] = ' '; 
        } 
        else {
            temp[j++] = str[i];
        }
    }
    temp[j] = '\0';
    strcpy(str, temp);
}

bool split_line(const char *str, char *ins, char *arg1, char *arg2, char *arg3, char *arg4) {
    char clean_line[MAX_LINE_LEN];
    strncpy(clean_line, str, MAX_LINE_LEN);

    for (int i = 0; i < MAX_LINE_LEN && clean_line[i] != '\0'; i++) {
        if (clean_line[i] == ';' || clean_line[i] == '#') {
            clean_line[i] = '\0';
            break;
        }
    }

    *ins = *arg1 = *arg2 = *arg3 = *arg4 = '\0';
    
    char *ptr = clean_line;
    while (isspace(*ptr)) ptr++;
    
    if (*ptr == '\0') return false;

    int found = sscanf(ptr, "%s %s %s %s %s", ins, arg1, arg2, arg3, arg4);
    return (found >= 1);
}

void process_pass(FILE *fp, FILE *fp_out, bool write_mode) {
    char line[MAX_LINE_LEN];
    uint32_t pc = 0;
    
    unique_id_counter = 0; 
    stack_ptr = -1; 
    defining_macro = false; 
    strcpy(current_parent, "global");
    rewind(fp);

    if (write_mode) {
        printf("\n--- PASS 2: STARTING BINARY GENERATION ---\n");
    }

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        char ins[32], a1[32], a2[32], a3[32], a4[32];
        char split_tmp[MAX_LINE_LEN];
        strcpy(split_tmp, line);
        simplify_punct(split_tmp);

        if (!split_line(split_tmp, ins, a1, a2, a3, a4)) continue;

        if (!strcmp(ins, "macro")) {
            defining_macro = true;
            strcpy(macro_lib[macro_lib_count].name, a1);
            macro_lib[macro_lib_count].line_count = 0;
            continue;
        }
        if (!strcmp(ins, "endm")) {
            defining_macro = false;
            macro_lib_count++;
            continue;
        }
        if (defining_macro) {
            strcpy(macro_lib[macro_lib_count].lines[macro_lib[macro_lib_count].line_count++], line);
            continue;
        }

        // Macro Expansion
        int macro_idx = find_macro(ins);
        if (macro_idx != -1) {
            int current_id;
            if (strncmp(ins, "end", 3) == 0)      current_id = pop_id();
            else if (strncmp(ins, "else", 4) == 0) current_id = peek_id();
            else {
                unique_id_counter++;
                current_id = unique_id_counter;
                push_id(current_id);
            }

            for (int j = 0; j < macro_lib[macro_idx].line_count; j++) {
                char expanded[MAX_LINE_LEN];
                strcpy(expanded, macro_lib[macro_idx].lines[j]);
                substitute_args_with_id(expanded, a1, a2, a3, current_id); 
                process_instruction(expanded, &pc, fp_out, write_mode);
            }
            continue;
        }

        process_instruction(line, &pc, fp_out, write_mode);
    }

    if (!write_mode) {
        dump_symbol_table();
    }
}

void process_instruction(char *line, uint32_t *pc, FILE *fp_out, bool write_mode) {
    char work_line[MAX_LINE_LEN];
    strcpy(work_line, line);
    simplify_punct(work_line);

    char ins[32], a1[32], a2[32], a3[32], a4[32];
    if (!split_line(work_line, ins, a1, a2, a3, a4)) return;

    if (ins[strlen(ins) - 1] == ':') {
        ins[strlen(ins) - 1] = '\0';
        
        if (!write_mode) {
            add_symbol(ins, *pc); 
        } else {
       
            if (ins[0] != '.') strcpy(current_parent, ins);
        }
        
        
        if (strlen(a1) == 0) return; 

      
        strcpy(ins, a1); strcpy(a1, a2); strcpy(a2, a3); strcpy(a3, a4); 
    }

    // --- Binary Generation (Pass 2 only) ---
    if (write_mode) {
        

        int v1 = resolve_val(a1, *pc);
        int v2 = resolve_val(a2, *pc);
        int v3 = resolve_val(a3, *pc);
        

        // DEBUG PRINT: See exactly what the assembler is doing for jumps/branches
        bool is_branch = (ins[0] == 'b' && strcmp(ins, "beq") >= 0);
        bool is_jal    = !strcmp(ins, "jal");
        bool is_j      = !strcmp(ins, "j");
        if (is_branch || is_jal || is_j) {
            const char* target_label = is_j ? a1 : (is_jal ? a2 : a3);
            int final_val = is_j ? v1 : (is_jal ? v2 : v3);
            printf("  [P2 DEBUG] PC 0x%04X: %s to label '%s' -> Offset value: %d\n", 
                    *pc, ins, target_label, final_val);
        }

        uint32_t bin = encode_instruction(ins, v1, v2, v3);
        if (bin != 0) fwrite(&bin, 4, 1, fp_out);
    }

    *pc += 4;
}
