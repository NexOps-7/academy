#include <stdlib.h>
#include "mem.h"

// if oldSize == 0, equals to malloc()
void* ralloc(void* ptr, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }
    void* res = realloc(ptr, newSize);
    if (res == NULL) exit(1);
    return res;
}