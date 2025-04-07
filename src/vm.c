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

InterpretResult runChunk(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = chunk->code;

    #ifdef DEBUG_TRACE_EXECUTION
        disassembleChunk(chunk, "chunk to execute");
    #endif

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
                return INTERPRET_OK;
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
            default:
                runtimeError("Unknown opcode: %d", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
        if (result != INTERPRET_OK) return result;
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
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