#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

// Adds a value to the end of the array, growing the capacity if necessary.
// Parameters:
//   array - Pointer to the ValueArray where the value will be written.
//   value - The value to add to the array.
void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values,
                                 oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

// Frees the memory associated with a ValueArray and resets it to its default state.
void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
    printf("[");
    switch (value.type) {
        case VAL_I32:
            printf("I32:%d", value.as.i32);
            break;
        case VAL_U32:
            printf("U32:%u", value.as.u32);
            break;
        case VAL_F64:
            printf("F64:%g", value.as.f64);
            break;
        case VAL_BOOL:
            printf("BOOL:%s", value.as.boolean ? "true" : "false");
            break;
        case VAL_NIL:
            printf("NIL");
            break;
        case VAL_STRING:
            if (value.as.string.chars != NULL) {
                printf("STR:%.*s", value.as.string.length, value.as.string.chars);
            } else {
                printf("STR:(null)");
            }
            break;
        default:
            printf("UNKNOWN_TYPE:%d", value.type);
            break;
    }
    printf("]");
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    
    switch (a.type) {
        case VAL_I32: return a.as.i32 == b.as.i32;
        case VAL_U32: return a.as.u32 == b.as.u32;
        case VAL_F64: return a.as.f64 == b.as.f64;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_NIL: return true;
        case VAL_STRING:
            return a.as.string.length == b.as.string.length &&
                   memcmp(a.as.string.chars, b.as.string.chars, a.as.string.length) == 0;
        default: return false;
    }
}
