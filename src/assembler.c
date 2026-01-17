#include "assembler.h"
#include "preprocessor.h"

Symbol symbol_table[MAX_SYMBOLS];
int symbol_count = 0;
char current_parent[64] = "global";
Assembler as_state;

// --- INITIALIZATION ---

void init_assembler_total() {
    memset(&as_state, 0, sizeof(Assembler));
    as_state.origin = 0x80000000; // Default for QEMU Virt
    symbol_count = 0;
}

void init_assembler_pass() {
    as_state.size = 0; // Start at the very beginning of the image
    as_state.current_section = SECTION_TEXT;
}

// --- SYMBOL MANAGEMENT ---

void add_symbol(const char* name, uint32_t offset) {
    if (symbol_count >= MAX_SYMBOLS) return;
    
    char full_name[128];
    // --- 1. Handle Local vs Global Labels ---
    if (name[0] == '.') {
        // Local label: append to current parent (e.g., main.loop)
        snprintf(full_name, sizeof(full_name), "%s%s", current_parent, name);
    } else {
        // Global label: update the parent and use the name directly
        strncpy(current_parent, name, 63);
        strncpy(full_name, name, 127);
    }
    
    strcpy(symbol_table[symbol_count].name, full_name);
    
    // --- 2. The FASM "Flat" Logic ---
    // In FASM, we don't care about sections for address calculation.
    // Every symbol address is simply the Global Origin + the offset in the binary.
    // If .org was 0x80000000, and this is the 10th byte, address is 0x8000000A.
    
    symbol_table[symbol_count].address = as_state.origin + offset;
    
    // Debug print so you can see it working during Pass 1
    printf("  [SYMBOL] %-15s -> 0x%08X\n", full_name, symbol_table[symbol_count].address);

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

int resolve_val(const char* name, uint32_t current_offset, bool is_relative) {
    int val;
    if (parse_arg(name, &val)) return val; 

    int target_addr = find_symbol(name);
    if (target_addr != -1) {
        if (is_relative) {
            uint32_t absolute_pc = as_state.origin + current_offset;
            int32_t diff = (int32_t)target_addr - (int32_t)absolute_pc;
            // DEBUG PRINT
            if (as_state.size > 0) { // Only print during Write Pass
                printf("  [REL_RESOLVE] Symbol: %s | Target: 0x%X | PC: 0x%X | Offset: %d\n", 
                        name, target_addr, absolute_pc, diff);
            }
            return diff;
        }
        return target_addr; 
    }
    return 0; 
}
// --- ENCODING UTILITIES ---

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
    // Cast to unsigned to ensure logical shifts (no sign-extension interference)
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

// --- CORE ENCODER ---

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

    // --- I-Type: Immediate Arithmetic ---
    if (!strcmp(name, "addi"))  return encode_I_type(0x13, 0x0, a3, a2, a1);
    if (!strcmp(name, "slti"))  return encode_I_type(0x13, 0x2, a3, a2, a1);
    if (!strcmp(name, "sltiu")) return encode_I_type(0x13, 0x3, a3, a2, a1);
    if (!strcmp(name, "xori"))  return encode_I_type(0x13, 0x4, a3, a2, a1);
    if (!strcmp(name, "ori"))   return encode_I_type(0x13, 0x6, a3, a2, a1);
    if (!strcmp(name, "andi"))  return encode_I_type(0x13, 0x7, a3, a2, a1);
    if (!strcmp(name, "slli"))  return encode_I_type(0x13, 0x1, a3 & 0x1F, a2, a1);
    if (!strcmp(name, "srli"))  return encode_I_type(0x13, 0x5, a3 & 0x1F, a2, a1);
    if (!strcmp(name, "srai"))  return encode_I_type(0x13, 0x5, (a3 & 0x1F) | 0x400, a2, a1);

    // --- I-Type: Loads ---
    // Usage: lw rd, offset(rs1) -> a1=rd, a2=offset, a3=rs1
    if (!strcmp(name, "lb"))  return encode_I_type(0x03, 0x0, a2, a3, a1);
    if (!strcmp(name, "lh"))  return encode_I_type(0x03, 0x1, a2, a3, a1);
    if (!strcmp(name, "lw"))  return encode_I_type(0x03, 0x2, a2, a3, a1);
    if (!strcmp(name, "lbu")) return encode_I_type(0x03, 0x4, a2, a3, a1);
    if (!strcmp(name, "lhu")) return encode_I_type(0x03, 0x5, a2, a3, a1);

    // --- S-Type: Stores ---
    // Usage: sw rs2, offset(rs1) -> a1=rs2, a2=offset, a3=rs1
    if (!strcmp(name, "sb")) return encode_S_type(0x23, 0x0, a1, a2, a3);
    if (!strcmp(name, "sh")) return encode_S_type(0x23, 0x1, a1, a2, a3);
    if (!strcmp(name, "sw")) return encode_S_type(0x23, 0x2, a1, a2, a3);

    // --- B-Type: Branches ---
    if (!strcmp(name, "beq"))  return encode_B_type(0x63, 0x0, a1, a2, a3);
    if (!strcmp(name, "bne"))  return encode_B_type(0x63, 0x1, a1, a2, a3);
    if (!strcmp(name, "blt"))  return encode_B_type(0x63, 0x4, a1, a2, a3);
    if (!strcmp(name, "bge"))  return encode_B_type(0x63, 0x5, a1, a2, a3);
    if (!strcmp(name, "bltu")) return encode_B_type(0x63, 0x6, a1, a2, a3);
    if (!strcmp(name, "bgeu")) return encode_B_type(0x63, 0x7, a1, a2, a3);
    if (!strcmp(name, "beqz")) return encode_B_type(0x63, 0x0, a1, 0, a2);
    if (!strcmp(name, "bnez")) return encode_B_type(0x63, 0x1, a1, 0, a2);

    // --- Jumps & Upper Immediates ---
    if (!strcmp(name, "jal"))   return encode_J_type(0x6F, a1, a2);
    if (!strcmp(name, "jalr"))  return encode_I_type(0x67, 0x0, a3, a2, a1);
    if (!strcmp(name, "lui"))   return encode_U_type(0x37, a1, a2);
    if (!strcmp(name, "auipc")) return encode_U_type(0x17, a1, a2);

    // --- System ---
    if (!strcmp(name, "ecall")) return 0x00000073;
    if (!strcmp(name, "ebreak")) return 0x00100073;

    // --- Pseudo-instructions ---
    if (!strcmp(name, "li"))   return encode_I_type(0x13, 0x0, a2, 0, a1);  // addi rd, x0, imm
    if (!strcmp(name, "mv"))   return encode_I_type(0x13, 0x0, 0, a2, a1);  // addi rd, rs1, 0
    if (!strcmp(name, "j"))    return encode_J_type(0x6F, 0, a2);           // jal x0, target
    if (!strcmp(name, "ret"))  return encode_I_type(0x67, 0x0, 0, 1, 0);    // jalr x0, x1, 0
    if (!strcmp(name, "nop"))  return encode_I_type(0x13, 0x0, 0, 0, 0);    // addi x0, x0, 0

    return 0; // Unknown instruction
}

