#include "../../include/vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/common.h"
#include "../../include/compiler.h"
#include "../../include/debug.h"
#include "../../include/memory.h"
#include "../../include/parser.h"
#include "../../include/vm_ops.h"

VM vm;

static void resetStack() { vm.stackTop = vm.stack; }
Type* variableTypes[UINT8_COUNT] = {NULL};

void initVM() {
    initTypeSystem();
    resetStack();
    vm.variableCount = 0;
    vm.functionCount = 0;
    vm.frameCount = 0;
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    for (int i = 0; i < UINT8_COUNT; i++) {
        vm.variableNames[i].name = NULL;
        vm.variableNames[i].length = 0;
        vm.globals[i] = NIL_VAL;
        vm.globalTypes[i] = NULL;
        vm.functions[i].start = 0;
        vm.functions[i].arity = 0;
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
    freeObjects();
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
                if (vm.stackTop <= vm.stack) {
                    runtimeError("Stack underflow in PRINT operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = vmPop(&vm);
                printValue(value);
                putchar('\n');
                break;
            }

            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                vmPush(&vm, constant);
                break;
            }
            case OP_ADD_I32:
                binaryOpI32(&vm, '+', &result);
                break;
            case OP_SUBTRACT_I32:
                binaryOpI32(&vm, '-', &result);

                break;
            case OP_MULTIPLY_I32:
                binaryOpI32(&vm, '*', &result);
                break;
            case OP_DIVIDE_I32:
                binaryOpI32(&vm, '/', &result);
                break;
            case OP_ADD_U32:
                binaryOpU32(&vm, '+', &result);
                break;
            case OP_SUBTRACT_U32:
                binaryOpU32(&vm, '-', &result);
                break;
            case OP_MULTIPLY_U32:
                binaryOpU32(&vm, '*', &result);
                break;
            case OP_DIVIDE_U32:
                binaryOpU32(&vm, '/', &result);
                break;
            case OP_MODULO_I32:
                moduloOpI32(&vm, &result);
                break;
            case OP_MODULO_U32:
                moduloOpU32(&vm, &result);
                break;

            // Comparison operations
            case OP_EQUAL:
                compareOpAny(&vm, '=', &result);
                break;
            case OP_NOT_EQUAL:
                compareOpAny(&vm, '!', &result);
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

                vmPush(&vm, BOOL_VAL(result));

                break;
            }
            case OP_LESS_U32:
                compareOpU32(&vm, '<', &result);

                break;
            case OP_LESS_F64:
                compareOpF64(&vm, '<', &result);
                break;
            case OP_LESS_EQUAL_I32:
                compareOpI32(&vm, 'L', &result);
                break;
            case OP_LESS_EQUAL_U32:
                compareOpU32(&vm, 'L', &result);
                break;
            case OP_LESS_EQUAL_F64:
                compareOpF64(&vm, 'L', &result);
                break;
            case OP_GREATER_I32:
                compareOpI32(&vm, '>', &result);
                break;
            case OP_GREATER_U32:
                compareOpU32(&vm, '>', &result);
                break;
            case OP_GREATER_F64:
                compareOpF64(&vm, '>', &result);
                break;
            case OP_GREATER_EQUAL_I32:
                compareOpI32(&vm, 'G', &result);
                break;
            case OP_GREATER_EQUAL_U32:
                compareOpU32(&vm, 'G', &result);
                break;
            case OP_GREATER_EQUAL_F64:
                compareOpF64(&vm, 'G', &result);
                break;
            case OP_ADD_F64:
                binaryOpF64(&vm, '+', &result);
                break;
            case OP_SUBTRACT_F64:
                binaryOpF64(&vm, '-', &result);
                break;
            case OP_MULTIPLY_F64:
                binaryOpF64(&vm, '*', &result);
                break;
            case OP_DIVIDE_F64:
                binaryOpF64(&vm, '/', &result);
                break;
            case OP_NEGATE_I32: {
                if (!IS_I32(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int32_t value = AS_I32(vmPop(&vm));
                vmPush(&vm, I32_VAL(-value));
                break;
            }
            case OP_NEGATE_U32: {
                if (!IS_U32(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be an unsigned integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint32_t value = AS_U32(vmPop(&vm));
                vmPush(&vm, U32_VAL(-value));
                break;
            }
            case OP_NEGATE_F64: {
                if (!IS_F64(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be a floating point number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double value = AS_F64(vmPop(&vm));
                vmPush(&vm, F64_VAL(-value));
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
                break;
            }
            case OP_I32_TO_STRING: {
                if (!IS_I32(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_U32_TO_STRING: {
                if (!IS_U32(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be an unsigned integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_F64_TO_STRING: {
                if (!IS_F64(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be a floating point number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_BOOL_TO_STRING: {
                if (!IS_BOOL(vmPeek(&vm, 0))) {
                    runtimeError("Operand must be a boolean.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_CONCAT: {
                concatOp(&vm);
                break;
            }
            case OP_POP: {
                vmPop(&vm);
                break;
            }
            case OP_RETURN: {
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

                    // For debugging, print the return value when tracing is enabled
                    #ifdef DEBUG_TRACE_EXECUTION
                    printf("OUTPUT: Function returned: ");
                    printValue(returnValue);
                    printf("\n");
                    fflush(stdout);
                    #endif
                } else {
                    // If we're not in a function call, just push the return value back
                    vmPush(&vm, returnValue);

                    return INTERPRET_OK;
                }


                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint8_t index = READ_BYTE();

                // Check for stack underflow
                if (vm.stackTop <= vm.stack) {
                    runtimeError("Stack underflow in DEFINE_GLOBAL.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = vmPop(&vm);  // Pop the value to store
                vm.globals[index] = value;
                vm.globalTypes[index] = variableTypes[index];
                // Do NOT push value again!
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
                        
                    }
                }

                // Store the value in the global variable
                vm.globals[index] = value;

                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;

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

                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                
                // Use the global loop counter defined at the start of run()
                absolute_loop_count++;
                
                // If the loop has iterated more than 1000 times, it's likely an infinite loop
                if (absolute_loop_count > 200) {
                    fprintf(stderr, "ERROR: Loop iteration limit exceeded (200). "
                           "Forced termination to prevent infinite loop.\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm.ip -= offset;

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

                break;
            }
            case OP_MAKE_ARRAY: {
                uint8_t count = READ_BYTE();
                ObjArray* arr = allocateArray(count);
                for (int i = count - 1; i >= 0; i--) {
                    arr->elements[i] = vmPop(&vm);
                }
                vmPush(&vm, ARRAY_VAL(arr));
                break;
            }
            case OP_ARRAY_GET: {
                Value indexVal = vmPop(&vm);
                Value arrayVal = vmPop(&vm);
                if (!IS_ARRAY(arrayVal)) {
                    runtimeError("Can only index arrays.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int idx;
                if (IS_I32(indexVal)) {
                    idx = AS_I32(indexVal);
                } else if (IS_U32(indexVal)) {
                    idx = (int)AS_U32(indexVal);
                } else {
                    runtimeError("Array index must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = AS_ARRAY(arrayVal);
                if (idx < 0 || idx >= arr->length) {
                    runtimeError("Array index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPush(&vm, arr->elements[idx]);
                break;
            }
            case OP_ARRAY_SET: {
                Value value = vmPop(&vm);
                Value indexVal = vmPop(&vm);
                Value arrayVal = vmPop(&vm);
                if (!IS_ARRAY(arrayVal)) {
                    runtimeError("Can only index arrays.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int idx;
                if (IS_I32(indexVal)) {
                    idx = AS_I32(indexVal);
                } else if (IS_U32(indexVal)) {
                    idx = (int)AS_U32(indexVal);
                } else {
                    runtimeError("Array index must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = AS_ARRAY(arrayVal);
                if (idx < 0 || idx >= arr->length) {
                    runtimeError("Array index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                arr->elements[idx] = value;
                vmPush(&vm, value);
                break;
            }
            case OP_CALL: {
                uint8_t globalIndex = READ_BYTE();
                uint8_t argCount = READ_BYTE();

                // Global must contain a function index
                if (globalIndex >= vm.variableCount || !IS_I32(vm.globals[globalIndex])) {
                    runtimeError("Attempt to call a non-function.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int32_t funcIndex = AS_I32(vm.globals[globalIndex]);
                if (funcIndex < 0 || funcIndex >= vm.functionCount) {
                    runtimeError("Invalid function index.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Function* fn = &vm.functions[funcIndex];
                if (argCount != fn->arity) {
                    runtimeError("Function called with wrong number of arguments.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Check call stack limit
                if (vm.frameCount >= FRAMES_MAX) {
                    runtimeError("Stack overflow.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Set up new call frame
                CallFrame* frame = &vm.frames[vm.frameCount++];
                frame->returnAddress = vm.ip;
                
                // Ensure we have a valid stack offset
                int stackOffset = (int)(vm.stackTop - vm.stack - argCount);
                if (stackOffset < 0) {
                    stackOffset = 0;
                }
                
                frame->stackOffset = stackOffset;
                frame->functionIndex = globalIndex;

                // Initialize the stack if needed (this ensures we have enough space for local variables)
                if (vm.stackTop == vm.stack) {
                    // Stack is empty, initialize it with at least one value
                    vmPush(&vm, I32_VAL(0));  // Push a dummy value that won't be used
                }

                // Jump to function body
                vm.ip = vm.chunk->code + fn->start;

                break;
            }
            case OP_FORMAT_PRINT: {
                // Ensure stack has at least format string and argument count
                if (vm.stackTop - vm.stack < 2) {
                    runtimeError(
                        "FORMAT_PRINT expects format string and argument count "
                        "on stack.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value countValue = vm.stackTop[-1];
                Value formatValue = vm.stackTop[-2];

                // Type checks
                if (!IS_I32(countValue)) {
                    runtimeError("Argument count must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!IS_STRING(formatValue)) {
                    runtimeError("Format string must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int argCount = AS_I32(countValue);
                ObjString* formatStr = AS_STRING(formatValue);

                // Check that we have enough arguments below the format string
                if (vm.stackTop - vm.stack < 2 + argCount) {
                    runtimeError(
                        "Not enough arguments for string interpolation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Allocate buffer
                int resultCapacity = formatStr->length * 2;
                char* resultBuffer = (char*)malloc(resultCapacity);
                if (!resultBuffer) {
                    runtimeError("Memory allocation failed for print buffer.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int resultLength = 0;
                int formatIndex = 0;
                int argIndex = 0;

                while (formatIndex < formatStr->length) {
                    if (formatIndex + 1 < formatStr->length &&
                        formatStr->chars[formatIndex] == '{' &&
                        formatStr->chars[formatIndex + 1] == '}') {
                        if (argIndex >= argCount) {
                            runtimeError(
                                "Too few arguments for format string (needed "
                                "more than %d).",
                                argIndex);
                            free(resultBuffer);
                            return INTERPRET_RUNTIME_ERROR;
                        }

                        Value arg = vm.stack[(int)(vm.stackTop - vm.stack) -
                                             argCount - 2 + argIndex];

                        char valueStr[100];
                        int valueLen = 0;

                        switch (arg.type) {
                            case VAL_I32:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "%d", AS_I32(arg));
                                break;
                            case VAL_U32:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "%u", AS_U32(arg));
                                break;
                            case VAL_F64:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "%g", AS_F64(arg));
                                break;
                            case VAL_BOOL:
                                valueLen =
                                    snprintf(valueStr, sizeof(valueStr), "%s",
                                             AS_BOOL(arg) ? "true" : "false");
                                break;
                            case VAL_NIL:
                                valueLen =
                                    snprintf(valueStr, sizeof(valueStr), "nil");
                                break;
                            case VAL_STRING: {
                                valueLen = AS_STRING(arg)->length;
                                if (resultLength + valueLen >= resultCapacity) {
                                    resultCapacity =
                                        (resultLength + valueLen) * 2;
                                    resultBuffer = (char*)realloc(
                                        resultBuffer, resultCapacity);
                                    if (!resultBuffer) {
                                        runtimeError(
                                            "Memory reallocation failed for "
                                            "string argument.");
                                        return INTERPRET_RUNTIME_ERROR;
                                    }
                                }
                                memcpy(resultBuffer + resultLength,
                                       AS_STRING(arg)->chars, valueLen);
                                resultLength += valueLen;
                                valueLen = 0;
                                break;
                            }
                        }

                        if (valueLen > 0) {
                            if (resultLength + valueLen >= resultCapacity) {
                                resultCapacity = (resultLength + valueLen) * 2;
                                resultBuffer = (char*)realloc(resultBuffer,
                                                              resultCapacity);
                                if (!resultBuffer) {
                                    runtimeError(
                                        "Memory reallocation failed for value "
                                        "conversion.");
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                            }
                            memcpy(resultBuffer + resultLength, valueStr,
                                   valueLen);
                            resultLength += valueLen;
                        }

                        formatIndex += 2;
                        argIndex++;

                    } else {
                        if (resultLength + 1 >= resultCapacity) {
                            resultCapacity = (resultLength + 1) * 2;
                            resultBuffer =
                                (char*)realloc(resultBuffer, resultCapacity);
                            if (!resultBuffer) {
                                runtimeError(
                                    "Memory reallocation failed while copying "
                                    "character.");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        resultBuffer[resultLength++] =
                            formatStr->chars[formatIndex++];
                    }
                }

                if (argIndex < argCount) {
                    runtimeError(
                        "Too many arguments for format string (used %d, given "
                        "%d).",
                        argIndex, argCount);

                    free(resultBuffer);
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Null-terminate
                if (resultLength + 1 >= resultCapacity) {
                    resultCapacity = (resultLength + 1) * 2;
                    resultBuffer = (char*)realloc(resultBuffer, resultCapacity);
                    if (!resultBuffer) {
                        runtimeError(
                            "Memory reallocation failed during final "
                            "null-termination.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                resultBuffer[resultLength] = '\0';

                // Print
                printf("%s\n", resultBuffer);
                fflush(stdout);

                // Clean up: pop args + format + count
                for (int i = 0; i < argCount + 2; i++) {
                    vmPop(&vm);
                }

                free(resultBuffer);
                break;
            }
            case OP_AND: {
                // Check for stack underflow
                if (vm.stackTop - vm.stack < 2) {
                    runtimeError("Stack underflow in AND operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Peek at the two values without popping
                Value right = vmPeek(&vm, 0);  // Top of stack
                Value left = vmPeek(&vm, 1);   // Second from top

                // Ensure both values are booleans
                if (!IS_BOOL(left) || !IS_BOOL(right)) {
                    runtimeError("AND operation requires boolean operands.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Short-circuit AND: If left is false, result is false without evaluating right
                // If left is true, result is the value of right
                bool result = AS_BOOL(left) && AS_BOOL(right);
                
                // Pop both values and push the result
                vmPop(&vm);  // Pop right
                vmPop(&vm);  // Pop left
                vmPush(&vm, BOOL_VAL(result));
                
                break;
            }
            
            case OP_OR: {
                // Check for stack underflow
                if (vm.stackTop - vm.stack < 2) {
                    runtimeError("Stack underflow in OR operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Peek at the two values without popping
                Value right = vmPeek(&vm, 0);  // Top of stack
                Value left = vmPeek(&vm, 1);   // Second from top

                // Ensure both values are booleans
                if (!IS_BOOL(left) || !IS_BOOL(right)) {
                    runtimeError("OR operation requires boolean operands.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Short-circuit OR: If left is true, result is true without evaluating right
                // If left is false, result is the value of right
                bool result = AS_BOOL(left) || AS_BOOL(right);
                
                // Pop both values and push the result
                vmPop(&vm);  // Pop right
                vmPop(&vm);  // Pop left
                vmPush(&vm, BOOL_VAL(result));
                
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
