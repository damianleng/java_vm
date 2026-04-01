#ifndef CLASSFILE_H
#define CLASSFILE_H

#include <stdint.h>

/* Java class file magic number */
#define CLASS_MAGIC 0xCAFEBABE

/* Constant pool tags */
#define CP_UTF8          1
#define CP_INTEGER       3
#define CP_FLOAT         4
#define CP_LONG          5
#define CP_DOUBLE        6
#define CP_CLASS         7
#define CP_STRING        8
#define CP_FIELDREF      9
#define CP_METHODREF    10
#define CP_IFMETHODREF  11
#define CP_NAMEANDTYPE  12

typedef struct cp_info {
    uint8_t  tag;
    uint8_t *info; /* raw bytes; UTF8 entries are null-terminated after 2-byte length prefix */
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
    uint32_t   magic;
    uint16_t   minor_version;
    uint16_t   major_version;
    uint16_t   constant_pool_count;
    cp_info   *constant_pool;
    uint16_t   access_flags;
    uint16_t   this_class;
    uint16_t   super_class;
    uint16_t   fields_count;
    field_info *fields;
    uint16_t   methods_count;
    method_info *methods;
} class_file;

class_file  *classfile_load(const char *path);
void         classfile_free(class_file *cf);

/* Return null-terminated UTF-8 string for constant pool index (1-based). */
const char  *classfile_get_utf8(const class_file *cf, uint16_t index);

/* Find a method by name (and optionally descriptor — pass NULL to match any). */
method_info *classfile_find_method(const class_file *cf, const char *name, const char *descriptor);

#endif /* CLASSFILE_H */