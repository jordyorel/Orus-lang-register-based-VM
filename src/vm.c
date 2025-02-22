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
    printf("DEBUG: vm.variableCount = %d\n", vm.variableCount);
    for (int i = 0; i < vm.variableCount; i++) {
        printf("DEBUG: Before free: vm.variableNames[%d].name = %s at %p\n", i,
               vm.variableNames[i].name ? vm.variableNames[i].name : "(null)",
               vm.variableNames[i].name);
        if (vm.variableNames[i].name != NULL) {
            printf("DEBUG: About to free variable name at index %d: %s at %p\n",
                   i, vm.variableNames[i].name, vm.variableNames[i].name);
            free(vm.variableNames[i].name);
            vm.variableNames[i].name = NULL;
            printf("DEBUG: Freed variable name at index %d\n", i);
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

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

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
    printf("DEBUG: Running chunk at %p\n", (void*)chunk);
    #ifdef DEBUG_TRACE_EXECUTION
        disassembleChunk(chunk, "chunk to execute");
    #endif

    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    InterpretResult result = INTERPRET_OK;

    for (;;) {
        traceExecution();
        uint8_t instruction = READ_BYTE();

        switch (instruction) {
            case OP_PRINT: {
                Value value = pop();
                printValue(value);
                printf("\n");
                fflush(stdout);
                break;
            }
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
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
                if (!IS_I32(peek(0))) {
                    runtimeError("Operand must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int32_t value = AS_I32(pop());
                push(I32_VAL(-value));
                break;
            }
            case OP_NEGATE_U32: {
                if (!IS_U32(peek(0))) {
                    runtimeError("Operand must be an unsigned integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint32_t value = AS_U32(pop());
                push(U32_VAL(-value));
                break;
            }
            case OP_NEGATE_F64: {
                if (!IS_F64(peek(0))) {
                    runtimeError("Operand must be a floating point number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double value = AS_F64(pop());
                push(F64_VAL(-value));
                break;
            }
            case OP_I32_TO_F64: {
                if (!IS_I32(peek(0))) {
                    runtimeError("Operand must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int32_t value = AS_I32(pop());
                push(F64_VAL((double)value));
                break;
            }
            case OP_U32_TO_F64: {
                if (!IS_U32(peek(0))) {
                    runtimeError("Operand must be an unsigned integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint32_t value = AS_U32(pop());
                push(F64_VAL((double)value));
                break;
            }
            case OP_POP: {
                pop();
                break;
            }
            case OP_RETURN: {
                return INTERPRET_OK;
            }
            case OP_DEFINE_GLOBAL: {
                uint8_t index = READ_BYTE();
                vm.globals[index] = pop();
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
                push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                uint8_t index = READ_BYTE();
                if (vm.globalTypes[index] == NULL) {
                    runtimeError("Attempt to assign to undefined variable.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value value = pop();
                vm.globals[index] = value;
                push(value);  // Push back for expression value
                pop();        // Immediately pop since assignment isn't used
                break;
            }
            default:
                runtimeError("Unknown opcode: %d", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
        if (result != INTERPRET_OK) return result;
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
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