#ifndef clox_value_h
#define clox_value_h

#include "common.h"
#include <string.h>

// Forward declarations used for object references
typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjArray ObjArray;
typedef struct ObjIntArray ObjIntArray;
typedef struct Value Value;

// Base object type for the garbage collector
typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_INT_ARRAY,
    OBJ_AST,
    OBJ_TYPE,
} ObjType;

struct Obj {
    ObjType type;
    bool marked;
    Obj* next;
};

typedef enum {
    VAL_I32,
    VAL_U32,
    VAL_F64,
    VAL_BOOL,
    VAL_NIL,
    VAL_STRING,
    VAL_ARRAY,
} ValueType;

typedef struct ObjString {
    Obj obj;
    int length;
    char* chars;
} ObjString;

typedef struct ObjArray {
    Obj obj;
    int length;
    int capacity;
    Value* elements;
} ObjArray;

typedef struct ObjIntArray {
    Obj obj;
    int length;
    int* elements;
} ObjIntArray;

typedef ObjString String;
typedef ObjArray Array;

typedef struct Value {
    ValueType type;
    union {
        int32_t i32;
        uint32_t u32;
        double f64;
        bool boolean;
        ObjString* string;
        ObjArray* array;
    } as;
} Value;

// Value creation macros
#define I32_VAL(value)   ((Value){VAL_I32, {.i32 = value}})
#define U32_VAL(value)   ((Value){VAL_U32, {.u32 = value}})
#define F64_VAL(value)   ((Value){VAL_F64, {.f64 = value}})
#define BOOL_VAL(value)  ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL          ((Value){VAL_NIL, {.i32 = 0}})
#define STRING_VAL(obj) ((Value){VAL_STRING, {.string = obj}})
#define ARRAY_VAL(obj)   ((Value){VAL_ARRAY, {.array = obj}})

// Value checking macros
#define IS_I32(value)    ((value).type == VAL_I32)
#define IS_U32(value)    ((value).type == VAL_U32)
#define IS_F64(value)    ((value).type == VAL_F64)
#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_STRING(value) ((value).type == VAL_STRING)
#define IS_ARRAY(value)  ((value).type == VAL_ARRAY)

// Value extraction macros
#define AS_I32(value)    ((value).as.i32)
#define AS_U32(value)    ((value).as.u32)
#define AS_F64(value)    ((value).as.f64)
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_STRING(value) ((value).as.string)
#define AS_ARRAY(value)  ((value).as.array)

// Generic dynamic array implementation used for storing Values.
#include "generic_array.h"

DEFINE_ARRAY_TYPE(Value, Value)
void printValue(Value value);
bool valuesEqual(Value a, Value b);

#endif
