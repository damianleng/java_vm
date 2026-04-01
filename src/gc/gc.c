#include <stdlib.h>
#include "../../include/gc.h"


void gc_mark(object *root) {
    if (!root || root->marked) return;
    root->marked = 1;
    /* TODO: recursively mark referenced objects via fields */
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

void gc_collect(void) {
    /* TODO: walk all GC roots (stack frames, static fields) and mark */
    gc_sweep();
}
