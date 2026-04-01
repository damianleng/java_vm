#include <stdio.h>
#include "../../include/heap.h"

/* Minimal stub for System.out.println(int) */
void runtime_println_int(int32_t value) {
    printf("%d\n", value);
}

/* Minimal stub for System.out.println(String) — placeholder */
void runtime_println_str(const char *value) {
    printf("%s\n", value ? value : "null");
}
