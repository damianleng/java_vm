#ifndef GC_H
#define GC_H

#include "heap.h"

void gc_mark(object *root);
void gc_sweep(void);
void gc_collect(void); /* mark + sweep */

#endif /* GC_H */
