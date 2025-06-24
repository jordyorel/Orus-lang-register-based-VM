/**
 * @file memory_stub.c
 * @brief Minimal memory management stub for register VM tests
 * 
 * This file provides minimal implementations of memory management functions
 * needed by the register VM tests without full garbage collection.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../include/memory.h"
#include "../../include/value.h"
#include "../../include/error.h"
#include "../../include/ast.h"
#include "../../include/type.h"

// Simple allocation without GC
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, newSize);
}

// Allocate string object
ObjString* allocateString(const char* chars, int length) {
    ObjString* string = malloc(sizeof(ObjString));
    string->length = length;
    string->chars = malloc(length + 1);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    return string;
}

// Allocate array object
ObjArray* allocateArray(int length) {
    ObjArray* array = malloc(sizeof(ObjArray));
    array->length = length;
    array->capacity = length > 0 ? length : 8;
    array->elements = malloc(sizeof(Value) * array->capacity);
    for (int i = 0; i < array->capacity; i++) {
        array->elements[i] = NIL_VAL;
    }
    return array;
}

// Allocate integer array object
ObjIntArray* allocateIntArray(int length) {
    ObjIntArray* array = malloc(sizeof(ObjIntArray));
    array->length = length;
    array->elements = malloc(sizeof(int64_t) * length);
    memset(array->elements, 0, sizeof(int64_t) * length);
    return array;
}

// Allocate range iterator
ObjRangeIterator* allocateRangeIterator(int64_t start, int64_t end) {
    ObjRangeIterator* it = malloc(sizeof(ObjRangeIterator));
    it->current = start;
    it->end = end;
    return it;
}

// Allocate error object
ObjError* allocateError(ErrorType type, const char* message, SrcLocation location) {
    ObjError* err = malloc(sizeof(ObjError));
    err->type = type;
    err->message = allocateString(message, (int)strlen(message));
    err->location = location;
    return err;
}

// Allocate AST node
ASTNode* allocateASTNode() {
    ASTNode* node = malloc(sizeof(ASTNode));
    memset(node, 0, sizeof(ASTNode));
    return node;
}

// Allocate type
Type* allocateType() {
    return malloc(sizeof(Type));
}

// Allocate enum object
ObjEnum* allocateEnum(int variantIndex, Value* data, int dataCount, ObjString* typeName) {
    ObjEnum* enumValue = malloc(sizeof(ObjEnum));
    enumValue->variantIndex = variantIndex;
    enumValue->dataCount = dataCount;
    enumValue->typeName = typeName;
    
    if (dataCount > 0) {
        enumValue->data = malloc(sizeof(Value) * dataCount);
        for (int i = 0; i < dataCount; i++) {
            enumValue->data[i] = data[i];
        }
    } else {
        enumValue->data = NULL;
    }
    
    return enumValue;
}

// Stub GC functions (no-ops for tests)
void markValue(Value value) {
    // No-op for tests
}

void markObject(Obj* object) {
    // No-op for tests
}

void collectGarbage() {
    // No-op for tests
}

void freeObjects() {
    // No-op for tests
}

void pauseGC() {
    // No-op for tests
}

void resumeGC() {
    // No-op for tests
}


char* copyString(const char* chars, int length) {
    char* copy = malloc(length + 1);
    memcpy(copy, chars, length);
    copy[length] = '\0';
    return copy;
}
