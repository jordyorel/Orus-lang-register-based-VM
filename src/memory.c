#include <stdlib.h>
#include <string.h>

#include "memory.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

char* copyString(const char* chars, int length) {
    char* copy = (char*)malloc(length + 1);
    memcpy(copy, chars, length);
    copy[length] = '\0';
    return copy;
}