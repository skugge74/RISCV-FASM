#include "assembler.h"
#include "preprocessor.h"

Symbol symbol_table[MAX_SYMBOLS];
int symbol_count = 0;
char current_parent[64] = "global";
Assembler as_state;

char* trim_whitespace(char* str) {
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

int eval_math(char *expr, uint32_t current_offset, bool is_relative) {
    char buffer[128];
    strncpy(buffer, expr, 127);
    char *str = trim_whitespace(buffer); 

    if (!*str) return 0;

    int parens = 0;
    int split_idx = -1;
    int lowest_prec = 999; 

    for (int i = 0; str[i]; i++) {
        if (str[i] == '(') parens++;
        else if (str[i] == ')') parens--;
        else if (parens == 0) {
            int prec = 999;
            if (str[i] == '+' || str[i] == '-') prec = 1;
            else if (str[i] == '*' || str[i] == '/') prec = 2;
            
            if (prec <= lowest_prec && prec < 999) {
                lowest_prec = prec;
                split_idx = i;
            }
        }
    }

    if (split_idx != -1) {
        char op = str[split_idx];
        str[split_idx] = '\0'; 
        
        // RECURSION: Always calculate ABSOLUTE value first (pass false)
        int left = eval_math(str, current_offset, false);
        int right = eval_math(str + split_idx + 1, current_offset, false);

        int result = 0;
        switch (op) {
            case '+': result = left + right; break;
            case '-': result = left - right; break;
            case '*': result = left * right; break;
            case '/': result = (right != 0) ? (left / right) : 0; break;
        }

        if (is_relative) {
            return result - (as_state.origin + current_offset);
        }
        return result;
    }

    if (str[0] == '(' && str[strlen(str)-1] == ')') {
        str[strlen(str)-1] = '\0'; 
        return eval_math(str + 1, current_offset, is_relative); 
    }

    return resolve_val(str, current_offset, is_relative);
}

void dump_symbol_table() {
    printf("\n--- SYMBOL TABLE DUMP ---\n");
    printf("%-30s | %-10s\n", "Symbol Name", "Address");
    printf("---------------------------------------------\n");
    for (int i = 0; i < symbol_count; i++) {
        printf("%-30s | 0x%08X\n", symbol_table[i].name, symbol_table[i].address);
    }
    printf("---------------------------------------------\n\n");
}
void init_assembler_total() {
    memset(&as_state, 0, sizeof(Assembler));
    as_state.origin = 0x80000000; // Default for QEMU Virt
    symbol_count = 0;
}

void init_assembler_pass() {
    as_state.size = 0; 
    as_state.current_section = SECTION_TEXT;
}


void add_symbol(const char* name, uint32_t offset) {
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
    symbol_table[symbol_count].address = as_state.origin + offset;
    symbol_count++;
}
int find_symbol(const char *name) {
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

void add_or_update_variable(const char* name, int value) {
    // 1. Search for existing symbol to OVERWRITE
    // We search backwards because usually we modify the most recent variables
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].address = value; // Update the value directly (Raw int)
            return;
        }
    }

    // 2. If not found, create new one
    if (symbol_count >= MAX_SYMBOLS) return;

    // For variables, we usually just use the raw name (Global scope)
    // If you want scoped variables (like .my_var), you can copy your parent-name logic here.
    strncpy(symbol_table[symbol_count].name, name, 127);
    symbol_table[symbol_count].name[127] = '\0';
    
    // Store RAW value (Do NOT add as_state.origin)
    symbol_table[symbol_count].address = value; 
    
    symbol_count++;
}

int resolve_val(const char* name, uint32_t current_offset, bool is_relative) {
    if (strcmp(name, "$") == 0) {
        int current_addr = as_state.origin + current_offset;
        if (is_relative) return 0; // $ - $ = 0
        return current_addr;
    }

    // $$: Section Origin
    if (strcmp(name, "$$") == 0) {
        int section_base = as_state.origin;
        if (is_relative) return section_base - (as_state.origin + current_offset);
        return section_base;
    }
    int val;
    if (parse_arg(name, &val)) return val; 

    int target_addr = find_symbol(name);
    if (target_addr != -1) {
        if (is_relative) {
    uint32_t absolute_pc = as_state.origin + current_offset;
    return (int32_t)target_addr - (int32_t)absolute_pc;
}
        return target_addr; 
    }
    //printf("DEBUG: Symbol '%s' not found. Returning 0.\n", name);
    return 0; 
}



