#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "memory.h"

// Define the array of primitive types
Type* primitiveTypes[TYPE_COUNT] = {NULL};
static bool typeSystemInitialized = false;

void initTypeSystem(void) {
    if (typeSystemInitialized) return;
    
    for (int i = 0; i < TYPE_COUNT; i++) {
        primitiveTypes[i] = NULL;
    }
    
    primitiveTypes[TYPE_I32] = createPrimitiveType(TYPE_I32);
    primitiveTypes[TYPE_U32] = createPrimitiveType(TYPE_U32);
    primitiveTypes[TYPE_F64] = createPrimitiveType(TYPE_F64);
    primitiveTypes[TYPE_BOOL] = createPrimitiveType(TYPE_BOOL);
    primitiveTypes[TYPE_STRING] = createPrimitiveType(TYPE_STRING);
    primitiveTypes[TYPE_NIL] = createPrimitiveType(TYPE_NIL);
    
    typeSystemInitialized = true;
}

// Add this to help with debugging
Type* getPrimitiveType(TypeKind kind) {
    if (kind < 0 || kind >= TYPE_COUNT) return NULL;
    return primitiveTypes[kind];
}

Type* createPrimitiveType(TypeKind kind) {
    Type* type = (Type*)malloc(sizeof(Type));
    type->kind = kind;
    return type;
}

Type* createArrayType(Type* elementType) {
    Type* type = (Type*)malloc(sizeof(Type));
    type->kind = TYPE_ARRAY;
    type->info.array.elementType = elementType;
    return type;
}

Type* createFunctionType(Type* returnType, Type** paramTypes, int paramCount) {
    Type* type = (Type*)malloc(sizeof(Type));
    type->kind = TYPE_FUNCTION;
    type->info.function.returnType = returnType;
    type->info.function.paramTypes = paramTypes;
    type->info.function.paramCount = paramCount;
    return type;
}

void freeType(Type* type) {
    if (type == NULL) return;
    
    switch (type->kind) {
        case TYPE_ARRAY:
            freeType(type->info.array.elementType);
            break;
        case TYPE_FUNCTION:
            freeType(type->info.function.returnType);
            for (int i = 0; i < type->info.function.paramCount; i++) {
                freeType(type->info.function.paramTypes[i]);
            }
            free(type->info.function.paramTypes);
            break;
        default:
            break;
    }
    
    free(type);
}

void freeTypeSystem(void) {
    if (!typeSystemInitialized) return;
    for (int i = 0; i < TYPE_COUNT; i++) {
        if (primitiveTypes[i] != NULL) {
            freeType(primitiveTypes[i]);
            primitiveTypes[i] = NULL;
        }
    }
    typeSystemInitialized = false;
}

bool typesEqual(Type* a, Type* b) {
    if (a == NULL || b == NULL) return false;
    if (a == b) return true;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case TYPE_I32:
        case TYPE_U32:
        case TYPE_F64:
        case TYPE_BOOL:
        case TYPE_STRING:
        case TYPE_NIL:
            return true;  // Same kind means same type for primitives
            
        case TYPE_ARRAY:
            return typesEqual(a->info.array.elementType,
                            b->info.array.elementType);
            
        case TYPE_FUNCTION: {
            if (!typesEqual(a->info.function.returnType,
                           b->info.function.returnType)) {
                return false;
            }
            if (a->info.function.paramCount != b->info.function.paramCount) {
                return false;
            }
            for (int i = 0; i < a->info.function.paramCount; i++) {
                if (!typesEqual(a->info.function.paramTypes[i],
                               b->info.function.paramTypes[i])) {
                    return false;
                }
            }
            return true;
        }
            
        default:
            return false;
    }
}

const char* getTypeName(TypeKind kind) {
    switch (kind) {
        case TYPE_I32: return "i32";
        case TYPE_U32: return "u32";
        case TYPE_F64: return "f64";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_NIL: return "nil";
        case TYPE_ARRAY: return "array";
        case TYPE_FUNCTION: return "function";
        default: return "unknown";
    }
}
