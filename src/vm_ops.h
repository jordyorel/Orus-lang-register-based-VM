#ifndef VM_OPS_H
#define VM_OPS_H

#include <stdio.h>
#include "value.h"
#include "vm.h"

// Helper functions for stack operations
static inline Value vmPeek(VM* vm, int distance) {
    return vm->stackTop[-1 - distance];
}

static inline void vmPush(VM* vm, Value value) {
    *vm->stackTop = value;
    vm->stackTop++;
}

static inline Value vmPop(VM* vm) {
    vm->stackTop--;
    return *vm->stackTop;
}

// Binary operations for i32
static inline void binaryOpI32(VM* vm, char op, InterpretResult* result) {
    if (!IS_I32(vmPeek(vm, 0)) || !IS_I32(vmPeek(vm, 1))) {
        fprintf(stderr, "Operands must be integers.\n");
        fprintf(stderr, "Top of stack: %d, Next: %d\n", (int)(vm->stackTop - vm->stack), (int)(vm->stackTop - vm->stack - 1));
        fprintf(stderr, "Values: ");
        printValue(vmPeek(vm, 0));
        fprintf(stderr, " and ");
        printValue(vmPeek(vm, 1));
        fprintf(stderr, "\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    int32_t b = AS_I32(vmPop(vm));
    int32_t a = AS_I32(vmPop(vm));
    switch (op) {
        case '+': vmPush(vm, I32_VAL(a + b)); break;
        case '-': vmPush(vm, I32_VAL(a - b)); break;
        case '*': vmPush(vm, I32_VAL(a * b)); break;
        case '/':
            if (b == 0) {
                fprintf(stderr, "Division by zero.\n");
                *result = INTERPRET_RUNTIME_ERROR;
                return;
            }
            vmPush(vm, I32_VAL(a / b));
            break;
        default:
            fprintf(stderr, "Unknown operator: %c\n", op);
            *result = INTERPRET_RUNTIME_ERROR;
    }
}

// Binary operations for u32
static inline void binaryOpU32(VM* vm, char op, InterpretResult* result) {
    if (!IS_U32(vmPeek(vm, 0)) || !IS_U32(vmPeek(vm, 1))) {
        fprintf(stderr, "Operands must be unsigned integers.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    uint32_t b = AS_U32(vmPop(vm));
    uint32_t a = AS_U32(vmPop(vm));
    switch (op) {
        case '+': vmPush(vm, U32_VAL(a + b)); break;
        case '-': vmPush(vm, U32_VAL(a - b)); break;
        case '*': vmPush(vm, U32_VAL(a * b)); break;
        case '/':
            if (b == 0) {
                fprintf(stderr, "Division by zero.\n");
                *result = INTERPRET_RUNTIME_ERROR;
                return;
            }
            vmPush(vm, U32_VAL(a / b));
            break;
        default:
            fprintf(stderr, "Unknown operator: %c\n", op);
            *result = INTERPRET_RUNTIME_ERROR;
    }
}

// Convert a value to f64 if needed
static inline double convertToF64(VM* vm, Value value, InterpretResult* result) {
    if (IS_F64(value)) {
        return AS_F64(value);
    } else if (IS_I32(value)) {
        return (double)AS_I32(value);
    } else if (IS_U32(value)) {
        return (double)AS_U32(value);
    } else {
        fprintf(stderr, "Cannot convert value to float.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return 0.0;
    }
}

// Binary operations for f64 with automatic type conversion
static inline void binaryOpF64(VM* vm, char op, InterpretResult* result) {
    // Pop both values first
    Value b_val = vmPop(vm);
    Value a_val = vmPop(vm);

    // Convert both values to f64
    double b = convertToF64(vm, b_val, result);
    if (*result != INTERPRET_OK) return;

    double a = convertToF64(vm, a_val, result);
    if (*result != INTERPRET_OK) return;

    // Perform the operation
    switch (op) {
        case '+': vmPush(vm, F64_VAL(a + b)); break;
        case '-': vmPush(vm, F64_VAL(a - b)); break;
        case '*': vmPush(vm, F64_VAL(a * b)); break;
        case '/':
            if (b == 0.0) {
                fprintf(stderr, "Division by zero.\n");
                *result = INTERPRET_RUNTIME_ERROR;
                return;
            }
            vmPush(vm, F64_VAL(a / b));
            break;
        default:
            fprintf(stderr, "Unknown operator: %c\n", op);
            *result = INTERPRET_RUNTIME_ERROR;
    }
}

// Modulo operation for i32
static inline void moduloOpI32(VM* vm, InterpretResult* result) {
    if (!IS_I32(vmPeek(vm, 0)) || !IS_I32(vmPeek(vm, 1))) {
        fprintf(stderr, "Operands must be integers.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    int32_t b = AS_I32(vmPop(vm));
    int32_t a = AS_I32(vmPop(vm));
    if (b == 0) {
        fprintf(stderr, "Modulo by zero.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    vmPush(vm, I32_VAL(a % b));
}

// Modulo operation for u32
static inline void moduloOpU32(VM* vm, InterpretResult* result) {
    if (!IS_U32(vmPeek(vm, 0)) || !IS_U32(vmPeek(vm, 1))) {
        fprintf(stderr, "Operands must be unsigned integers.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    uint32_t b = AS_U32(vmPop(vm));
    uint32_t a = AS_U32(vmPop(vm));
    if (b == 0) {
        fprintf(stderr, "Modulo by zero.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    vmPush(vm, U32_VAL(a % b));
}

// Comparison operations for i32
static inline void compareOpI32(VM* vm, char op, InterpretResult* result) {
    if (!IS_I32(vmPeek(vm, 0)) || !IS_I32(vmPeek(vm, 1))) {
        fprintf(stderr, "Operands must be integers.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    int32_t b = AS_I32(vmPop(vm));
    int32_t a = AS_I32(vmPop(vm));
    bool value = false;
    switch (op) {
        case '<': value = a < b; break;
        case '>': value = a > b; break;
        case 'L': value = a <= b; break; // 'L' for Less or Equal
        case 'G': value = a >= b; break; // 'G' for Greater or Equal
        case '=': value = a == b; break;
        case '!': value = a != b; break;
        default:
            fprintf(stderr, "Unknown comparison operator: %c\n", op);
            *result = INTERPRET_RUNTIME_ERROR;
            return;
    }
    vmPush(vm, BOOL_VAL(value));
}

// Comparison operations for u32
static inline void compareOpU32(VM* vm, char op, InterpretResult* result) {
    if (!IS_U32(vmPeek(vm, 0)) || !IS_U32(vmPeek(vm, 1))) {
        fprintf(stderr, "Operands must be unsigned integers.\n");
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    uint32_t b = AS_U32(vmPop(vm));
    uint32_t a = AS_U32(vmPop(vm));
    bool value = false;
    switch (op) {
        case '<': value = a < b; break;
        case '>': value = a > b; break;
        case 'L': value = a <= b; break; // 'L' for Less or Equal
        case 'G': value = a >= b; break; // 'G' for Greater or Equal
        case '=': value = a == b; break;
        case '!': value = a != b; break;
        default:
            fprintf(stderr, "Unknown comparison operator: %c\n", op);
            *result = INTERPRET_RUNTIME_ERROR;
            return;
    }
    vmPush(vm, BOOL_VAL(value));
}

// Comparison operations for f64
static inline void compareOpF64(VM* vm, char op, InterpretResult* result) {
    // Pop both values first
    Value b_val = vmPop(vm);
    Value a_val = vmPop(vm);

    // Convert both values to f64
    double b = convertToF64(vm, b_val, result);
    if (*result != INTERPRET_OK) return;

    double a = convertToF64(vm, a_val, result);
    if (*result != INTERPRET_OK) return;

    bool value = false;
    switch (op) {
        case '<': value = a < b; break;
        case '>': value = a > b; break;
        case 'L': value = a <= b; break; // 'L' for Less or Equal
        case 'G': value = a >= b; break; // 'G' for Greater or Equal
        case '=': value = a == b; break;
        case '!': value = a != b; break;
        default:
            fprintf(stderr, "Unknown comparison operator: %c\n", op);
            *result = INTERPRET_RUNTIME_ERROR;
            return;
    }
    vmPush(vm, BOOL_VAL(value));
}

#endif // VM_OPS_H