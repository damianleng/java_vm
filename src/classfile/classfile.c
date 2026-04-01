#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../../include/classfile.h"

/* ── binary read helpers ─────────────────────────────────────────────────── */

static uint8_t read_u8(FILE *f) {
    uint8_t b;
    fread(&b, 1, 1, f);
    return b;
}

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

static void skip_bytes(FILE *f, long count) {
    fseek(f, count, SEEK_CUR);
}

/* ── constant pool lookup (defined early; used by method attribute parser) ── */

const char *classfile_get_utf8(const class_file *cf, uint16_t index) {
    if (index == 0 || index >= cf->constant_pool_count) return NULL;
    cp_info *cp = &cf->constant_pool[index - 1];
    if (cp->tag != CP_UTF8 || !cp->info) return NULL;
    /* info layout: [u8 hi][u8 lo][utf8 bytes...]['\0'] */
    return (const char *)(cp->info + 2);
}

/* ── constant pool entry parser ──────────────────────────────────────────── */

/* Returns number of slots consumed (2 for Long/Double, 1 for everything else).
   Returns -1 on error. */
static int parse_cp_entry(FILE *f, cp_info *cp) {
    cp->tag = read_u8(f);

    switch (cp->tag) {
        case CP_UTF8: {
            uint16_t len = read_u16(f);
            /* Store: [hi byte of len][lo byte of len][utf8 bytes]['\0'] */
            cp->info = malloc(2 + len + 1);
            if (!cp->info) return -1;
            cp->info[0] = (uint8_t)(len >> 8);
            cp->info[1] = (uint8_t)(len & 0xff);
            fread(cp->info + 2, 1, len, f);
            cp->info[2 + len] = '\0';
            return 1;
        }
        case CP_INTEGER:
        case CP_FLOAT:
            cp->info = malloc(4);
            if (!cp->info) return -1;
            fread(cp->info, 1, 4, f);
            return 1;

        case CP_LONG:
        case CP_DOUBLE:
            cp->info = malloc(8);
            if (!cp->info) return -1;
            fread(cp->info, 1, 8, f);
            return 2; /* occupies two constant pool slots */

        case CP_CLASS:
        case CP_STRING:
            cp->info = malloc(2);
            if (!cp->info) return -1;
            fread(cp->info, 1, 2, f);
            return 1;

        case CP_FIELDREF:
        case CP_METHODREF:
        case CP_IFMETHODREF:
        case CP_NAMEANDTYPE:
            cp->info = malloc(4);
            if (!cp->info) return -1;
            fread(cp->info, 1, 4, f);
            return 1;

        default:
            fprintf(stderr, "Error: unknown constant pool tag %d\n", cp->tag);
            return -1;
    }
}

/* ── attribute helpers ───────────────────────────────────────────────────── */

/* Skip one attribute (attribute_name_index already consumed; reads length+data). */
static void skip_attribute(FILE *f) {
    uint32_t len = read_u32(f);
    skip_bytes(f, (long)len);
}

/* Parse attributes for a method, extracting the Code attribute into mi. */
static void parse_method_attributes(FILE *f, const class_file *cf, method_info *mi) {
    uint16_t attr_count = read_u16(f);

    for (uint16_t i = 0; i < attr_count; i++) {
        uint16_t name_idx = read_u16(f);
        uint32_t attr_len = read_u32(f);
        const char *attr_name = classfile_get_utf8(cf, name_idx);

        if (attr_name && strcmp(attr_name, "Code") == 0) {
            mi->max_stack   = read_u16(f);
            mi->max_locals  = read_u16(f);
            mi->code_length = read_u32(f);
            mi->code = malloc(mi->code_length);
            if (mi->code) fread(mi->code, 1, mi->code_length, f);

            /* skip exception table */
            uint16_t exc_len = read_u16(f);
            skip_bytes(f, exc_len * 8L);

            /* skip nested Code attributes (LineNumberTable, etc.) */
            uint16_t sub_count = read_u16(f);
            for (uint16_t j = 0; j < sub_count; j++) {
                read_u16(f); /* attribute_name_index */
                skip_attribute(f);
            }
        } else {
            skip_bytes(f, (long)attr_len);
        }
    }
}

