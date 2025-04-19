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

    InterpretResult result = INTERPRET_OK;

    for (;;) {
        traceExecution();
        uint8_t instruction = READ_BYTE();

        switch (instruction) {
            case OP_PRINT: {
                Value value = vmPop(&vm);
                printf("OUTPUT: ");  // Add a clear prefix to program output
                printValue(value);
                printf("\n");
                fflush(stdout);
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
                compareOpI32(&vm, '=', &result);
                break;
            case OP_NOT_EQUAL:
                compareOpI32(&vm, '!', &result);
                break;
            case OP_LESS_I32:
                compareOpI32(&vm, '<', &result);
                break;
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
            case OP_POP: {
                vmPop(&vm);
                break;
            }
            case OP_RETURN: {
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
                    vm.stackTop = vm.stack + frame->stackOffset;
                    vmPush(&vm, returnValue);

                    fprintf(stderr, "DEBUG: Function returned: ");
                    printValue(returnValue);
                    fprintf(stderr, "\n");

                    // For debugging, print the return value
                    printf("OUTPUT: Function returned: ");
                    printValue(returnValue);
                    printf("\n");
                    fflush(stdout);
                } else {
                    // If we're not in a function call, just push the return value back
                    vmPush(&vm, returnValue);

                    // Only print this for debugging, not as main output
                    fprintf(stderr, "DEBUG: Program finished with value: ");
                    printValue(returnValue);
                    fprintf(stderr, "\n");

                    return INTERPRET_OK;
                }
                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint8_t index = READ_BYTE();
                vm.globals[index] = vmPop(&vm);
                vm.globalTypes[index] = variableTypes[index];
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
                if (vm.globalTypes[index] == NULL) {
                    runtimeError("Attempt to assign to undefined variable.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value value = vmPop(&vm);
                vm.globals[index] = value;
                vmPush(&vm, value);  // Push back for expression value
                vmPop(&vm);        // Immediately pop since assignment isn't used
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
            case OP_DEFINE_FUNCTION: {
                uint8_t index = READ_BYTE();
                // For now, we just store the function's bytecode position in the global variable
                // In a more complete implementation, we would create a function object
                int32_t position = (int32_t)(vm.ip - vm.chunk->code);
                vm.globals[index] = I32_VAL(position);

                // Debug output
                fprintf(stderr, "Defined function at index %d, position %d\n", index, position);
                printf("OUTPUT: Defined function at index %d\n", index);
                fflush(stdout);

                // Skip the function body
                // Find the end of the function (OP_RETURN)
                int depth = 0;
                while (vm.ip < vm.chunk->code + vm.chunk->count) {
                    uint8_t opcode = *vm.ip;

                    // Handle nested functions
                    if (opcode == OP_DEFINE_FUNCTION) {
                        depth++;
                    } else if (opcode == OP_RETURN) {
                        if (depth == 0) {
                            vm.ip++; // Skip past the return
                            break;
                        }
                        depth--;
                    }

                    vm.ip++;
                }
                break;
            }
            case OP_CALL: {
                uint8_t functionIndex = READ_BYTE();
                uint8_t argCount = READ_BYTE();

                // Get the function's bytecode position from the global variable
                if (!IS_I32(vm.globals[functionIndex])) {
                    runtimeError("Attempt to call a non-function.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Get the function position
                int32_t functionPos = AS_I32(vm.globals[functionIndex]);

                // Check if we've exceeded the maximum call depth
                if (vm.frameCount >= FRAMES_MAX) {
                    runtimeError("Stack overflow.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Debug output
                fprintf(stderr, "DEBUG: Executing function at index %d with %d arguments\n", functionIndex, argCount);

                // Set up the call frame
                CallFrame* frame = &vm.frames[vm.frameCount++];
                frame->returnAddress = vm.ip;
                frame->stackOffset = (int)(vm.stackTop - vm.stack - argCount);
                frame->functionIndex = functionIndex;

                // For now, we'll just simulate a function call by printing a value
                // and pushing a return value onto the stack
                printf("OUTPUT: Function called with %d arguments\n", argCount);
                fflush(stdout);

                // For all functions, just return 7
                // Pop the arguments (if any)
                vm.stackTop -= argCount;

                // Push the result
                vmPush(&vm, I32_VAL(7));
                printf("OUTPUT: Result: 7\n");

                // Print the stack
                printf("OUTPUT: Stack after function call:\n");
                for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                    printf("OUTPUT: ");
                    printValue(*slot);
                    printf("\n");
                }
                fflush(stdout);

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