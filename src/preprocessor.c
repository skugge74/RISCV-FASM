#include "preprocessor.h"
#include <ctype.h>

Macro macro_lib[MAX_MACROS];
int macro_lib_count = 0;
bool defining_macro = false;
ControlBlock control_stack[MAX_STACK_DEPTH];
int stack_ptr = -1;




int line_contains_comma(const char *line) {
    if (!line) return 0;
    return (strchr(line, ',') != NULL) ? 1 : 0;
}

void scan_macros(FILE *fp) {
    char line[MAX_LINE_LEN];
    rewind(fp);
    while (fgets(line, sizeof(line), fp)) {
        char ins[32], a1[32], a2[32];
        if (sscanf(line, "%s %s %s", ins, a1, a2) < 2) continue;
        
        if (strcmp(ins, "macro") == 0) {
            Macro *m = &macro_lib[macro_lib_count++];
            strcpy(m->name, a1);
            m->line_count = 0;
            m->arg_count = (a2[0] != '\0') + line_contains_comma(line); 

            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "endm")) break;
                strcpy(m->lines[m->line_count++], line);
            }
        }
    }
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

void push_id(int id) {
    if (stack_ptr < MAX_STACK_DEPTH - 1) control_stack[++stack_ptr].id = id;
}

int pop_id() {
    if (stack_ptr >= 0) return control_stack[stack_ptr--].id;
    return 0;
}
int peek_id() {
    if (stack_ptr >= 0) return control_stack[stack_ptr].id;
    return 0;
}
