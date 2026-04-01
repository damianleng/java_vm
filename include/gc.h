#ifndef GC_H
#define GC_H

#include <stdint.h>
#include "heap.h"

/* One entry per active interpreter frame, kept as a stack in interpreter.c.
   gc_collect() walks these to find all live object references. */
typedef struct gc_root_frame {
    int64_t  *stack;         /* frame's operand stack array     */
    uint16_t *stack_top_ptr; /* live pointer into frame.stack_top */
    int64_t  *locals;        /* frame's local variable array    */
    uint16_t  num_locals;
} gc_root_frame;

#define GC_MAX_CALL_DEPTH 256
extern gc_root_frame gc_call_stack[];
extern int           gc_call_depth;

void gc_mark(object *root);
void gc_sweep(void);
void gc_collect(void);

#endif /* GC_H */
