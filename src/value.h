#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include <string.h>

typedef enum {
    VAL_I32,
    VAL_U32,
    VAL_F64,
    VAL_BOOL,
    VAL_NIL,
    VAL_STRING,
} ValueType;

typedef struct {
    char* chars;
    int length;
} String;

typedef struct {
    ValueType type;
    union {
        int32_t i32;
        uint32_t u32;
        double f64;
        bool boolean;
        String string;
    } as;
} Value;

// Value creation macros
#define I32_VAL(value)   ((Value){VAL_I32, {.i32 = value}})
#define U32_VAL(value)   ((Value){VAL_U32, {.u32 = value}})
#define F64_VAL(value)   ((Value){VAL_F64, {.f64 = value}})
#define BOOL_VAL(value)  ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL          ((Value){VAL_NIL, {.i32 = 0}})
#define STRING_VAL(chars, len) ((Value){VAL_STRING, {.string = (String){chars, len}}})

// Value checking macros
#define IS_I32(value)    ((value).type == VAL_I32)
#define IS_U32(value)    ((value).type == VAL_U32)
#define IS_F64(value)    ((value).type == VAL_F64)
#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_STRING(value) ((value).type == VAL_STRING)

// Value extraction macros
#define AS_I32(value)    ((value).as.i32)
#define AS_U32(value)    ((value).as.u32)
#define AS_F64(value)    ((value).as.f64)
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_STRING(value) ((value).as.string)

// A dynamic array of Value elements.
// Parameters:
//   capacity - The number of elements that the array can hold before it needs to grow.
//   count - The number of elements currently in the array.
//   values - A pointer to the first element in the array.
typedef struct {
    int capacity; // 4 bytes
    int count; // 4 bytes
    Value* values; // 8 bytes
} ValueArray; // 16 bytes

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);
bool valuesEqual(Value a, Value b);

#endif