// --- PARSER ---

bool parse_arg(const char *arg, int *out_val) {
    if (!arg || *arg == '\0') return false;
    while (isspace((unsigned char)*arg)) arg++;

    // Standard x0-x31
    if (arg[0] == 'x' && isdigit(arg[1])) { *out_val = atoi(arg + 1); return true; }
    
    // ABI Names
    if (!strcmp(arg, "zero")) { *out_val = 0; return true; }
    if (!strcmp(arg, "ra"))   { *out_val = 1; return true; }
    if (!strcmp(arg, "sp"))   { *out_val = 2; return true; }
    if (!strcmp(arg, "gp"))   { *out_val = 3; return true; }
    if (!strcmp(arg, "tp"))   { *out_val = 4; return true; }

    // Temporary registers: t0-t2 (5-7), t3-t6 (28-31)
    if (arg[0] == 't' && isdigit(arg[1])) {
        int n = atoi(arg + 1);
        *out_val = (n <= 2) ? (n + 5) : (n + 25);
        return true;
    }

    // Saved registers: s0/fp (8), s1 (9), s2-s11 (18-27)
    if (arg[0] == 's' && isdigit(arg[1])) {
        int n = atoi(arg + 1);
        if (n == 0) *out_val = 8;
        else if (n == 1) *out_val = 9;
        else *out_val = n + 16;
        return true;
    }
    if (!strcmp(arg, "fp")) { *out_val = 8; return true; }

    // Argument registers: a0-a7 (10-17)
    if (arg[0] == 'a' && isdigit(arg[1])) {
        *out_val = atoi(arg + 1) + 10;
        return true;
    }

    // Immediate values (decimals or hex)
    if (isdigit(*arg) || (*arg == '-' && isdigit(*(arg + 1)))) {
        *out_val = (int)strtol(arg, NULL, 0);
        return true;
    }

    return false;
}
void simplify_punct(char *str) {
    for (int i = 0; str[i]; i++) if (str[i] == ',' || str[i] == '(' || str[i] == ')') str[i] = ' ';
}

