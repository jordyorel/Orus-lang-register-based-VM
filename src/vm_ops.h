#ifndef VM_OPS_H
#define VM_OPS_H

#include "value.h"
#include "vm.h"

// Forward declarations
static void runtimeError(const char* format, ...);
static Value peek(int distance);
void push(Value value);
Value pop(void);

// Binary operations for i32
static inline void binaryOpI32(VM* vm, char op, InterpretResult* result) {
    if (!IS_I32(peek(0)) || !IS_I32(peek(1))) {
        runtimeError("Operands must be integers.");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    int32_t b = AS_I32(pop());
    int32_t a = AS_I32(pop());
    switch (op) {
        case '+': push(I32_VAL(a + b)); break;
        case '-': push(I32_VAL(a - b)); break;
        case '*': push(I32_VAL(a * b)); break;
        case '/': push(I32_VAL(a / b)); break;
        default: runtimeError("Unknown operator: %c", op); *result = INTERPRET_RUNTIME_ERROR;
    }
}

// Binary operations for u32
static inline void binaryOpU32(VM* vm, char op, InterpretResult* result) {
    if (!IS_U32(peek(0)) || !IS_U32(peek(1))) {
        runtimeError("Operands must be unsigned integers.");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    uint32_t b = AS_U32(pop());
    uint32_t a = AS_U32(pop());
    switch (op) {
        case '+': push(U32_VAL(a + b)); break;
        case '-': push(U32_VAL(a - b)); break;
        case '*': push(U32_VAL(a * b)); break;
        case '/': push(U32_VAL(a / b)); break;
        default: runtimeError("Unknown operator: %c", op); *result = INTERPRET_RUNTIME_ERROR;
    }
}

// Binary operations for f64
static inline void binaryOpF64(VM* vm, char op, InterpretResult* result) {
    if (!IS_F64(peek(0)) || !IS_F64(peek(1))) {
        runtimeError("Operands must be floating point numbers.");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    double b = AS_F64(pop());
    double a = AS_F64(pop());
    switch (op) {
        case '+': push(F64_VAL(a + b)); break;
        case '-': push(F64_VAL(a - b)); break;
        case '*': push(F64_VAL(a * b)); break;
        case '/': push(F64_VAL(a / b)); break;
        default: runtimeError("Unknown operator: %c", op); *result = INTERPRET_RUNTIME_ERROR;
    }
}

// Modulo operation for i32
static inline void moduloOpI32(VM* vm, InterpretResult* result) {
    if (!IS_I32(peek(0)) || !IS_I32(peek(1))) {
        runtimeError("Operands must be integers.");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    int32_t b = AS_I32(pop());
    int32_t a = AS_I32(pop());
    if (b == 0) {
        runtimeError("Modulo by zero.");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    push(I32_VAL(a % b));
}

// Modulo operation for u32
static inline void moduloOpU32(VM* vm, InterpretResult* result) {
    if (!IS_U32(peek(0)) || !IS_U32(peek(1))) {
        runtimeError("Operands must be unsigned integers.");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    uint32_t b = AS_U32(pop());
    uint32_t a = AS_U32(pop());
    if (b == 0) {
        runtimeError("Modulo by zero.");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    push(U32_VAL(a % b));
}

#endif // VM_OPS_H