uint32_t encode_R_type(int op, int f3, int f7, int rs1, int rs2, int rd) {
    return (op & 0x7F) | ((rd & 0x1F) << 7) | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | ((f7 & 0x7F) << 25);
}

uint32_t encode_I_type(int op, int f3, int imm, int rs1, int rd) {
    return (op & 0x7F) | ((rd & 0x1F) << 7) | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((imm & 0xFFF) << 20);
}

uint32_t encode_U_type(int op, int rd, int imm) {
    return (op & 0x7F) | ((rd & 0x1F) << 7) | (imm & 0xFFFFF000);
}

uint32_t encode_S_type(int op, int f3, int rs2, int imm, int rs1) {
    return (op & 0x7F) | ((imm & 0x1F) << 7) | ((f3 & 0x7) << 12) | ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | (((imm >> 5) & 0x7F) << 25);
}

uint32_t encode_B_type(int op, int f3, int rs1, int rs2, int imm) {

    uint32_t u = (uint32_t)imm;
    
    uint32_t b12   = (u >> 12) & 0x1;
    uint32_t b11   = (u >> 11) & 0x1;
    uint32_t b10_5 = (u >> 5)  & 0x3F;
    uint32_t b4_1  = (u >> 1)  & 0xF;

    return (op & 0x7F) | (b11 << 7) | (b4_1 << 8) | ((f3 & 0x7) << 12) | 
           ((rs1 & 0x1F) << 15) | ((rs2 & 0x1F) << 20) | (b10_5 << 25) | (b12 << 31);
}

uint32_t encode_J_type(int op, int rd, int imm) {
    uint32_t u = (uint32_t)imm;
    
    uint32_t b20    = (u >> 20) & 0x1;
    uint32_t b10_1  = (u >> 1)  & 0x3FF;
    uint32_t b11    = (u >> 11) & 0x1;
    uint32_t b19_12 = (u >> 12) & 0xFF;

    return (op & 0x7F) | ((rd & 0x1F) << 7) | 
           (b19_12 << 12) | (b11 << 20) | (b10_1 << 21) | (b20 << 31);
}