bool split_line(const char *str, char *ins, char *arg1, char *arg2, char *arg3, char *arg4) {
    char clean[MAX_LINE_LEN];
    strncpy(clean, str, MAX_LINE_LEN - 1); clean[MAX_LINE_LEN-1] = '\0';
    char *comment = strpbrk(clean, ";#"); if (comment) *comment = '\0';
    *ins = *arg1 = *arg2 = *arg3 = *arg4 = '\0';
    return sscanf(clean, "%s %s %s %s %s", ins, arg1, arg2, arg3, arg4) >= 1;
}

// --- DIRECTIVE & PASS LOGIC ---


void handle_directive(char *directive, char *args, bool write_mode) {
    if (!strcmp(directive, ".text")) as_state.current_section = SECTION_TEXT;
    else if (!strcmp(directive, ".data")) as_state.current_section = SECTION_DATA;
    else if (!strcmp(directive, ".org")) {
        as_state.origin = (uint32_t)strtol(args, NULL, 0);
    }
    else if (!strcmp(directive, ".word")) {
        while ((as_state.origin + as_state.size) % 4 != 0) as_state.size++;
        if (write_mode) {
            uint32_t val = (uint32_t)strtol(args, NULL, 0);
            memcpy(&as_state.image[as_state.size], &val, 4);
        }
        as_state.size += 4;
    }
    else if (!strcmp(directive, ".asciz")) {
    char *start = strchr(args, '\"'), *end = strrchr(args, '\"');
    if (start && end && start != end) {
        for (char *p = start + 1; p < end; p++) {
            if (*p == '\\' && *(p+1) == 'n') {
                if (write_mode) as_state.image[as_state.size] = 0x0A; // Newline
                p++; // Skip 'n'
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
    int align_val = (int)strtol(args, NULL, 0);
    while ((as_state.origin + as_state.size) % align_val != 0) {
        if (write_mode) {
            as_state.image[as_state.size] = 0; // Pad with zeros
        }
        as_state.size++;
    }
  }
  else if (!strcmp(directive, ".byte")) {
    char *p = args;
    while (*p) {
        if (isdigit(*p) || (*p == '0' && *(p+1) == 'x')) {
            uint8_t val = (uint8_t)strtol(p, &p, 0);
            if (write_mode) as_state.image[as_state.size] = val;
            as_state.size += 1;
        } else p++;
    }
  }
  else if (!strcmp(directive, ".half")) {
    // RISC-V expects 2-byte alignment for half-words
    while ((as_state.origin + as_state.size) % 2 != 0) as_state.size++;
    
    char *p = args;
    while (*p) {
        if (isdigit(*p) || (*p == '0' && *(p+1) == 'x')) {
            uint16_t val = (uint16_t)strtol(p, &p, 0);
            if (write_mode) memcpy(&as_state.image[as_state.size], &val, 2);
            as_state.size += 2;
        } else p++;
    }
  }
}

void process_instruction(char *line, bool write_mode) {
    char work[MAX_LINE_LEN]; 
    strcpy(work, line); 
    simplify_punct(work);
    
    char ins[32], a1[32], a2[32], a3[32], a4[32];
    if (!split_line(work, ins, a1, a2, a3, a4)) return;

    uint32_t current_offset = as_state.size;

    // 1. Handle Labels
    if (ins[strlen(ins)-1] == ':') {
        ins[strlen(ins)-1] = '\0';
        if (!write_mode) add_symbol(ins, current_offset);
        if (strlen(a1) == 0) return;
        strcpy(ins, a1); strcpy(a1, a2); strcpy(a2, a3); strcpy(a3, a4);
    }

    // 2. Handle Directives
    if (ins[0] == '.') {
        char *args_ptr = strstr(line, ins) + strlen(ins);
        handle_directive(ins, args_ptr, write_mode);
        return; 
    }

    // 3. Instruction Encoding
    if (as_state.current_section == SECTION_DATA) return;

    if (!strcmp(ins, "la")) {
    // During Pass 1, addr will be 0. 
    // We MUST increment by 8 in BOTH passes to keep offsets consistent.
    int rd = resolve_val(a1, current_offset, false);
    int addr = resolve_val(a2, current_offset, false);
    
    if (write_mode) {
        uint32_t lower = addr & 0xFFF;
        uint32_t upper = (lower & 0x800) ? (addr >> 12) + 1 : (addr >> 12);
        uint32_t lui = encode_U_type(0x37, rd, upper << 12);
        uint32_t addi = encode_I_type(0x13, 0x0, lower, rd, rd);
        memcpy(&as_state.image[as_state.size], &lui, 4);
        memcpy(&as_state.image[as_state.size + 4], &addi, 4);
    }
    as_state.size += 8; // Always 8
}
    else if (!strcmp(ins, "li")) {
    int rd = resolve_val(a1, current_offset, false);
    int val;
    // Check if the argument is a raw number or a symbol
    bool is_const = parse_arg(a2, &val);
    
    // If it's a symbol OR a large constant, it's ALWAYS 8 bytes
    if (!is_const || val < -2048 || val > 2047) {
        if (write_mode) {
            val = resolve_val(a2, current_offset, false);
            uint32_t lower = val & 0xFFF;
            uint32_t upper = (lower & 0x800) ? ((uint32_t)val >> 12) + 1 : ((uint32_t)val >> 12);
            uint32_t lui = encode_U_type(0x37, rd, upper << 12);
            uint32_t addi = encode_I_type(0x13, 0x0, lower, rd, rd);
            memcpy(&as_state.image[as_state.size], &lui, 4);
            memcpy(&as_state.image[as_state.size + 4], &addi, 4);
        }
        as_state.size += 8;
    } else {
        if (write_mode) {
            uint32_t addi = encode_I_type(0x13, 0x0, val, 0, rd);
            memcpy(&as_state.image[as_state.size], &addi, 4);
        }
        as_state.size += 4;
    }
}
else {
        int v1 = 0, v2 = 0, v3 = 0;

        if (!strcmp(ins, "j")) {
            v1 = 0; // x0
            v2 = resolve_val(a1, current_offset, true); // <--- Relative distance
        } 
        else if (!strcmp(ins, "jal")) {
            v1 = resolve_val(a1, current_offset, false);
            v2 = resolve_val(a2, current_offset, true); // <--- Relative distance
        } 
        else if (ins[0] == 'b') { // beqz, bnez, beq, bne
            v1 = resolve_val(a1, current_offset, false);
            if (!strcmp(ins, "beqz") || !strcmp(ins, "bnez")) {
                v2 = 0; // x0
                v3 = resolve_val(a2, current_offset, true); // <--- Relative distance
            } else {
                v2 = resolve_val(a2, current_offset, false);
                v3 = resolve_val(a3, current_offset, true); // <--- Relative distance
            }
        }
        else if (!strcmp(ins, "lb") || !strcmp(ins, "lw") || !strcmp(ins, "sb") || !strcmp(ins, "sw")) {
            v1 = resolve_val(a1, current_offset, false);
            v2 = resolve_val(a2, current_offset, false); 
            v3 = resolve_val(a3, current_offset, false);
        } 
        else {
            v1 = resolve_val(a1, current_offset, false);
            v2 = resolve_val(a2, current_offset, false);
            v3 = resolve_val(a3, current_offset, false);
        }

        if (write_mode) {
            uint32_t bin = encode_instruction(ins, v1, v2, v3);
            if (bin != 0) {
                memcpy(&as_state.image[as_state.size], &bin, 4);
            }
        }
        as_state.size += 4;
    }
}
void process_pass(FILE *fp, bool write_mode) {
    char line[MAX_LINE_LEN];
    init_assembler_pass(); // Reset offsets for the start of the pass
    rewind(fp);

    while (fgets(line, sizeof(line), fp)) {
        // 1. Clean the line
        line[strcspn(line, "\r\n")] = '\0';
        
        char ins[32], a1[32], a2[32], a3[32], a4[32];
        char split_tmp[MAX_LINE_LEN];
        strcpy(split_tmp, line);
        simplify_punct(split_tmp);

        // 2. Skip empty lines
        if (!split_line(split_tmp, ins, a1, a2, a3, a4)) continue;

        // 3. Handle Section Switching and ORG immediately
        if (ins[0] == '.') {
            char *args_ptr = strstr(line, ins) + strlen(ins);
            handle_directive(ins, args_ptr, write_mode);
            // Crucial: After a directive, we move to the NEXT line
            continue; 
        }

        // 4. If it's an instruction or label, process it
        process_instruction(line, write_mode);
    }
}
void save_binary(const char* filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    // Write the unified image from byte 0 to as_state.size
    fwrite(as_state.image, 1, as_state.size, f);
    
    fclose(f);
    printf("FASM Binary Saved: %d bytes\n", as_state.size);
}
