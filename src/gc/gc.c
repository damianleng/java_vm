#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../include/gc.h"

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
    /* Walk every active interpreter frame and mark reachable objects.
       We treat every non-zero stack slot and local as a potential object
       pointer (conservative GC) — may retain a few extra objects but will
       never collect anything that is actually live. */
    for (int i = 0; i < gc_call_depth; i++) {
        gc_root_frame *rf = &gc_call_stack[i];

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

    gc_sweep();
}
