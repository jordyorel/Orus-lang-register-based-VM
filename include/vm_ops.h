#ifndef VM_OPS_H
#define VM_OPS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "vm.h"
#include "memory.h"

// Helper functions for stack operations
static inline Value vmPeek(VM* vm, int distance) {
    // Safety check to prevent segfaults when trying to peek into an invalid stack position
    if (vm->stackTop - 1 - distance < vm->stack) {
        fprintf(stderr, "Error: Attempted to peek at an invalid stack position\n");
        return NIL_VAL;
    }
    return vm->stackTop[-1 - distance];
}

static inline void vmPush(VM* vm, Value value) {
    // Safety check for stack overflow
    if (vm->stackTop - vm->stack >= STACK_MAX) {
        fprintf(stderr, "Error: Stack overflow\n");
        return;
    }
    *vm->stackTop = value;
    vm->stackTop++;
}

static inline Value vmPop(VM* vm) {
    // Safety check for stack underflow
    if (vm->stackTop <= vm->stack) {
        fprintf(stderr, "Error: Stack underflow\n");
        return NIL_VAL;
    }
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
                vmRuntimeError("Division by zero.");
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
                vmRuntimeError("Division by zero.");
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

static inline Value convertToString(Value value) {
    char buffer[64];
    int length = 0;
    switch (value.type) {
        case VAL_I32:
            length = snprintf(buffer, sizeof(buffer), "%d", AS_I32(value));
            break;
        case VAL_U32:
            length = snprintf(buffer, sizeof(buffer), "%u", AS_U32(value));
            break;
        case VAL_F64:
            length = snprintf(buffer, sizeof(buffer), "%g", AS_F64(value));
            break;
        case VAL_BOOL:
            length = snprintf(buffer, sizeof(buffer), "%s", AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            length = snprintf(buffer, sizeof(buffer), "nil");
            break;
        case VAL_STRING:
            return value;
        default:
            length = snprintf(buffer, sizeof(buffer), "<obj>");
            break;
    }
    ObjString* obj = allocateString(buffer, length);
    return STRING_VAL(obj);
}

static inline void concatOp(VM* vm) {
    Value b = vmPop(vm);
    Value a = vmPop(vm);
    if (!IS_STRING(a)) a = convertToString(a);
    if (!IS_STRING(b)) b = convertToString(b);

    int len = AS_STRING(a)->length + AS_STRING(b)->length;
    char* chars = (char*)malloc(len + 1);
    memcpy(chars, AS_STRING(a)->chars, AS_STRING(a)->length);
    memcpy(chars + AS_STRING(a)->length, AS_STRING(b)->chars, AS_STRING(b)->length);
    chars[len] = '\0';
    ObjString* result = allocateString(chars, len);
    free(chars);
    vmPush(vm, STRING_VAL(result));
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
    // First check if we have two values on the stack
    if (vm->stackTop - vm->stack < 2) {
        // Not enough values on stack, push a default false value
        fprintf(stderr, "Error: Not enough values on stack for comparison\n");
        vmPush(vm, BOOL_VAL(false));
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    
    if (!IS_I32(vmPeek(vm, 0)) || !IS_I32(vmPeek(vm, 1))) {
        // If one of the values isn't an i32, try to safely handle the error
        fprintf(stderr, "Operands must be integers for comparison.\n");
        
        // Pop the values safely (already checked we have 2 values above)
        vmPop(vm);
        vmPop(vm);
        
        // Push a default false value to keep the stack consistent
        vmPush(vm, BOOL_VAL(false));
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    
    // Now safely perform the operation
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
            vmPush(vm, BOOL_VAL(false));
            return;
    }
    
    vmPush(vm, BOOL_VAL(value));
}

// Comparison operations for u32
static inline void compareOpU32(VM* vm, char op, InterpretResult* result) {
    // First check if we have two values on the stack
    if (vm->stackTop - vm->stack < 2) {
        // Not enough values on stack, push a default false value
        fprintf(stderr, "Error: Not enough values on stack for comparison\n");
        vmPush(vm, BOOL_VAL(false));
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    
    if (!IS_U32(vmPeek(vm, 0)) || !IS_U32(vmPeek(vm, 1))) {
        // If one of the values isn't a u32, try to safely handle the error
        fprintf(stderr, "Operands must be unsigned integers for comparison.\n");
        
        // Pop the values safely (already checked we have 2 values above)
        vmPop(vm);
        vmPop(vm);
        
        // Push a default false value to keep the stack consistent
        vmPush(vm, BOOL_VAL(false));
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    
    // Now safely perform the operation
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
            vmPush(vm, BOOL_VAL(false));
            return;
    }
    
    vmPush(vm, BOOL_VAL(value));
}

// Comparison operations for f64
static inline void compareOpF64(VM* vm, char op, InterpretResult* result) {
    // First check if we have two values on the stack
    if (vm->stackTop - vm->stack < 2) {
        // Not enough values on stack, push a default false value
        fprintf(stderr, "Error: Not enough values on stack for comparison\n");
        vmPush(vm, BOOL_VAL(false));
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    
    // Pop both values safely
    Value b_val = vmPop(vm);
    Value a_val = vmPop(vm);

    // Convert both values to f64
    double b = convertToF64(vm, b_val, result);
    if (*result != INTERPRET_OK) {
        vmPush(vm, BOOL_VAL(false)); // Ensure we keep the stack consistent even on error
        return;
    }

    double a = convertToF64(vm, a_val, result);
    if (*result != INTERPRET_OK) {
        vmPush(vm, BOOL_VAL(false)); // Ensure we keep the stack consistent even on error
        return;
    }

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
            vmPush(vm, BOOL_VAL(false));
            return;
    }
    
    vmPush(vm, BOOL_VAL(value));
}

// Compare any two values for equality, regardless of type
static inline void compareOpAny(VM* vm, char op, InterpretResult* result) {
    // First check if we have two values on the stack
    if (vm->stackTop - vm->stack < 2) {
        // Not enough values on stack, push a default false value
        fprintf(stderr, "Error: Not enough values on stack for comparison\n");
        vmPush(vm, BOOL_VAL(false));
        *result = INTERPRET_RUNTIME_ERROR;
        return;
    }
    
    // Pop both values safely
    Value b = vmPop(vm);
    Value a = vmPop(vm);
    
    bool value = false;
    
    // Handle equality based on the value types
    if (op == '=') {
        if (IS_I32(a) && IS_I32(b)) {
            value = AS_I32(a) == AS_I32(b);
        } else if (IS_U32(a) && IS_U32(b)) {
            value = AS_U32(a) == AS_U32(b);
        } else if (IS_F64(a) && IS_F64(b)) {
            value = AS_F64(a) == AS_F64(b);
        } else if (IS_BOOL(a) && IS_BOOL(b)) {
            value = AS_BOOL(a) == AS_BOOL(b);
        } else if (IS_NIL(a) && IS_NIL(b)) {
            value = true;  // Two nil values are always equal
        } else if (IS_STRING(a) && IS_STRING(b)) {
            // For strings, compare the actual string contents instead of pointer equality
            ObjString* strA = AS_STRING(a);
            ObjString* strB = AS_STRING(b);
            
            // First check if lengths match
            if (strA->length == strB->length) {
                // Then compare the actual characters
                value = (memcmp(strA->chars, strB->chars, strA->length) == 0);
            } else {
                value = false; // Different lengths means different strings
            }
        } else {
            // Different types are never equal
            value = false;
        }
    } else if (op == '!') {
        if (IS_I32(a) && IS_I32(b)) {
            value = AS_I32(a) != AS_I32(b);
        } else if (IS_U32(a) && IS_U32(b)) {
            value = AS_U32(a) != AS_U32(b);
        } else if (IS_F64(a) && IS_F64(b)) {
            value = AS_F64(a) != AS_F64(b);
        } else if (IS_BOOL(a) && IS_BOOL(b)) {
            value = AS_BOOL(a) != AS_BOOL(b);
        } else if (IS_NIL(a) && IS_NIL(b)) {
            value = false;  // Two nil values are always equal, so not equal is false
        } else if (IS_STRING(a) && IS_STRING(b)) {
            // For strings, compare the actual string contents instead of pointer equality
            ObjString* strA = AS_STRING(a);
            ObjString* strB = AS_STRING(b);
            
            // First check if lengths match
            if (strA->length == strB->length) {
                // Then compare the actual characters - if they're equal, result is false
                value = (memcmp(strA->chars, strB->chars, strA->length) != 0);
            } else {
                value = true; // Different lengths means different strings
            }
        } else {
            // Different types are never equal, so they're always not equal
            value = true;
        }
    } else {
        fprintf(stderr, "Unknown comparison operator: %c\n", op);
        *result = INTERPRET_RUNTIME_ERROR;
        vmPush(vm, BOOL_VAL(false));
        return;
    }
    
    vmPush(vm, BOOL_VAL(value));
}

// ---- Dynamic array helpers ----
static inline void arrayPush(VM* vm, ObjArray* array, Value value) {
    if (array->length >= array->capacity) {
        int oldCap = array->capacity;
        array->capacity = GROW_CAPACITY(oldCap);
        array->elements = GROW_ARRAY(Value, array->elements, oldCap, array->capacity);
        vm->bytesAllocated += sizeof(Value) * (array->capacity - oldCap);
    }
    array->elements[array->length++] = value;
}

static inline Value arrayPop(ObjArray* array) {
    if (array->length == 0) return NIL_VAL;
    return array->elements[--array->length];
}

#endif // VM_OPS_H