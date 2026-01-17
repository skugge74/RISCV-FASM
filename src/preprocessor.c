#include "preprocessor.h"
#include <ctype.h>

Macro macro_lib[MAX_MACROS];
int macro_lib_count = 0;
bool defining_macro = false;
ControlBlock control_stack[MAX_STACK_DEPTH];
int stack_ptr = -1;

int find_macro(const char *name) {
    for (int i = 0; i < macro_lib_count; i++) {
        if (strcmp(name, macro_lib[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

int line_contains_comma(const char *line) {
    if (!line) return 0;
    return (strchr(line, ',') != NULL) ? 1 : 0;
}


 int unique_id_counter = 0; 

void substitute_args_with_id(char *line, char *arg1, char *arg2, char *arg3, int id) {
    char buffer[MAX_LINE_LEN];
    char *p;
    char *args[] = {arg1, arg2, arg3};
    
    for (int i = 1; i <= 3; i++) {
        char tag[4]; snprintf(tag, 4, "%%%d", i);
        while ((p = strstr(line, tag))) {
            if (strlen(args[i-1]) == 0) break;
            int pos = p - line;
            memcpy(buffer, line, pos);
            buffer[pos] = '\0';
            strcat(buffer, args[i-1]);
            strcat(buffer, p + 2);
            strcpy(line, buffer);
        }
    }

    while ((p = strstr(line, "%u"))) {
        char id_str[16];
        snprintf(id_str, 16, "%d", id);
        int pos = p - line;
        memcpy(buffer, line, pos);
        buffer[pos] = '\0';
        strcat(buffer, id_str);
        strcat(buffer, p + 2);
        strcpy(line, buffer);
    }
}

void push_id(int id, const char* name) {
    if (stack_ptr < MAX_STACK_DEPTH - 1) {
        stack_ptr++;
        control_stack[stack_ptr].id = id;
        strncpy(control_stack[stack_ptr].name, name, 31);
    }
}

int pop_id(const char* end_name) {
    if (stack_ptr < 0) return 0;



    if (strcmp(control_stack[stack_ptr].name, end_name) != 0) {
        printf("Warning: Mismatched block! Found end%s but top is %s\n", 
                end_name, control_stack[stack_ptr].name);
    }
    return control_stack[stack_ptr--].id;
}
int peek_id() {
    if (stack_ptr >= 0) return control_stack[stack_ptr].id;
    return 0;
}

