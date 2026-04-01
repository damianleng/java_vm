#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../../include/classfile.h"

/* Read big-endian values from file */
static uint16_t read_u16(FILE *f) {
    uint8_t b[2];
    fread(b, 1, 2, f);
    return (uint16_t)((b[0] << 8) | b[1]);
}

static uint32_t read_u32(FILE *f) {
    uint8_t b[4];
    fread(b, 1, 4, f);
    return (uint32_t)((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
}

class_file *classfile_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", path);
        return NULL;
    }

    class_file *cf = calloc(1, sizeof(class_file));
    if (!cf) { fclose(f); return NULL; }

    cf->magic = read_u32(f);
    if (cf->magic != CLASS_MAGIC) {
        fprintf(stderr, "Error: not a valid .class file\n");
        free(cf);
        fclose(f);
        return NULL;
    }

    cf->minor_version = read_u16(f);
    cf->major_version = read_u16(f);

    /* TODO: parse constant pool, fields, methods */

    fclose(f);
    return cf;
}

void classfile_free(class_file *cf) {
    if (!cf) return;
    free(cf->constant_pool);
    free(cf->fields);
    free(cf->methods);
    free(cf);
}
