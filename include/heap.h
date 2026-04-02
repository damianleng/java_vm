#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#define HEAP_SIZE (1024 * 1024 * 16) /* 16 MB default heap */

typedef struct object {
    uint16_t        class_index;
    uint8_t         marked;
    int             is_thread;   /* 1 after Thread.start() is called on this object */
    size_t          size;
    struct object  *next;        /* intrusive linked list for GC */
    pthread_mutex_t monitor;     /* per-object lock for synchronized() blocks */
    pthread_t       thread_id;   /* pthread handle when is_thread == 1 */
    int64_t         fields[];    /* flexible array — holds ints and object pointers */
} object;

extern object *heap_head;

void    heap_init(void);
void    heap_destroy(void);
object *heap_alloc(uint16_t class_index, size_t num_fields);

#endif /* HEAP_H */
