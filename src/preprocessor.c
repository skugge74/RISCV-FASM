#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "preprocessor.h"
#include "assembler.h" // For MAX_LINE_LEN definition if needed

// ==========================================
// GLOBAL STATE
// ==========================================
Macro macro_lib[MAX_MACROS];
int macro_lib_count = 0;
bool defining_macro = false;

// Stack for nested macros (e.g., loops inside loops)
ControlBlock control_stack[MAX_STACK_DEPTH];
int stack_ptr = -1;
int unique_id_counter = 0;
// This variable is likely defined in assembler.c
// We use 'extern' to tell the compiler it exists elsewhere.

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
// STRING SUBSTITUTION
// ==========================================
void substitute_args_with_id(char *line, char *arg1, char *arg2, char *arg3, char *arg4, int id) {
    char buffer[MAX_LINE_LEN];
    char *p;
    char *args[] = {arg1, arg2, arg3, arg4};
    
    // 1. Substitute Arguments (%1, %2, %3, %4)
    for (int i = 1; i <= 4; i++) {
        char tag[4]; 
        snprintf(tag, 4, "%%%d", i); // Create tag "%1", "%2"...
        
        while ((p = strstr(line, tag))) {
            // Safety: Don't overflow buffer
            if (strlen(line) + strlen(args[i-1]) >= MAX_LINE_LEN) {
                printf("[WARN] Macro expansion too long, truncating.\n");
                break;
            }

            int pos = p - line;
            memcpy(buffer, line, pos); // Copy text BEFORE the tag
            buffer[pos] = '\0';
            
            strcat(buffer, args[i-1]); // Insert argument
            strcat(buffer, p + 2);     // Copy text AFTER the tag
            
            strcpy(line, buffer);      // Update the main line
        }
    }

    // 2. Substitute Unique ID (%u)
    while ((p = strstr(line, "%u"))) {
        char id_str[16];
        snprintf(id_str, 16, "%d", id);

        if (strlen(line) + strlen(id_str) >= MAX_LINE_LEN) break;

        int pos = p - line;
        memcpy(buffer, line, pos);
        buffer[pos] = '\0';
        
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
        printf("[ERROR] Macro stack overflow! Too much nesting.\n");
    }
}

int pop_id(const char* end_name) {
    if (stack_ptr < 0) return 0;

    // Logic Check: Are we closing the right block?
    // e.g., if we see 'endloop' but stack top is 'if', that's a bug.
    if (strcmp(control_stack[stack_ptr].name, end_name) != 0) {
        printf("[WARN] Mismatched block! Found end%s but expected end%s\n", 
                end_name, control_stack[stack_ptr].name);
    }
    return control_stack[stack_ptr--].id;
}

int peek_id() {
    if (stack_ptr >= 0) return control_stack[stack_ptr].id;
    return 0; // Default ID if no stack
}

int line_contains_comma(const char *line) {
    if (!line) return 0;
    return (strchr(line, ',') != NULL) ? 1 : 0;
}