uint32_t encode_instruction(char* name, int a1, int a2, int a3) {

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

    if (!strcmp(name, "mul"))    return encode_R_type(0x33, 0x0, 0x01, a2, a3, a1);
    if (!strcmp(name, "div"))    return encode_R_type(0x33, 0x4, 0x01, a2, a3, a1);
    if (!strcmp(name, "divu"))   return encode_R_type(0x33, 0x5, 0x01, a2, a3, a1);
    if (!strcmp(name, "rem"))    return encode_R_type(0x33, 0x6, 0x01, a2, a3, a1);
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



    if (!strcmp(name, "lb"))  return encode_I_type(0x03, 0x0, a2, a3, a1);
    if (!strcmp(name, "lh"))  return encode_I_type(0x03, 0x1, a2, a3, a1);
    if (!strcmp(name, "lw"))  return encode_I_type(0x03, 0x2, a2, a3, a1);
    if (!strcmp(name, "lbu")) return encode_I_type(0x03, 0x4, a2, a3, a1);
    if (!strcmp(name, "lhu")) return encode_I_type(0x03, 0x5, a2, a3, a1);

    // Standard CSR Instructions (rd, csr, rs1)
    if (!strcmp(name, "csrrw")) return encode_I_type(0x73, 0x1, a2, a3, a1); // f3=001
    if (!strcmp(name, "csrrs")) return encode_I_type(0x73, 0x2, a2, a3, a1); // f3=010
    if (!strcmp(name, "csrrc")) return encode_I_type(0x73, 0x3, a2, a3, a1); // f3=011

    if (!strcmp(name, "csrr")) {
      // a1=rd, a2=csr.  We encode: csrrs rd, csr, x0 (rs1=0)
      return encode_I_type(0x73, 0x2, a2, 0, a1); 
    }
    if (!strcmp(name, "csrw")) {
      // a1=csr, a2=rs1. We encode: csrrw x0, csr, rs1 (rd=0)
      return encode_I_type(0x73, 0x1, a1, a2, 0);
    }
    // csrc csr, rs1 -> csrrc x0, csr, rs1
    if (!strcmp(name, "csrc")) {
      return encode_I_type(0x73, 0x3, a1, a2, 0); 
    }
    // csrs csr, rs1 -> csrrs x0, csr, rs1
    if (!strcmp(name, "csrs")) {
      return encode_I_type(0x73, 0x2, a1, a2, 0); 
    }

    if (!strcmp(name, "sb")) return encode_S_type(0x23, 0x0, a1, a2, a3);
    if (!strcmp(name, "sh")) return encode_S_type(0x23, 0x1, a1, a2, a3);
    if (!strcmp(name, "sw")) return encode_S_type(0x23, 0x2, a1, a2, a3);


    if (!strcmp(name, "beq"))  return encode_B_type(0x63, 0x0, a1, a2, a3);
    if (!strcmp(name, "bne"))  return encode_B_type(0x63, 0x1, a1, a2, a3);
    if (!strcmp(name, "blt"))  return encode_B_type(0x63, 0x4, a1, a2, a3);
    if (!strcmp(name, "bge"))  return encode_B_type(0x63, 0x5, a1, a2, a3);
    if (!strcmp(name, "bltu")) return encode_B_type(0x63, 0x6, a1, a2, a3);
    if (!strcmp(name, "bgeu")) return encode_B_type(0x63, 0x7, a1, a2, a3);
    if (!strcmp(name, "beqz")) return encode_B_type(0x63, 0x0, a1, 0, a3);
    if (!strcmp(name, "bnez")) return encode_B_type(0x63, 0x1, a1, 0, a3);


    if (!strcmp(name, "jal"))   return encode_J_type(0x6F, a1, a2);
    if (!strcmp(name, "jalr"))  return encode_I_type(0x67, 0x0, a3, a2, a1);
    if (!strcmp(name, "lui"))   return encode_U_type(0x37, a1, a2);
    if (!strcmp(name, "auipc")) return encode_U_type(0x17, a1, a2);


    if (!strcmp(name, "ecall")) return 0x00000073;
    if (!strcmp(name, "ebreak")) return 0x00100073;


    if (!strcmp(name, "li"))   return encode_I_type(0x13, 0x0, a2, 0, a1);  // addi rd, x0, imm
    if (!strcmp(name, "mv"))   return encode_I_type(0x13, 0x0, 0, a2, a1);  // addi rd, rs1, 0
    if (!strcmp(name, "j"))    return encode_J_type(0x6F, 0, a2);           // jal x0, target
    if (!strcmp(name, "ret"))  return encode_I_type(0x67, 0x0, 0, 1, 0);    // jalr x0, x1, 0
    if (!strcmp(name, "nop"))  return encode_I_type(0x13, 0x0, 0, 0, 0);    // addi x0, x0, 0
    // System Instructions (Trap handling)
    if (!strcmp(name, "mret")) return 0x30200073; // Return from Machine Trap
    if (!strcmp(name, "sret")) return 0x10200073; // Return from Supervisor Trap
    if (!strcmp(name, "wfi"))  return 0x10500073; // Wait For Interrupt
      return 0; // Unknown instruction
    }



bool parse_arg(const char *arg, int *out_val) {
    if (!arg || *arg == '\0') return false;
    while (isspace((unsigned char)*arg)) arg++;


    if (arg[0] == 'x' && isdigit(arg[1])) { *out_val = atoi(arg + 1); return true; }
    

    if (!strcmp(arg, "zero")) { *out_val = 0; return true; }
    if (!strcmp(arg, "ra"))   { *out_val = 1; return true; }
    if (!strcmp(arg, "sp"))   { *out_val = 2; return true; }
    if (!strcmp(arg, "gp"))   { *out_val = 3; return true; }
    if (!strcmp(arg, "tp"))   { *out_val = 4; return true; }


    if (arg[0] == 't' && isdigit(arg[1])) {
        int n = atoi(arg + 1);
        *out_val = (n <= 2) ? (n + 5) : (n + 25);
        return true;
    }


    if (arg[0] == 's' && isdigit(arg[1])) {
        int n = atoi(arg + 1);
        if (n == 0) *out_val = 8;
        else if (n == 1) *out_val = 9;
        else *out_val = n + 16;
        return true;
    }
    if (!strcmp(arg, "fp")) { *out_val = 8; return true; }


    if (arg[0] == 'a' && isdigit(arg[1])) {
        *out_val = atoi(arg + 1) + 10;
        return true;
    }


    if (isdigit(*arg) || (*arg == '-' && isdigit(*(arg + 1)))) {
        *out_val = (int)strtol(arg, NULL, 0);
        return true;
    }

    return false;
}
void simplify_punct(char *str) {
    char buffer[MAX_LINE_LEN * 2]; 
    int j = 0;

    for (int i = 0; str[i]; i++) {
        if (str[i] == ',' || str[i] == '(' || str[i] == ')') {
            buffer[j++] = ' ';
        }
        else if (str[i] == '=') {
            buffer[j++] = ' ';
            buffer[j++] = '=';
            buffer[j++] = ' ';
        }
        else {
            buffer[j++] = str[i];
        }
    }
    
    buffer[j] = '\0';
    strcpy(str, buffer); 
}

