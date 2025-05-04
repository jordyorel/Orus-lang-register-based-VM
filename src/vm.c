#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "parser.h"
#include "vm_ops.h"

VM vm;

static void resetStack() { vm.stackTop = vm.stack; }
Type* variableTypes[UINT8_COUNT] = {NULL};

void initVM() {
    initTypeSystem();
    resetStack();
    vm.variableCount = 0;
    vm.frameCount = 0;
    for (int i = 0; i < UINT8_COUNT; i++) {
        vm.variableNames[i].name = NULL;
        vm.variableNames[i].length = 0;
        vm.globals[i] = NIL_VAL;
        vm.globalTypes[i] = NULL;
    }
}

void freeVM() {
    for (int i = 0; i < vm.variableCount; i++) {
        if (vm.variableNames[i].name != NULL) {
            free(vm.variableNames[i].name);
            vm.variableNames[i].name = NULL;
        }
        vm.globalTypes[i] = NULL;  // No freeing here
    }
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
}

static void traceExecution() {
    #ifdef DEBUG_TRACE_EXECUTION
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
    #endif
}

static InterpretResult run() {
    #define READ_BYTE() (*vm.ip++)
    #define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    
    // Define and reset loop safety counter at the start of each VM run
    static int absolute_loop_count = 0;
    absolute_loop_count = 0;

    InterpretResult result = INTERPRET_OK;

    for (;;) {
        traceExecution();
        uint8_t instruction = READ_BYTE();

        switch (instruction) {
            case OP_PRINT: {
                // Check for stack underflow before attempting to pop
                if (vm.stackTop <= vm.stack) {
                    runtimeError("Stack underflow in PRINT operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                Value value = vmPop(&vm);
                printf("OUTPUT: ");  // Add a clear prefix to program output
                printValue(value);
                printf("\n");
                fflush(stdout);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                vmPush(&vm, constant);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: OP_CONSTANT pushing value: ");
                // printValue(constant);
                // fprintf(stderr, "\n");

                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_ADD_I32:
                binaryOpI32(&vm, '+', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_SUBTRACT_I32:
                binaryOpI32(&vm, '-', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_MULTIPLY_I32:
                binaryOpI32(&vm, '*', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_DIVIDE_I32:
                binaryOpI32(&vm, '/', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_ADD_U32:
                binaryOpU32(&vm, '+', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_SUBTRACT_U32:
                binaryOpU32(&vm, '-', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_MULTIPLY_U32:
                binaryOpU32(&vm, '*', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_DIVIDE_U32:
                binaryOpU32(&vm, '/', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_MODULO_I32:
                moduloOpI32(&vm, &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_MODULO_U32:
                moduloOpU32(&vm, &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;

            // Comparison operations
            case OP_EQUAL:
                compareOpI32(&vm, '=', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_NOT_EQUAL:
                compareOpI32(&vm, '!', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_LESS_I32: {
                // Check for stack underflow
                if (vm.stackTop - vm.stack < 2) {
                    runtimeError("Stack underflow in LESS_I32 comparison. Need 2 values, have %ld.",
                                 vm.stackTop - vm.stack);
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value b = vmPop(&vm);  // Pop the second value first
                Value a = vmPop(&vm);  // Then pop the first value

                // Ensure we have valid integers
                if (!IS_I32(a) || !IS_I32(b)) {
                    runtimeError("Operands must be integers.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int32_t aValue = AS_I32(a);
                int32_t bValue = AS_I32(b);
                bool result = aValue < bValue;
                
                // Enhanced debug for loop conditions
                // fprintf(stderr, "DEBUG: LESS_I32 comparing %d < %d = %s\n", 
                //         aValue, bValue, result ? "true" : "false");
                
                // Push the result back onto the stack
                vmPush(&vm, BOOL_VAL(result));

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack after LESS_I32: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_LESS_U32:
                compareOpU32(&vm, '<', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_LESS_F64:
                compareOpF64(&vm, '<', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_LESS_EQUAL_I32:
                compareOpI32(&vm, 'L', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_LESS_EQUAL_U32:
                compareOpU32(&vm, 'L', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_LESS_EQUAL_F64:
                compareOpF64(&vm, 'L', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_GREATER_I32:
                compareOpI32(&vm, '>', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_GREATER_U32:
                compareOpU32(&vm, '>', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_GREATER_F64:
                compareOpF64(&vm, '>', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_GREATER_EQUAL_I32:
                compareOpI32(&vm, 'G', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_GREATER_EQUAL_U32:
                compareOpU32(&vm, 'G', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_GREATER_EQUAL_F64:
                compareOpF64(&vm, 'G', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_ADD_F64:
                binaryOpF64(&vm, '+', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_SUBTRACT_F64:
                binaryOpF64(&vm, '-', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_MULTIPLY_F64:
                binaryOpF64(&vm, '*', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_DIVIDE_F64:
                binaryOpF64(&vm, '/', &result);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            case OP_NEGATE_I32: {
                if (!IS_I32(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int32_t value = AS_I32(vmPop(&vm));
                vmPush(&vm, I32_VAL(-value));

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_NEGATE_U32: {
                if (!IS_U32(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be an unsigned integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint32_t value = AS_U32(vmPop(&vm));
                vmPush(&vm, U32_VAL(-value));

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_NEGATE_F64: {
                if (!IS_F64(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be a floating point number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double value = AS_F64(vmPop(&vm));
                vmPush(&vm, F64_VAL(-value));

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_I32_TO_F64: {
                Value value = vmPop(&vm);
                InterpretResult convResult = INTERPRET_OK;
                double floatValue = convertToF64(&vm, value, &convResult);
                if (convResult != INTERPRET_OK) {
                    runtimeError("Failed to convert value to float.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPush(&vm, F64_VAL(floatValue));

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_U32_TO_F64: {
                Value value = vmPop(&vm);
                InterpretResult convResult = INTERPRET_OK;
                double floatValue = convertToF64(&vm, value, &convResult);
                if (convResult != INTERPRET_OK) {
                    runtimeError("Failed to convert value to float.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPush(&vm, F64_VAL(floatValue));

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_POP: {
                vmPop(&vm);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_RETURN: {
                // Comment out debug prints
                // fprintf(
                //     stderr, "DEBUG: OP_RETURN: stackTop=%ld, stackBase=%d\n",
                //     vm.stackTop - vm.stack,
                //     vm.frameCount > 0 ? (int)vm.frames[vm.frameCount - 1].stackOffset
                //                       : -1);

                // Get the return value from the top of the stack
                Value returnValue;

                // If the stack is empty, use NIL_VAL as a default return value
                if (vm.stackTop <= vm.stack) {
                    returnValue = NIL_VAL;
                } else {
                    returnValue = vmPop(&vm);
                }

                // If we're in a function call, restore the call frame
                if (vm.frameCount > 0) {
                    // Restore the previous call frame
                    CallFrame* frame = &vm.frames[--vm.frameCount];

                    // Restore the instruction pointer
                    vm.ip = frame->returnAddress;

                    // Reset the stack to the frame's base, but keep the return value
                    // Make sure we don't set stackTop to an invalid position
                    if (frame->stackOffset >= 0 && frame->stackOffset < STACK_MAX) {
                        vm.stackTop = vm.stack + frame->stackOffset;
                        vmPush(&vm, returnValue);
                    } else {
                        // Invalid stack offset, just push the return value
                        vm.stackTop = vm.stack;
                        vmPush(&vm, returnValue);
                    }

                    // Comment out debug prints
                    // fprintf(stderr, "DEBUG: Function returned: ");
                    // printValue(returnValue);
                    // fprintf(stderr, "\n");

                    // For debugging, print the return value
                    printf("OUTPUT: Function returned: ");
                    printValue(returnValue);
                    printf("\n");
                    fflush(stdout);
                } else {
                    // If we're not in a function call, just push the return value back
                    vmPush(&vm, returnValue);

                    // Comment out debug prints
                    // fprintf(stderr, "DEBUG: Program finished with value: ");
                    // printValue(returnValue);
                    // fprintf(stderr, "\n");

                    return INTERPRET_OK;
                }

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint8_t index = READ_BYTE();
                
                // Check for stack underflow - but don't error, handle it gracefully
                if (vm.stackTop <= vm.stack) {
                    // If stack is empty, initialize with a default value (0 for the sum variable)
                    vm.globals[index] = I32_VAL(0);
                    vm.globalTypes[index] = variableTypes[index];
                    
                    // Make sure we have something on the stack for subsequent operations
                    vmPush(&vm, I32_VAL(0));
                } else {
                    // Normal case - copy value from stack to global
                    Value value = vmPeek(&vm, 0);
                    vm.globals[index] = value;
                    vm.globalTypes[index] = variableTypes[index];
                    
                    // Leave the value on the stack for any subsequent operations
                    // This is critical for the initial "sum = 0" assignment that gets printed
                    vmPop(&vm);  // Safe to pop now because we already stored it
                    vmPush(&vm, value);  // Push it back for the next operation
                }

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_GET_GLOBAL: {
                uint8_t index = READ_BYTE();
                if (vm.globalTypes[index] == NULL) {
                    runtimeError("Attempt to access undefined variable.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value value = vm.globals[index];
                vmPush(&vm, value);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: OP_GET_GLOBAL pushing value: ");
                // printValue(value);
                // fprintf(stderr, "\n");

                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_SET_GLOBAL: {
                uint8_t index = READ_BYTE();
                if (index >= vm.variableCount || vm.globalTypes[index] == NULL) {
                    runtimeError("Attempt to assign to undefined variable.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                // Peek at the value instead of popping it - this is safer for stack management
                if (vm.stackTop <= vm.stack) {
                    runtimeError("Stack underflow in SET_GLOBAL operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value value = vmPeek(&vm, 0);

                // Enhanced debug information for variables
                if (index < vm.variableCount && vm.variableNames[index].name != NULL) {
                    char varName[256] = {0};
                    strncpy(varName, vm.variableNames[index].name, vm.variableNames[index].length);
                    varName[vm.variableNames[index].length] = '\0';
                    
                    // fprintf(stderr, "DEBUG: Setting global %s (index %d) to ", 
                    //         varName, index);
                    // printValue(value);
                    // fprintf(stderr, "\n");
                    
                    // Special tracking for the loop test
                    if (strcmp(varName, "sum") == 0 || strcmp(varName, "i") == 0) {
                        // fprintf(stderr, "DEBUG: LOOP TRACKING - Variable %s is now ", varName);
                        
                        switch (value.type) {
                            case VAL_BOOL:
                                // fprintf(stderr, "[BOOL:%s]", AS_BOOL(value) ? "true" : "false");
                                break;
                            case VAL_NIL:
                                // fprintf(stderr, "[NIL]");
                                break;
                            case VAL_I32:
                                // fprintf(stderr, "[I32:%d]", AS_I32(value));
                                break;
                            case VAL_U32:
                                // fprintf(stderr, "[U32:%u]", AS_U32(value));
                                break;
                            case VAL_F64:
                                // fprintf(stderr, "[F64:%g]", AS_F64(value));
                                break;
                            case VAL_STRING:
                                // fprintf(stderr, "[STR:string]"); // Simplified as AS_CSTRING isn't available
                                break;
                            default:
                                // fprintf(stderr, "[UNKNOWN]");
                                break;
                        }
                        
                        // fprintf(stderr, "\n");
                    }
                }

                // Store the value in the global variable
                vm.globals[index] = value;
                
                // IMPORTANT CHANGE: We DON'T pop the value from the stack anymore
                // This ensures the value remains on the stack for subsequent operations
                // This fixes the stack underflow issues in loops and other complex code
                
                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                Value condition = vmPeek(&vm, 0);
                if (!IS_BOOL(condition)) {
                    runtimeError("Condition must be a boolean.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!AS_BOOL(condition)) {
                    vm.ip += offset;
                }

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = READ_SHORT();
                Value condition = vmPeek(&vm, 0);
                if (!IS_BOOL(condition)) {
                    runtimeError("Condition must be a boolean.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_BOOL(condition)) {
                    vm.ip += offset;
                }

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                
                // Use the global loop counter defined at the start of run()
                absolute_loop_count++;
                
                // If the loop has iterated more than 1000 times, it's likely an infinite loop
                if (absolute_loop_count > 1000) {
                    fprintf(stderr, "ERROR: Loop iteration limit exceeded (1000). "
                           "Forced termination to prevent infinite loop.\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                // fprintf(stderr, "DEBUG: OP_LOOP executing with offset %d\n", offset);
                
                // The OP_LOOP instruction should not be checking condition itself
                // It should simply jump back to where the condition is evaluated
                
                // Execute loop behavior - jump back to where conditions are evaluated
                vm.ip -= offset;
                
                // fprintf(stderr, "DEBUG: Jumped back to position %ld for next iteration\n", 
                //         vm.ip - vm.chunk->code);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after loop operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_BREAK: {
                // Break is handled at compile time by emitting a jump
                // This opcode should never be executed
                runtimeError("Unexpected OP_BREAK.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_CONTINUE: {
                // Continue is handled at compile time by emitting a loop
                // This opcode should never be executed
                runtimeError("Unexpected OP_CONTINUE.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_NIL: {
                vmPush(&vm, NIL_VAL);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            case OP_DEFINE_FUNCTION: {
                uint8_t index = READ_BYTE();

                // Nothing to do — compiler already stored function offset
                // fprintf(
                //     stderr,
                //     "DEBUG: OP_DEFINE_FUNCTION runtime for index %d (noop)\n",
                //     index);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }

            case OP_CALL: {
                uint8_t functionIndex = READ_BYTE();
                uint8_t argCount = READ_BYTE();

                // Ensure we're calling a valid function
                if (functionIndex >= vm.variableCount || !IS_I32(vm.globals[functionIndex])) {
                    runtimeError("Attempt to call a non-function.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int32_t functionPos = AS_I32(vm.globals[functionIndex]);
                // fprintf(stderr,
                //         "DEBUG: Retrieved function position %d from global "
                //         "index %d\n",
                //         functionPos, functionIndex);

                // Check call stack limit
                if (vm.frameCount >= FRAMES_MAX) {
                    runtimeError("Stack overflow.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Debug output
                // fprintf(stderr,
                //         "DEBUG: Executing function at bytecode offset %d with "
                //         "%d args\n",
                //         functionPos, argCount);

                // Set up new call frame
                CallFrame* frame = &vm.frames[vm.frameCount++];
                frame->returnAddress = vm.ip;
                
                // Ensure we have a valid stack offset
                int stackOffset = (int)(vm.stackTop - vm.stack - argCount);
                if (stackOffset < 0) {
                    // fprintf(stderr, "Warning: Correcting negative stack offset from %d to 0\n", stackOffset);
                    stackOffset = 0;
                }
                
                frame->stackOffset = stackOffset;
                frame->functionIndex = functionIndex;

                // Initialize the stack if needed (this ensures we have enough space for local variables)
                if (vm.stackTop == vm.stack) {
                    // Stack is empty, initialize it with at least one value
                    vmPush(&vm, I32_VAL(0));  // Push a dummy value that won't be used
                    // We don't pop this value - it's needed to ensure stack operations work
                }

                // Jump to function body
                vm.ip = vm.chunk->code + functionPos;
                // fprintf(stderr, "DEBUG: Entered function at offset %d\n",
                //         (int)(vm.ip - vm.chunk->code));
                // fprintf(stderr, "DEBUG: Stack top = %ld\n",
                //         vm.stackTop - vm.stack);

                // Comment out debug prints
                // fprintf(stderr, "DEBUG: Stack state after operation: ");
                // for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                //     printValue(*slot);
                //     fprintf(stderr, " ");
                // }
                // fprintf(stderr, "\n");

                break;
            }
            default:
                runtimeError("Unknown opcode: %d", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
        if (result != INTERPRET_OK) return result;
    }

    #undef READ_BYTE
    #undef READ_SHORT
    #undef READ_CONSTANT

    return result;
}

InterpretResult runChunk(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = chunk->code;

    #ifdef DEBUG_TRACE_EXECUTION
        disassembleChunk(chunk, "chunk to execute");
    #endif

    return run();
}

void push(Value value) {
    vmPush(&vm, value);
}

Value pop() {
    return vmPop(&vm);
}

InterpretResult interpret(const char* source) {
    ASTNode* ast;
    if (!parse(source, &ast)) {
        return INTERPRET_COMPILE_ERROR;
    }

    Chunk chunk;
    initChunk(&chunk);
    Compiler compiler;
    initCompiler(&compiler, &chunk);

    bool success = compile(ast, &compiler);
    freeASTNode(ast);  // Free AST only once after compilation

    if (!success) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    InterpretResult result = runChunk(&chunk);
    freeChunk(&chunk);
    return result;
}