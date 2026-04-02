#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>
#include "gc.h"  /* gc_root_frame, GC_MAX_CALL_DEPTH */

/* ── Thread-local GC root storage ───────────────────────────────────────────
   Each pthread gets its own call stack array.  The macros in gc.h redirect
   gc_call_stack / gc_call_depth to these TLS variables so interpreter.c
   needs no changes for the GC registration logic.                           */
extern __thread gc_root_frame tls_gc_call_stack[GC_MAX_CALL_DEPTH];
extern __thread int           tls_gc_call_depth;

/* ── Thread registry ─────────────────────────────────────────────────────────
   Every live JVM thread registers itself here so gc_collect() can walk all
   active frames, not just those on the calling thread's stack.              */
#define MAX_JVM_THREADS 64

typedef struct {
    pthread_t       tid;
    gc_root_frame  *call_stack;  /* points at this thread's tls_gc_call_stack */
    int            *call_depth;  /* points at this thread's tls_gc_call_depth */
    int             active;
} jvm_thread;

extern jvm_thread      thread_registry[MAX_JVM_THREADS];
extern pthread_mutex_t thread_registry_mutex;

void threads_init(void);           /* call once from main before any threads   */
void threads_register_self(void);  /* call at thread start (main does it too)  */
void threads_unregister_self(void);/* call at thread exit                       */

#endif /* THREADS_H */
