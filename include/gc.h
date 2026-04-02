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

/* TLS variables are defined in src/threads/threads.c.
   The macros below let the rest of the codebase use the old names
   transparently — each thread automatically sees its own copy. */
extern __thread gc_root_frame tls_gc_call_stack[];
extern __thread int           tls_gc_call_depth;

#define gc_call_stack tls_gc_call_stack
#define gc_call_depth tls_gc_call_depth

void gc_mark(object *root);
void gc_sweep(void);
void gc_collect(void);

#endif /* GC_H */
