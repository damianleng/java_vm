#include <stdio.h>
#include <stdlib.h>
#include "classfile.h"
#include "interpreter.h"
#include "heap.h"
#include "gc.h"
#include "threads.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: jvm <classfile>\n");
        return 1;
    }

    heap_init();
    threads_init();

    class_file *cf = classfile_load(argv[1]);
    if (!cf) {
        heap_destroy();
        return 1;
    }

    method_info *main_method = classfile_find_method(cf, "main", NULL);
    if (!main_method) {
        fprintf(stderr, "Error: no main method found in %s\n", argv[1]);
        classfile_free(cf);
        heap_destroy();
        return 1;
    }

    interpreter_execute(cf, main_method, NULL, 0); /* main takes no primitive args */

    classfile_free(cf);
    threads_unregister_self();
    heap_destroy();
    return 0;
}