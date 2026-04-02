#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../include/gc.h"
#include "../../include/threads.h"

void gc_mark(object *root) {
    if (!root || root->marked) return;
    root->marked = 1;

    size_t num_fields = (root->size - sizeof(object)) / sizeof(int64_t);
    for (size_t i = 0; i < num_fields; i++) {
        if (root->fields[i] != 0) {
            object *ref = (object *)(uintptr_t)root->fields[i];
            gc_mark(ref);
        }
    }
}

void gc_sweep(void) {
    object **cur = &heap_head;
    while (*cur) {
        if (!(*cur)->marked) {
            object *unreachable = *cur;
            *cur = unreachable->next;
            free(unreachable);
        } else {
            (*cur)->marked = 0; /* reset for next GC cycle */
            cur = &(*cur)->next;
        }
    }
}

/* Conservative check: is ptr the address of a live heap object? */
static int is_heap_object(object *ptr) {
    for (object *cur = heap_head; cur; cur = cur->next)
        if (cur == ptr) return 1;
    return 0;
}

void gc_collect(void) {
    /* Walk every active interpreter frame across ALL threads and mark
       reachable objects.  Conservative: every non-zero slot is treated as a
       potential object pointer — may retain a few extra objects but will
       never collect anything actually live. */
    pthread_mutex_lock(&thread_registry_mutex);
    for (int t = 0; t < MAX_JVM_THREADS; t++) {
        if (!thread_registry[t].active) continue;
        int depth = *thread_registry[t].call_depth;
        for (int i = 0; i < depth; i++) {
            gc_root_frame *rf = &thread_registry[t].call_stack[i];
            uint16_t top = *rf->stack_top_ptr;
            for (uint16_t j = 0; j < top; j++) {
                object *obj = (object *)(uintptr_t)rf->stack[j];
                if (is_heap_object(obj)) gc_mark(obj);
            }
            for (uint16_t j = 0; j < rf->num_locals; j++) {
                object *obj = (object *)(uintptr_t)rf->locals[j];
                if (is_heap_object(obj)) gc_mark(obj);
            }
        }
    }
    pthread_mutex_unlock(&thread_registry_mutex);

    gc_sweep();
}
