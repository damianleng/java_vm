#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/heap.h"

object *heap_head = NULL;

void heap_init(void) {
    heap_head = NULL;
}

void heap_destroy(void) {
    object *cur = heap_head;
    while (cur) {
        object *next = cur->next;
        free(cur);
        cur = next;
    }
    heap_head = NULL;
}

object *heap_alloc(uint16_t class_index, size_t num_fields) {
    size_t size = sizeof(object) + num_fields * sizeof(int32_t);
    object *obj = calloc(1, size);
    if (!obj) {
        fprintf(stderr, "OutOfMemoryError\n");
        return NULL;
    }
    obj->class_index = class_index;
    obj->marked      = 0;
    obj->size        = size;
    obj->next        = heap_head;
    heap_head        = obj;
    return obj;
}
