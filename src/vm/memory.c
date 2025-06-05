#include <stdlib.h>
#include <string.h>

#include "../../include/memory.h"
#include "../../include/vm.h"

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

extern VM vm;

static size_t gcThreshold = 1024 * 1024;

static void* allocateObject(size_t size, ObjType type) {
    vm.bytesAllocated += size;
    if (vm.bytesAllocated > gcThreshold) {
        collectGarbage();
        gcThreshold = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
    }

    Obj* object = (Obj*)malloc(size);
    object->type = type;
    object->marked = false;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

ObjString* allocateString(const char* chars, int length) {
    ObjString* string = (ObjString*)allocateObject(sizeof(ObjString), OBJ_STRING);
    string->length = length;
    vm.bytesAllocated += length + 1;
    string->chars = (char*)malloc(length + 1);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    return string;
}

ObjArray* allocateArray(int length) {
    ObjArray* array = (ObjArray*)allocateObject(sizeof(ObjArray), OBJ_ARRAY);
    array->length = length;
    vm.bytesAllocated += sizeof(Value) * length;
    array->elements = (Value*)malloc(sizeof(Value) * length);
    return array;
}

static void markObject(Obj* object);
static void freeObject(Obj* object);

static void markValue(Value value) {
    if (IS_STRING(value)) {
        markObject((Obj*)AS_STRING(value));
    } else if (IS_ARRAY(value)) {
        markObject((Obj*)AS_ARRAY(value));
    }
}

static void markObject(Obj* object) {
    if (object == NULL || object->marked) return;
    object->marked = true;

    switch (object->type) {
        case OBJ_STRING:
            // Strings do not reference other objects
            break;
        case OBJ_ARRAY: {
            ObjArray* array = (ObjArray*)object;
            for (int i = 0; i < array->length; i++) {
                markValue(array->elements[i]);
            }
            break;
        }
    }
}

void collectGarbage() {
    // Mark roots
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }
    for (int i = 0; i < vm.variableCount; i++) {
        markValue(vm.globals[i]);
    }

    // Sweep
    Obj** object = &vm.objects;
    while (*object) {
        if (!(*object)->marked) {
            Obj* unreached = *object;
            *object = unreached->next;
            freeObject(unreached);
        } else {
            (*object)->marked = false;
            object = &(*object)->next;
        }
    }
}

static void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            vm.bytesAllocated -= sizeof(ObjString) + string->length + 1;
            free(string->chars);
            free(string);
            break;
        }
        case OBJ_ARRAY: {
            ObjArray* array = (ObjArray*)object;
            vm.bytesAllocated -= sizeof(ObjArray) + sizeof(Value) * array->length;
            FREE_ARRAY(Value, array->elements, array->length);
            free(array);
            break;
        }
    }
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}

char* copyString(const char* chars, int length) {
    char* copy = (char*)malloc(length + 1);
    memcpy(copy, chars, length);
    copy[length] = '\0';
    return copy;
}
