#include <stdio.h>
#include <stdlib.h>
#include "classfile.h"
#include "interpreter.h"
#include "heap.h"
#include "gc.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: jvm <classfile>\n");
        return 1;
    }

    /* TODO: initialize heap, load class, run main method */
    (void)argv;
    return 0;
}
