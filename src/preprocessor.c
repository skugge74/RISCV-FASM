#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "preprocessor.h"
#include "assembler.h" 

// ==========================================
// GLOBAL STATE
// ==========================================
Macro macro_lib[MAX_MACROS];
int macro_lib_count = 0;
bool defining_macro = false;

ControlBlock control_stack[MAX_STACK_DEPTH];
int stack_ptr = -1;
int unique_id_counter = 0;

// ==========================================
// MACRO LOOKUP
// ==========================================
int find_macro(const char *name) {
    for (int i = 0; i < macro_lib_count; i++) {
        if (strcmp(name, macro_lib[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

// ==========================================
// STRING SUBSTITUTION (The Heart of Variadic Macros)
// ==========================================
void substitute_args_with_id(char *line, char args[MAX_ARGS][128], int arg_count, int id) {
    char buffer[MAX_LINE_LEN];
    char *p;

    // --- 1. Substitute %# (Actual Argument Count) ---
    // Useful for: addi sp, sp, -(%# * 4)
    while ((p = strstr(line, "%#"))) {
        char count_str[16];
        sprintf(count_str, "%d", arg_count);
        int pos = p - line;
        
        buffer[0] = '\0';
        strncat(buffer, line, pos);
        strcat(buffer, count_str);
        strcat(buffer, p + 2);
        strcpy(line, buffer);
    }

    // --- 2. Substitute %* (Variadic: All Arguments) ---
    if (strstr(line, "%*")) {
        char var_string[MAX_LINE_LEN] = "";
        for (int i = 0; i < arg_count; i++) {
            if (strlen(args[i]) > 0) {
                if (strlen(var_string) > 0) strcat(var_string, ", ");
                strcat(var_string, args[i]);
            }
        }

        while ((p = strstr(line, "%*"))) {
            int pos = p - line;
            buffer[0] = '\0';
            strncat(buffer, line, pos);
            strcat(buffer, var_string);
            strcat(buffer, p + 2);
            strcpy(line, buffer);
        }
    }

    // --- 3. Substitute %1, %2, ... %n (Numbered Arguments) ---
    for (int i = 0; i < arg_count; i++) {
        char tag[16]; 
        sprintf(tag, "%%%d", i + 1);
        
        while ((p = strstr(line, tag))) {
            // Edge case: if tag is %1, make sure we aren't accidentally hitting %10
            if (isdigit(p[strlen(tag)]) && (i + 1 < 10)) {
                // Skip if this is a partial match of a larger number
                p++; 
                continue; 
            }

            int pos = p - line;
            buffer[0] = '\0';
            strncat(buffer, line, pos);
            strcat(buffer, args[i]);
            strcat(buffer, p + strlen(tag));
            
            if (strlen(buffer) < MAX_LINE_LEN) {
                strcpy(line, buffer);
            } else {
                break;
            }
        }
    }

    // --- 4. Substitute %u (Unique Macro Instance ID) ---
    while ((p = strstr(line, "%u"))) {
        char id_str[16];
        sprintf(id_str, "%d", id);

        int pos = p - line;
        buffer[0] = '\0';
        strncat(buffer, line, pos);
        strcat(buffer, id_str);
        strcat(buffer, p + 2);
        strcpy(line, buffer);
    }
}

// ==========================================
// STACK MANAGEMENT (For Nested Macros)
// ==========================================
void push_id(int id, const char* name) {
    if (stack_ptr < MAX_STACK_DEPTH - 1) {
        stack_ptr++;
        control_stack[stack_ptr].id = id;
        strncpy(control_stack[stack_ptr].name, name, 31);
    } else {
        fprintf(stderr, "[ERROR] Macro stack overflow!\n");
    }
}

int pop_id(const char* end_name) {
    if (stack_ptr < 0) return 0;

    // Verification check to ensure blocks are closed in order
    if (end_name && strcmp(control_stack[stack_ptr].name, end_name) != 0) {
        fprintf(stderr, "[WARN] Mismatched block! Expected end%s, found end%s\n", 
                control_stack[stack_ptr].name, end_name);
    }
    return control_stack[stack_ptr--].id;
}

int peek_id() {
    return (stack_ptr >= 0) ? control_stack[stack_ptr].id : 0;
}

int line_contains_comma(const char *line) {
    if (!line) return 0;
    return (strchr(line, ',') != NULL) ? 1 : 0;
}