/* Skip all attributes for a field (we don't need field attribute data). */
static void skip_field_attributes(FILE *f) {
    uint16_t attr_count = read_u16(f);
    for (uint16_t i = 0; i < attr_count; i++) {
        read_u16(f); /* attribute_name_index */
        skip_attribute(f);
    }
}

/* ── public API ──────────────────────────────────────────────────────────── */

class_file *classfile_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", path);
        return NULL;
    }

    class_file *cf = calloc(1, sizeof(class_file));
    if (!cf) { fclose(f); return NULL; }

    /* Magic + version */
    cf->magic = read_u32(f);
    if (cf->magic != CLASS_MAGIC) {
        fprintf(stderr, "Error: not a valid .class file\n");
        free(cf); fclose(f); return NULL;
    }
    cf->minor_version = read_u16(f);
    cf->major_version = read_u16(f);

    /* Constant pool */
    cf->constant_pool_count = read_u16(f);
    uint16_t cp_size = cf->constant_pool_count - 1;
    cf->constant_pool = calloc(cp_size, sizeof(cp_info));
    if (!cf->constant_pool) { fclose(f); classfile_free(cf); return NULL; }

    for (uint16_t i = 0; i < cp_size; ) {
        int slots = parse_cp_entry(f, &cf->constant_pool[i]);
        if (slots < 0) { fclose(f); classfile_free(cf); return NULL; }
        i += (uint16_t)slots;
    }

    /* Class metadata */
    cf->access_flags = read_u16(f);
    cf->this_class   = read_u16(f);
    cf->super_class  = read_u16(f);

    /* Skip interfaces */
    uint16_t iface_count = read_u16(f);
    skip_bytes(f, iface_count * 2L);

    /* Fields */
    cf->fields_count = read_u16(f);
    if (cf->fields_count > 0) {
        cf->fields = calloc(cf->fields_count, sizeof(field_info));
        if (!cf->fields) { fclose(f); classfile_free(cf); return NULL; }
    }
    for (uint16_t i = 0; i < cf->fields_count; i++) {
        cf->fields[i].access_flags     = read_u16(f);
        cf->fields[i].name_index       = read_u16(f);
        cf->fields[i].descriptor_index = read_u16(f);
        skip_field_attributes(f);
    }

    /* Methods */
    cf->methods_count = read_u16(f);
    if (cf->methods_count > 0) {
        cf->methods = calloc(cf->methods_count, sizeof(method_info));
        if (!cf->methods) { fclose(f); classfile_free(cf); return NULL; }
    }
    for (uint16_t i = 0; i < cf->methods_count; i++) {
        cf->methods[i].access_flags     = read_u16(f);
        cf->methods[i].name_index       = read_u16(f);
        cf->methods[i].descriptor_index = read_u16(f);
        parse_method_attributes(f, cf, &cf->methods[i]);
    }

    fclose(f);
    return cf;
}

void classfile_free(class_file *cf) {
    if (!cf) return;
    if (cf->constant_pool) {
        for (uint16_t i = 0; i < cf->constant_pool_count - 1; i++)
            free(cf->constant_pool[i].info);
        free(cf->constant_pool);
    }
    free(cf->fields);
    if (cf->methods) {
        for (uint16_t i = 0; i < cf->methods_count; i++)
            free(cf->methods[i].code);
        free(cf->methods);
    }
    free(cf);
}

method_info *classfile_find_method(const class_file *cf, const char *name,
                                   const char *descriptor) {
    for (uint16_t i = 0; i < cf->methods_count; i++) {
        const char *mname = classfile_get_utf8(cf, cf->methods[i].name_index);
        if (!mname || strcmp(mname, name) != 0) continue;
        if (descriptor) {
            const char *mdesc = classfile_get_utf8(cf, cf->methods[i].descriptor_index);
            if (!mdesc || strcmp(mdesc, descriptor) != 0) continue;
        }
        return &cf->methods[i];
    }
    return NULL;
}
