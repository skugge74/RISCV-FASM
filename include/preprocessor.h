#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LEN 256
#define MAX_MACROS 64
#define MAX_STACK_DEPTH 32

typedef struct {
    char name[32];
    char lines[16][MAX_LINE_LEN];
    int line_count;
    int arg_count;
} Macro;

typedef struct {
    int id;
    char name[32];
} ControlBlock;

extern Macro macro_lib[MAX_MACROS];
extern int macro_lib_count;
extern bool defining_macro;
extern int unique_id_counter;
extern ControlBlock control_stack[MAX_STACK_DEPTH];
extern int stack_ptr;

void scan_macros(FILE *fp);
void substitute_args(char *line, char *arg1, char *arg2, char *arg3);
int line_contains_comma(const char *line);
int pop_id();
void push_id(int id);
int peek_id();
void substitute_args_with_id(char *line, char *arg1, char *arg2, char *arg3, int id);

#endif
