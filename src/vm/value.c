#include <stdio.h>
#include <string.h>

#include "../../include/memory.h"
#include "../../include/value.h"


void printValue(Value value) {
    switch (value.type) {
        case VAL_I32:
            printf("%d", AS_I32(value));
            break;
        case VAL_U32:
            printf("%u", AS_U32(value));
            break;
        case VAL_F64:
            printf("%g", AS_F64(value));
            break;
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_STRING:
            printf("%s", AS_STRING(value)->chars);
            break;
        case VAL_ARRAY: {
            printf("[");
            ObjArray* arr = AS_ARRAY(value);
            for (int i = 0; i < arr->length; i++) {
                printValue(arr->elements[i]);
                if (i < arr->length - 1) printf(", ");
            }
            printf("]");
            break;
        }
        case VAL_ERROR: {
            printf("Error(%d): %s", AS_ERROR(value)->type,
                   AS_ERROR(value)->message->chars);
            break;
        }
        default:
            printf("unknown");
            break;
    }
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
            return a.as.string->length == b.as.string->length &&
                   memcmp(a.as.string->chars, b.as.string->chars, a.as.string->length) == 0;
        case VAL_ARRAY: {
            if (a.as.array->length != b.as.array->length) return false;
            for (int i = 0; i < a.as.array->length; i++) {
                if (!valuesEqual(a.as.array->elements[i], b.as.array->elements[i])) return false;
            }
            return true;
        }
        case VAL_ERROR:
            return a.as.error == b.as.error;
        default: return false;
    }
}
