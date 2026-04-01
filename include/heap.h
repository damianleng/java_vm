#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_SIZE (1024 * 1024 * 16) /* 16 MB default heap */

typedef struct object {
    uint16_t class_index;
    uint8_t marked;
    size_t size;
    struct object *next; /* intrusive linked list for GC */
    int64_t fields[];   /* flexible array — holds ints and object pointers */
} object;

extern object *heap_head;

void    heap_init(void);
void    heap_destroy(void);
object *heap_alloc(uint16_t class_index, size_t num_fields);

#endif /* HEAP_H */
