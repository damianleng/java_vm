#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>
#include "classfile.h"

typedef struct frame {
    uint8_t *code;
    uint32_t code_length;
    uint32_t pc;
    int32_t *locals;
    uint16_t max_locals;
    int32_t *operand_stack;
    uint16_t stack_top;
    uint16_t max_stack;
} frame;

void interpreter_execute(class_file *cf, method_info *method);

#endif /* INTERPRETER_H */
