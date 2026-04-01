#ifndef CLASSFILE_H
#define CLASSFILE_H

#include <stdint.h>

/* Java class file magic number */
#define CLASS_MAGIC 0xCAFEBABE

typedef struct cp_info {
    uint8_t tag;
    uint8_t *info;
} cp_info;

typedef struct field_info {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
} field_info;

typedef struct method_info {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint8_t *code;
    uint32_t code_length;
    uint16_t max_stack;
    uint16_t max_locals;
} method_info;

typedef struct class_file {
    uint32_t magic;
    uint16_t minor_version;
    uint16_t major_version;
    uint16_t constant_pool_count;
    cp_info *constant_pool;
    uint16_t access_flags;
    uint16_t this_class;
    uint16_t super_class;
    uint16_t fields_count;
    field_info *fields;
    uint16_t methods_count;
    method_info *methods;
} class_file;

class_file *classfile_load(const char *path);
void classfile_free(class_file *cf);

#endif /* CLASSFILE_H */
