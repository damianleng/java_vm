#include <stdio.h>
#include <stdlib.h>
#include "../../include/interpreter.h"

void interpreter_execute(class_file *cf, method_info *method) {
    frame f;
    f.code         = method->code;
    f.code_length  = method->code_length;
    f.pc           = 0;
    f.max_locals   = method->max_locals;
    f.max_stack    = method->max_stack;
    f.locals       = calloc(method->max_locals, sizeof(int32_t));
    f.operand_stack = calloc(method->max_stack, sizeof(int32_t));
    f.stack_top    = 0;

    if (!f.locals || !f.operand_stack) {
        free(f.locals);
        free(f.operand_stack);
        return;
    }

    /* TODO: main bytecode dispatch loop */
    (void)cf;

    free(f.locals);
    free(f.operand_stack);
}
