#include <string.h>
#include "../../include/threads.h"

/* ── TLS definitions — one copy per pthread ─────────────────────────────── */
__thread gc_root_frame tls_gc_call_stack[GC_MAX_CALL_DEPTH];
__thread int           tls_gc_call_depth = 0;

/* ── Global thread registry ─────────────────────────────────────────────── */
jvm_thread      thread_registry[MAX_JVM_THREADS];
pthread_mutex_t thread_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

void threads_init(void) {
    memset(thread_registry, 0, sizeof(thread_registry));
    threads_register_self(); /* register the main thread */
}

void threads_register_self(void) {
    pthread_mutex_lock(&thread_registry_mutex);
    for (int i = 0; i < MAX_JVM_THREADS; i++) {
        if (!thread_registry[i].active) {
            thread_registry[i].tid        = pthread_self();
            thread_registry[i].call_stack = tls_gc_call_stack;
            thread_registry[i].call_depth = &tls_gc_call_depth;
            thread_registry[i].active     = 1;
            break;
        }
    }
    pthread_mutex_unlock(&thread_registry_mutex);
}

void threads_unregister_self(void) {
    pthread_t self = pthread_self();
    pthread_mutex_lock(&thread_registry_mutex);
    for (int i = 0; i < MAX_JVM_THREADS; i++) {
        if (thread_registry[i].active && pthread_equal(thread_registry[i].tid, self)) {
            thread_registry[i].active = 0;
            break;
        }
    }
    pthread_mutex_unlock(&thread_registry_mutex);
}