bool split_line(const char *str, char *ins, char *arg1, char *arg2, char *arg3, char *arg4) {
    char clean[MAX_LINE_LEN];
    strncpy(clean, str, MAX_LINE_LEN - 1); clean[MAX_LINE_LEN-1] = '\0';
    char *comment = strpbrk(clean, ";#"); if (comment) *comment = '\0';
    *ins = *arg1 = *arg2 = *arg3 = *arg4 = '\0';
    return sscanf(clean, "%s %s %s %s %s", ins, arg1, arg2, arg3, arg4) >= 1;
}




void handle_directive(char *directive, char *args, bool write_mode) {
char *comment = strpbrk(args, ";#"); 
    if (comment) *comment = '\0';
    if (!strcmp(directive, ".include")) {
    // Usage: .include "macros.s"
    char *start = strchr(args, '\"');
    char *end = strrchr(args, '\"');
    
    if (start && end && start != end) {
        *end = '\0'; // Temporarily terminate for the filename
        char *filename = start + 1;

        FILE *inc_fp = fopen(filename, "r");
        if (inc_fp) {
            // RECURSIVE PASS: Process the included file immediately
            // It continues using the global 'as_state' and 'symbol_table'
            process_pass(inc_fp, write_mode); 
            fclose(inc_fp);
        } else {
            printf("FASM Error: Include file not found: %s\n", filename);
        }
    }
  }
  else if (!strcmp(directive, ".text")) as_state.current_section = SECTION_TEXT;
    else if (!strcmp(directive, ".data")) as_state.current_section = SECTION_DATA;
    else if (!strcmp(directive, ".org")) {
        as_state.origin = (uint32_t)strtol(args, NULL, 0);
    }
    else if (!strcmp(directive, ".word")) {
        while ((as_state.origin + as_state.size) % 4 != 0) as_state.size++;
        
        if (write_mode) {
            uint32_t val = (uint32_t)eval_math(args, as_state.size, false);
            
            memcpy(&as_state.image[as_state.size], &val, 4);
        }
        as_state.size += 4;
    }
    else if (!strcmp(directive, ".asciz")) {
    char *start = strchr(args, '\"'), *end = strrchr(args, '\"');
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
  }

else if (!strcmp(directive, ".align")) {
    int align_val = eval_math(args, as_state.size, false);
    
    if (align_val <= 0) align_val = 4; 

    while ((as_state.origin + as_state.size) % align_val != 0) {
        if (write_mode) {
            as_state.image[as_state.size] = 0; 
        }
        as_state.size++;
    }
}
else if (!strcmp(directive, ".fill")) {
    int vals[3] = {0, 1, 0}; 
    int idx = 0;
    
    char *ptr = args;
    while (*ptr && idx < 3) {
        while (isspace((unsigned char)*ptr)) ptr++; // Skip spaces
        if (!*ptr) break;
        
        char *end = ptr;
        while (*end && !isspace((unsigned char)*end)) end++; // Find end of token
        
        char expr[128];
        int len = end - ptr;
        if (len > 127) len = 127;
        strncpy(expr, ptr, len);
        expr[len] = '\0';
        
        vals[idx++] = eval_math(expr, as_state.size, false);
        ptr = end;
    }

    int count = vals[0];
    int size = vals[1];
    int value = vals[2];
    
    for (int i = 0; i < (count * size); i++) {
        if (write_mode) {
            // For multi-byte fills, this writes the LSBs repeatedly. 
            // Standard behavior varies, but this mimics .space with a fill value.
            as_state.image[as_state.size] = (uint8_t)value;
        }
        as_state.size++;
    }
}
else if (!strcmp(directive, ".byte")) {
    char *ptr = args;
    while (*ptr) {
        while (isspace((unsigned char)*ptr)) ptr++;
        if (!*ptr) break;
        
        char *end = ptr;
        while (*end && !isspace((unsigned char)*end)) end++;
        
        char expr[128];
        int len = end - ptr;
        if (len > 127) len = 127;
        strncpy(expr, ptr, len);
        expr[len] = '\0';
        
        uint8_t val = (uint8_t)eval_math(expr, as_state.size, false);
        if (write_mode) as_state.image[as_state.size] = val;
        as_state.size += 1;
        
        ptr = end;
    }
}
else if (!strcmp(directive, ".half")) {
    while ((as_state.origin + as_state.size) % 2 != 0) as_state.size++;
    
    char *ptr = args;
    while (*ptr) {
        while (isspace((unsigned char)*ptr)) ptr++;
        if (!*ptr) break;
        
        char *end = ptr;
        while (*end && !isspace((unsigned char)*end)) end++;
        
        char expr[128];
        int len = end - ptr;
        if (len > 127) len = 127;
        strncpy(expr, ptr, len);
        expr[len] = '\0';
        
        uint16_t val = (uint16_t)eval_math(expr, as_state.size, false);
        if (write_mode) memcpy(&as_state.image[as_state.size], &val, 2);
        as_state.size += 2;
        
        ptr = end;
    }
}
else if (!strcmp(directive, ".space")) {
    int num_bytes = eval_math(args, as_state.size, false);
    
    if (num_bytes < 0) {
        printf("Assembler Error: Negative space allocation (%d)\n", num_bytes);
        return;
    }

    for (int i = 0; i < num_bytes; i++) {
        if (write_mode) {
            as_state.image[as_state.size] = 0;
        }
        as_state.size++;
    }
}
}
void process_pass(FILE *fp, bool write_mode) {
    char line[MAX_LINE_LEN];
    //init_assembler_pass();
    rewind(fp);

    unique_id_counter = 0;
    stack_ptr = -1;
    defining_macro = false;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        char work[MAX_LINE_LEN];
        strcpy(work, line);
        simplify_punct(work);

        char ins[32], a1[32], a2[32], a3[32], a4[32];
        if (!split_line(work, ins, a1, a2, a3, a4)) continue;

        
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

       
        int m_idx = find_macro(ins);
        if (m_idx != -1) {
            int current_id;

      
            if (strncmp(ins, "end", 3) == 0) {
                const char* base_name = ins + 3;
                current_id = pop_id(base_name);
            } 
            else {
                
                current_id = ++unique_id_counter;
                
               
                char end_search[64];
                snprintf(end_search, 64, "end%s", ins);
                if (find_macro(end_search) != -1) {
                    push_id(current_id, ins);
                }
                
            }

            for (int i = 0; i < macro_lib[m_idx].line_count; i++) {
                char expanded[MAX_LINE_LEN];
                strcpy(expanded, macro_lib[m_idx].lines[i]);
                substitute_args_with_id(expanded, a1, a2, a3, a4, current_id);
                process_instruction(expanded, write_mode); 
            }
            continue;
        }

        
        if (ins[0] == '.' && ins[strlen(ins)-1] != ':') {
            char *args_ptr = strstr(line, ins) + strlen(ins);
            handle_directive(ins, args_ptr, write_mode);
            continue; 
        }

       
        if (!defining_macro) {
      
            if (stack_ptr >= 0) {
                int current_id = peek_id();
     
                substitute_args_with_id(line, "", "", "", "", current_id);
            }
            process_instruction(line, write_mode);
        }
    }
}

