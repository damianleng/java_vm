#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>
#include "classfile.h"

typedef struct frame {
    uint8_t  *code;
    uint32_t  code_length;
    uint32_t  pc;
    int64_t  *locals;        /* int64_t holds both int32 values and object pointers */
    uint16_t  max_locals;
    int64_t  *operand_stack;
    uint16_t  stack_top;
    uint16_t  max_stack;
} frame;

/* Bytecode opcodes */
#define OP_NOP           0x00
#define OP_ACONST_NULL   0x01
#define OP_ICONST_M1     0x02
#define OP_ICONST_0      0x03
#define OP_ICONST_1      0x04
#define OP_ICONST_2      0x05
#define OP_ICONST_3      0x06
#define OP_ICONST_4      0x07
#define OP_ICONST_5      0x08
#define OP_BIPUSH        0x10
#define OP_SIPUSH        0x11
#define OP_LDC           0x12
#define OP_ILOAD         0x15
#define OP_ILOAD_0       0x1a
#define OP_ILOAD_1       0x1b
#define OP_ILOAD_2       0x1c
#define OP_ILOAD_3       0x1d
#define OP_ISTORE        0x36
#define OP_ISTORE_0      0x3b
#define OP_ISTORE_1      0x3c
#define OP_ISTORE_2      0x3d
#define OP_ISTORE_3      0x3e
#define OP_DUP           0x59
#define OP_IADD          0x60
#define OP_ISUB          0x64
#define OP_IMUL          0x68
#define OP_IDIV          0x6c
#define OP_IREM          0x70
#define OP_INEG          0x74
#define OP_ISHL          0x78
#define OP_ISHR          0x7a
#define OP_IUSHR         0x7c
#define OP_IAND          0x7e
#define OP_IOR           0x80
#define OP_IXOR          0x82
#define OP_IINC          0x84
#define OP_I2L           0x85
#define OP_I2F           0x86
#define OP_I2D           0x87
#define OP_L2I           0x88
#define OP_F2I           0x8b
#define OP_D2I           0x8e
#define OP_I2B           0x91
#define OP_I2C           0x92
#define OP_I2S           0x93
#define OP_IFEQ          0x99
#define OP_IFNE          0x9a
#define OP_IFLT          0x9b
#define OP_IFGE          0x9c
#define OP_IFGT          0x9d
#define OP_IFLE          0x9e
#define OP_IF_ICMPEQ     0x9f
#define OP_IF_ICMPNE     0xa0
#define OP_IF_ICMPLT     0xa1
#define OP_IF_ICMPGE     0xa2
#define OP_IF_ICMPGT     0xa3
#define OP_IF_ICMPLE     0xa4
#define OP_ALOAD         0x19
#define OP_IALOAD        0x2e
#define OP_AALOAD        0x32
#define OP_BALOAD        0x33
#define OP_CALOAD        0x34
#define OP_SALOAD        0x35
#define OP_ALOAD_0       0x2a
#define OP_ALOAD_1       0x2b
#define OP_ALOAD_2       0x2c
#define OP_ALOAD_3       0x2d
#define OP_ASTORE        0x3a
#define OP_IASTORE       0x4f
#define OP_AASTORE       0x53
#define OP_BASTORE       0x54
#define OP_CASTORE       0x55
#define OP_SASTORE       0x56
#define OP_POP           0x57
#define OP_POP2          0x58
#define OP_ASTORE_0      0x4b
#define OP_ASTORE_1      0x4c
#define OP_ASTORE_2      0x4d
#define OP_ASTORE_3      0x4e
#define OP_GOTO          0xa7
#define OP_TABLESWITCH   0xaa
#define OP_LOOKUPSWITCH  0xab
#define OP_IRETURN       0xac
#define OP_ARETURN       0xb0
#define OP_RETURN        0xb1
#define OP_GETSTATIC     0xb2
#define OP_PUTSTATIC     0xb3
#define OP_GETFIELD      0xb4
#define OP_PUTFIELD      0xb5
#define OP_INVOKESPECIAL 0xb7
#define OP_INVOKEVIRTUAL 0xb6
#define OP_INVOKESTATIC  0xb8
#define OP_NEW           0xbb
#define OP_NEWARRAY      0xbc
#define OP_ANEWARRAY     0xbd
#define OP_ARRAYLENGTH   0xbe
#define OP_MONITORENTER  0xc2
#define OP_MONITOREXIT   0xc3

/*
 * Execute a method. args/args_count pre-populate locals[0..args_count-1].
 * Returns the int32_t return value (0 for void methods).
 */
int64_t interpreter_execute(class_file *cf, method_info *method,
                             int64_t *args, uint16_t args_count);

#endif /* INTERPRETER_H */
