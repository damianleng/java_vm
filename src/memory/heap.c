#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/heap.h"

object *heap_head = NULL;

static pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;

void heap_init(void) {
    heap_head = NULL;
}

void heap_destroy(void) {
    object *cur = heap_head;
    while (cur) {
        object *next = cur->next;
        pthread_mutex_destroy(&cur->monitor);
        free(cur);
        cur = next;
    }
    heap_head = NULL;
}

object *heap_alloc(uint16_t class_index, size_t num_fields) {
    size_t size = sizeof(object) + num_fields * sizeof(int64_t);
    object *obj = calloc(1, size);
    if (!obj) {
        fprintf(stderr, "OutOfMemoryError\n");
        return NULL;
    }
    obj->class_index = class_index;
    obj->marked      = 0;
    obj->is_thread   = 0;
    obj->size        = size;
    pthread_mutex_init(&obj->monitor, NULL);

    pthread_mutex_lock(&heap_mutex);
    obj->next  = heap_head;
    heap_head  = obj;
    pthread_mutex_unlock(&heap_mutex);

    return obj;
}