void save_binary(const char* filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
   
    fwrite(as_state.image, 1, as_state.size, f);
    
    fclose(f);
    printf("FASM Binary Saved: %d bytes\n", as_state.size);
}
void process_instruction(char *line, bool write_mode) {
    char work[MAX_LINE_LEN]; 
    strcpy(work, line); 
    simplify_punct(work);
    
    char ins[32], a1[32], a2[32], a3[32], a4[32];
    if (!split_line(work, ins, a1, a2, a3, a4)) return;

    uint32_t current_offset = as_state.size;

  
    if (ins[strlen(ins)-1] == ':') {
        ins[strlen(ins)-1] = '\0';
        if (!write_mode) add_symbol(ins, current_offset);
        if (strlen(a1) == 0) return;
        strcpy(ins, a1); strcpy(a1, a2); strcpy(a2, a3); strcpy(a3, a4);
    }

if (strcmp(a1, "=") == 0) {
        char *eq_pos = strchr(work, '='); 
        if (eq_pos) {
            char *math_expression = eq_pos + 1; 
            char *comment = strpbrk(math_expression, ";#");
            if (comment) *comment = '\0';
            while (*math_expression == ' ' || *math_expression == '\t') {
                math_expression++;
            }
            
            int val = eval_math(math_expression, current_offset, false);
            add_or_update_variable(ins, val);
        }
        return; 
    }
 
    if (ins[0] == '.') {
        char *args_ptr = strstr(line, ins) + strlen(ins);
        handle_directive(ins, args_ptr, write_mode);
        return; 
    }

    if (as_state.current_section == SECTION_DATA) return;


    int v1 = 0, v2 = 0, v3 = 0;

    if (!strcmp(ins, "la")) {
        int rd = eval_math(a1, current_offset, false);
        if (write_mode) {
            int addr = eval_math(a2, current_offset, false);
            uint32_t lower = addr & 0xFFF;
            uint32_t upper = (lower & 0x800) ? ((uint32_t)addr >> 12) + 1 : ((uint32_t)addr >> 12);
            uint32_t lui = encode_U_type(0x37, rd, upper << 12);
            uint32_t addi = encode_I_type(0x13, 0x0, lower, rd, rd);
            memcpy(&as_state.image[as_state.size], &lui, 4);
            memcpy(&as_state.image[as_state.size + 4], &addi, 4);
        }
        as_state.size += 8;
    }
    else if (!strcmp(ins, "li")) {
        int rd = eval_math(a1, current_offset, false);
        
        // <--- FIX: Always evaluate the math first!
        int val = eval_math(a2, current_offset, false); 
        
        // Now check the CALCULATED value (15), not the parsed string
        if (val < -2048 || val > 2047) {
            if (write_mode) {
                // Large number: LUI + ADDI
                uint32_t lower = val & 0xFFF;
                uint32_t upper = (lower & 0x800) ? ((uint32_t)val >> 12) + 1 : ((uint32_t)val >> 12);
                uint32_t lui = encode_U_type(0x37, rd, upper << 12);
                uint32_t addi = encode_I_type(0x13, 0x0, lower, rd, rd);
                memcpy(&as_state.image[as_state.size], &lui, 4);
                memcpy(&as_state.image[as_state.size + 4], &addi, 4);
            }
            as_state.size += 8;
        } else {
            // Small number: ADDI only
            if (write_mode) {
                uint32_t bin = encode_I_type(0x13, 0x0, val, 0, rd);
                memcpy(&as_state.image[as_state.size], &bin, 4);
            }
            as_state.size += 4;
        }
    } else {
        if (!strcmp(ins, "j")) {
            v1 = 0;   // rd = x0
            v2 = eval_math(a1, current_offset, true); // offset maps to a2
            v3 = 0;   // unused
        } 
        else if (ins[0] == 'b') { 
            v1 = eval_math(a1, current_offset, false); // rs1
            if (!strcmp(ins, "beqz") || !strcmp(ins, "bnez")) {
                v2 = 0; // rs2 = x0
                v3 = eval_math(a2, current_offset, true); // offset maps to a3
            } else {
                v2 = eval_math(a2, current_offset, false); // rs2
                v3 = eval_math(a3, current_offset, true);  // offset maps to a3
            }
        }
        else if (!strcmp(ins, "jal")) {
            v1 = eval_math(a1, current_offset, false); // rd
            v2 = eval_math(a2, current_offset, true);  // offset maps to a2
            v3 = 0;
        }
        else {
            
            v1 = eval_math(a1, current_offset, false);
            v2 = eval_math(a2, current_offset, false);
            v3 = eval_math(a3, current_offset, false);
        }

        if (write_mode) {
            uint32_t bin = encode_instruction(ins, v1, v2, v3);
            if (bin != 0) memcpy(&as_state.image[as_state.size], &bin, 4);
        }
        as_state.size += 4;
    }   
}
