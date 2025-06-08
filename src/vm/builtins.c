#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/builtins.h"
#include "../../include/error.h"
#include "../../include/memory.h"
#include "../../include/vm_ops.h"
#include "../../include/type.h"

extern VM vm;


static Value native_len(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("len() takes exactly one argument.");
        return NIL_VAL;
    }
    Value val = args[0];
    if (IS_ARRAY(val)) {
        return I32_VAL(AS_ARRAY(val)->length);
    } else if (IS_STRING(val)) {
        return I32_VAL(AS_STRING(val)->length);
    }
    vmRuntimeError("len() expects array or string.");
    return NIL_VAL;
}

static Value native_substring(int argCount, Value* args) {
    if (argCount != 3) {
        vmRuntimeError("substring() takes exactly three arguments.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0]) || !IS_I32(args[1]) || !IS_I32(args[2])) {
        vmRuntimeError("substring() expects (string, i32, i32).");
        return NIL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int start = AS_I32(args[1]);
    int length = AS_I32(args[2]);
    if (start < 0) start = 0;
    if (start > str->length) start = str->length;
    if (length < 0) length = 0;
    if (start + length > str->length) length = str->length - start;
    ObjString* result = allocateString(str->chars + start, length);
    return STRING_VAL(result);
}

static Value native_push(int argCount, Value* args) {
    if (argCount != 2) {
        vmRuntimeError("push() takes exactly two arguments.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("First argument to push() must be array.");
        return NIL_VAL;
    }
    ObjArray* arr = AS_ARRAY(args[0]);
    arrayPush(&vm, arr, args[1]);
    return args[0];
}

static Value native_pop(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("pop() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_ARRAY(args[0])) {
        vmRuntimeError("pop() expects array.");
        return NIL_VAL;
    }
    ObjArray* arr = AS_ARRAY(args[0]);
    return arrayPop(arr);
}

static Value native_range(int argCount, Value* args) {
    if (argCount != 2) {
        vmRuntimeError("range() takes exactly two arguments.");
        return NIL_VAL;
    }
    if (!(IS_I32(args[0]) || IS_U32(args[0])) ||
        !(IS_I32(args[1]) || IS_U32(args[1]))) {
        vmRuntimeError("range() expects (i32/u32, i32/u32).");
        return NIL_VAL;
    }
    int start = IS_I32(args[0]) ? AS_I32(args[0]) : (int)AS_U32(args[0]);
    int end = IS_I32(args[1]) ? AS_I32(args[1]) : (int)AS_U32(args[1]);
    if (end < start) {
        ObjArray* arr = allocateArray(0);
        arr->length = 0;
        return ARRAY_VAL(arr);
    }
    int len = end - start;
    ObjArray* arr = allocateArray(len);
    arr->length = len;
    for (int i = 0; i < len; i++) {
        arr->elements[i] = I32_VAL(start + i);
    }
    return ARRAY_VAL(arr);
}

static const char* getValueTypeName(Value val) {
    switch (val.type) {
        case VAL_I32:   return "i32";
        case VAL_U32:   return "u32";
        case VAL_F64:   return "f64";
        case VAL_BOOL:  return "bool";
        case VAL_NIL:   return "nil";
        case VAL_STRING:return "string";
        case VAL_ARRAY: return "array";
        case VAL_ERROR: return "error";
        default:        return "unknown";
    }
}

static Value native_type_of(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("type_of() takes exactly one argument.");
        return NIL_VAL;
    }
    const char* name = getValueTypeName(args[0]);
    ObjString* result = allocateString(name, (int)strlen(name));
    return STRING_VAL(result);
}

static Value native_is_type(int argCount, Value* args) {
    if (argCount != 2) {
        vmRuntimeError("is_type() takes exactly two arguments.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[1])) {
        vmRuntimeError("Second argument to is_type() must be a string.");
        return NIL_VAL;
    }
    const char* name = getValueTypeName(args[0]);
    ObjString* query = AS_STRING(args[1]);
    bool result = query->length == (int)strlen(name) &&
                  strncmp(query->chars, name, query->length) == 0;
    return BOOL_VAL(result);
}

static Value native_input(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("input() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0])) {
        vmRuntimeError("input() argument must be a string.");
        return NIL_VAL;
    }
    ObjString* prompt = AS_STRING(args[0]);
    printf("%s", prompt->chars);
    fflush(stdout);
    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return STRING_VAL(allocateString("", 0));
    }
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[--len] = '\0';
    }
    ObjString* result = allocateString(buffer, (int)len);
    return STRING_VAL(result);
}

static Value native_int(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("int() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0])) {
        vmRuntimeError("int() argument must be a string.");
        return NIL_VAL;
    }
    char* end;
    const char* text = AS_STRING(args[0])->chars;
    long value = strtol(text, &end, 10);
    if (*end != '\0') {
        vmRuntimeError("invalid integer literal.");
        return NIL_VAL;
    }
    if (value < INT32_MIN || value > INT32_MAX) {
        vmRuntimeError("integer value out of range.");
        return NIL_VAL;
    }
    return I32_VAL((int32_t)value);
}

static Value native_float(int argCount, Value* args) {
    if (argCount != 1) {
        vmRuntimeError("float() takes exactly one argument.");
        return NIL_VAL;
    }
    if (!IS_STRING(args[0])) {
        vmRuntimeError("float() argument must be a string.");
        return NIL_VAL;
    }
    char* end;
    const char* text = AS_STRING(args[0])->chars;
    double value = strtod(text, &end);
    if (*end != '\0') {
        vmRuntimeError("invalid float literal.");
        return NIL_VAL;
    }
    return F64_VAL(value);
}

typedef struct {
    const char* name;
    NativeFn fn;
    int arity;
    TypeKind returnKind; // TYPE_COUNT indicates no specific return type
} BuiltinEntry;

static BuiltinEntry builtinTable[] = {
    {"len", native_len, 1, TYPE_I32},
    {"substring", native_substring, 3, TYPE_STRING},
    {"push", native_push, 2, TYPE_COUNT},
    {"pop", native_pop, 1, TYPE_COUNT},
    {"range", native_range, 2, TYPE_COUNT},
    {"type_of", native_type_of, 1, TYPE_STRING},
    {"is_type", native_is_type, 2, TYPE_BOOL},
    {"input", native_input, 1, TYPE_STRING},
    {"int", native_int, 1, TYPE_I32},
    {"float", native_float, 1, TYPE_F64},
};

void initBuiltins(void) {
    size_t count = sizeof(builtinTable) / sizeof(BuiltinEntry);
    for (size_t i = 0; i < count; i++) {
        Type* ret = builtinTable[i].returnKind == TYPE_COUNT
                         ? NULL
                         : getPrimitiveType(builtinTable[i].returnKind);
        defineNative(builtinTable[i].name,
                     builtinTable[i].fn,
                     builtinTable[i].arity,
                     ret);
    }
}

