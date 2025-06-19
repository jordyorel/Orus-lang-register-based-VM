/**
 * @file vm.c
 * @brief Virtual machine execution engine.
 */
#include "../../include/vm.h"
#include "../../include/builtins.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../../include/common.h"
#include "../../include/compiler.h"
#include "../../include/debug.h"
#include "../../include/memory.h"
#include "../../include/error.h"
#include "../../include/location.h"
#include "../../include/file_utils.h"
#include "../../include/bytecode_io.h"
#include "../../include/parser.h"
#include "../../include/vm_ops.h"
#include "../../include/modules.h"
#include "../../include/type.h"
#include "../../include/reg_ir.h"
#include "../../include/builtin_stdlib.h"
#include <sys/stat.h>
/**
 * Compute the cache file path for a module within ORUS_CACHE_PATH.
 */

static char* cache_path_for(const char* module_path) {
    if (!vm.cachePath) return NULL;
    const char* base = strrchr(module_path, '/');
    base = base ? base + 1 : module_path;
    char* buf = malloc(strlen(vm.cachePath) + strlen(base) + 6);
    sprintf(buf, "%s/%s.obc", vm.cachePath, base);
    return buf;
}

VM vm;

/**
 * Reset the VM stack pointer to the bottom of the stack.
 */
static void resetStack() {
    vm.stackTop = vm.stack;
    vm.stackI64Top = vm.stackI64;
}
Type* variableTypes[UINT8_COUNT] = {NULL};

static const char* runtimeStack[UINT8_COUNT];
static uint8_t runtimeStackCount = 0;

/**
 * Validate a runtime value against a static type.
 *
 * @param value Value to check.
 * @param type  Expected type or NULL.
 * @return      True if the value matches.
 */
static bool checkValueAgainstType(Value value, Type* type) {
    if (!type) return true;
    switch (type->kind) {
        case TYPE_I32: return IS_I32(value);
        case TYPE_I64: return IS_I64(value);
        case TYPE_U32: return IS_U32(value);
        case TYPE_U64: return IS_U64(value);
        case TYPE_F64: return IS_F64(value);
        case TYPE_BOOL: return IS_BOOL(value);
        case TYPE_STRING: return IS_STRING(value);
        case TYPE_VOID: return false; // void has no runtime value
        case TYPE_NIL: return IS_NIL(value);
        case TYPE_ARRAY: return IS_ARRAY(value);
        default: return true;
    }
}


static InterpretResult run();

/**
 * Initialize the virtual machine state and register builtins.
 */
void initVM() {
    initTypeSystem();
    vm.stackCapacity = STACK_INIT_CAPACITY;
    vm.stack = GROW_ARRAY(Value, NULL, 0, vm.stackCapacity);
    if (!vm.stack) {
        fprintf(stderr, "Failed to allocate VM stack\n");
        exit(1);
    }
    vm.stackI64 = GROW_ARRAY(int64_t, NULL, 0, vm.stackCapacity);
    if (!vm.stackI64) {
        fprintf(stderr, "Failed to allocate VM i64 stack\n");
        exit(1);
    }
    resetStack();
    vm.variableCount = 0;
    vm.functionCount = 0;
    vm.frameCount = 0;
    vm.regFrameCount = 0;
    vm.tryFrameCount = 0;
    vm.lastError = NIL_VAL;
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.gcPaused = false;
    vm.instruction_count = 0;
    vm.astRoot = NULL;
    vm.filePath = NULL;
    vm.currentLine = 0;
    vm.currentColumn = 1;
    vm.moduleCount = 0;
    vm.nativeFunctionCount = 0;
    const char* envTrace = getenv("ORUS_TRACE");
    vm.trace = envTrace && envTrace[0] != '\0';
    const char* envPath = getenv("ORUS_PATH");
    vm.stdPath = envPath && envPath[0] != '\0' ? envPath : NULL;
    const char* envCache = getenv("ORUS_CACHE_PATH");
    vm.cachePath = envCache && envCache[0] != '\0' ? envCache : NULL;
    const char* envDev = getenv("ORUS_DEV_MODE");
    vm.devMode = envDev && envDev[0] != '\0';

    vm.useRegisterVM = true;
    initRegisterChunk(&vm.regChunk);
    initRegisterVM(&vm.regVM, &vm.regChunk);
    for (int i = 0; i < UINT8_COUNT; i++) {
        vm.loadedModules[i] = NULL;
    }
    for (int i = 0; i < UINT8_COUNT; i++) {
        vm.variableNames[i].name = NULL;
        vm.variableNames[i].length = 0;
        vm.globals[i] = NIL_VAL;
        vm.globalTypes[i] = NULL;
        vm.publicGlobals[i] = false;
        vm.functions[i].start = 0;
        vm.functions[i].arity = 0;
        vm.functions[i].chunk = NULL;
        vm.functionDecls[i] = NULL;
    }

    // Register core native functions
    initBuiltins();
}

/**
 * Release all resources associated with the virtual machine.
 */
void freeVM() {
    
    for (int i = 0; i < UINT8_COUNT; i++) {
        vm.variableNames[i].name = NULL;
        vm.globalTypes[i] = NULL;  // No freeing here
        vm.functionDecls[i] = NULL;
        vm.publicGlobals[i] = false;
    }
    vm.astRoot = NULL;
    if (vm.stack) {
        FREE_ARRAY(Value, vm.stack, vm.stackCapacity);
        vm.stack = NULL;
        vm.stackTop = NULL;
        vm.stackCapacity = 0;
    }
    if (vm.stackI64) {
        FREE_ARRAY(int64_t, vm.stackI64, vm.stackCapacity);
        vm.stackI64 = NULL;
        vm.stackI64Top = NULL;
    }
    freeRegisterVM(&vm.regVM);
    freeRegisterChunk(&vm.regChunk);
    freeObjects();
    vm.tryFrameCount = 0;
    vm.lastError = NIL_VAL;
    vm.moduleCount = 0;
    vm.filePath = NULL;
    vm.currentLine = 0;
    vm.currentColumn = 1;
}

/**
 * Output the current call stack to stderr.
 */
static void printStackTrace() {
    if (vm.frameCount == 0) return;
    fprintf(stderr, "Stack trace:\n");
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        fprintf(stderr, "  at function %d\n", frame->functionIndex);
    }
}

/**
 * Public wrapper to print the VM stack trace.
 */
void vmPrintStackTrace(void) { printStackTrace(); }

static void runtimeError(ErrorType type, SrcLocation location,
                         const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // If no explicit location provided, use the current executing line
    if (location.file == NULL && vm.filePath) {
        location.file = vm.filePath;
        location.line = vm.currentLine;
        location.column = vm.currentColumn;
    }

    ObjError* err = allocateError(type, buffer, location);
    vm.lastError = ERROR_VAL(err);
}

/**
 * Convenience wrapper to raise a runtime error without location info.
 *
 * @param message Error message.
 */
void vmRuntimeError(const char* message) {
    runtimeError(ERROR_RUNTIME, (SrcLocation){NULL, 0, 0}, "%s", message);
}

#define RUNTIME_ERROR(fmt, ...) \
    runtimeError(ERROR_RUNTIME, (SrcLocation){NULL, 0, 0}, fmt, ##__VA_ARGS__)

static bool appendStringDynamic(const char* src, char** buffer,
                                int* length, int* capacity) {
    int srcLen = (int)strlen(src);
    if (*length + srcLen >= *capacity) {
        *capacity = (*length + srcLen) * 2;
        char* newBuf = (char*)realloc(*buffer, *capacity);
        if (!newBuf) {
            RUNTIME_ERROR("Memory reallocation failed during string append.");
            return false;
        }
        *buffer = newBuf;
    }
    memcpy(*buffer + *length, src, srcLen);
    *length += srcLen;
    return true;
}

static bool appendValueString(Value value, char** buffer, int* length,
                              int* capacity) {
    char tmp[100];
    switch (value.type) {
        case VAL_I32:
            snprintf(tmp, sizeof(tmp), "%d", AS_I32(value));
            return appendStringDynamic(tmp, buffer, length, capacity);
        case VAL_I64:
            snprintf(tmp, sizeof(tmp), "%lld", (long long)AS_I64(value));
            return appendStringDynamic(tmp, buffer, length, capacity);
        case VAL_U32:
            snprintf(tmp, sizeof(tmp), "%u", AS_U32(value));
            return appendStringDynamic(tmp, buffer, length, capacity);
        case VAL_U64:
            snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)AS_U64(value));
            return appendStringDynamic(tmp, buffer, length, capacity);
        case VAL_F64:
            snprintf(tmp, sizeof(tmp), "%g", AS_F64(value));
            return appendStringDynamic(tmp, buffer, length, capacity);
        case VAL_BOOL:
            return appendStringDynamic(AS_BOOL(value) ? "true" : "false",
                                       buffer, length, capacity);
        case VAL_NIL:
            return appendStringDynamic("nil", buffer, length, capacity);
        case VAL_STRING:
            return appendStringDynamic(AS_STRING(value)->chars, buffer, length,
                                       capacity);
        case VAL_ARRAY: {
            if (!appendStringDynamic("[", buffer, length, capacity)) return false;
            ObjArray* arr = AS_ARRAY(value);
            for (int i = 0; i < arr->length; i++) {
                if (!appendValueString(arr->elements[i], buffer, length,
                                      capacity))
                    return false;
                if (i < arr->length - 1) {
                    if (!appendStringDynamic(", ", buffer, length, capacity))
                        return false;
                }
            }
            if (!appendStringDynamic("]", buffer, length, capacity)) return false;
            return true;
        }
        case VAL_ERROR: {
            snprintf(tmp, sizeof(tmp), "Error(%d): %s", AS_ERROR(value)->type,
                     AS_ERROR(value)->message->chars);
            return appendStringDynamic(tmp, buffer, length, capacity);
        }
        default:
            return appendStringDynamic("unknown", buffer, length, capacity);
    }
}

/**
 * Debug helper to print the VM stack and current instruction.
 */
static void traceExecution() {
#ifdef DEBUG_TRACE_EXECUTION
    if (!vm.trace) return;
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
}

/**
 * Register a builtin native function with the VM.
 *
 * @param name       Name exposed to Orus programs.
 * @param function   C function implementing the builtin.
 * @param arity      Number of arguments.
 * @param returnType Expected return type or NULL.
 */
void defineNative(const char* name, NativeFn function, int arity, Type* returnType) {
    if (vm.nativeFunctionCount >= MAX_NATIVES) return;
    ObjString* str = allocateString(name, (int)strlen(name));
    NativeFunction nf = {str, function, arity, returnType};
    vm.nativeFunctions[vm.nativeFunctionCount++] = nf;
}

/**
 * Find a registered native function by name.
 *
 * @param name Name string object.
 * @return     Index in the native function table or -1.
 */
int findNative(ObjString* name) {
    for (int i = 0; i < vm.nativeFunctionCount; i++) {
        if (vm.nativeFunctions[i].name->length == name->length &&
            strncmp(vm.nativeFunctions[i].name->chars, name->chars, name->length) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Load and execute a module, caching its exports.
 *
 * @param path Path to the module file.
 * @return     Interpretation result code.
 */
static InterpretResult interpretModule(const char* path) {
    if (traceImports) fprintf(stderr, "[import] interpreting %s\n", path);
    for (int i = 0; i < runtimeStackCount; i++) {
        if (strcmp(runtimeStack[i], path) == 0) {
            RUNTIME_ERROR("Import cycle detected for module '%s'.", path);
            return INTERPRET_RUNTIME_ERROR;
        }
    }
    if (runtimeStackCount < UINT8_MAX)
        runtimeStack[runtimeStackCount++] = path;

    Module* cached = get_module(path);
    if (cached) {
        if (vm.devMode && !cached->from_embedded && cached->disk_path) {
            struct stat st;
            if (stat(cached->disk_path, &st) == 0 && st.st_mtime != cached->mtime) {
                char* src = load_module_source(cached->disk_path);
                if (src) {
                    ASTNode* ast = parse_module_source(src, path);
                    if (ast) {
                        freeChunk(cached->bytecode);
                        cached->bytecode = compile_module_ast(ast, path);
                        cached->mtime = st.st_mtime;
                        cached->executed = false;
                    }
                    free(src);
                }
            }
        }

        if (cached->executed) {
            RUNTIME_ERROR("Module '%s' already executed.", path);
            runtimeStackCount--;
            return INTERPRET_RUNTIME_ERROR;
        }

        Chunk* prevChunk = vm.chunk;
        uint8_t* prevIp = vm.ip;
        InterpretResult res;
        freeRegisterChunk(&vm.regChunk);
        initRegisterChunk(&vm.regChunk);
        chunkToRegisterIR(cached->bytecode, &vm.regChunk);
        initRegisterVM(&vm.regVM, &vm.regChunk);
        runRegisterVM(&vm.regVM);
        res = INTERPRET_OK;
        for (int i = 0; i < cached->export_count; i++) {
            cached->exports[i].value = vm.globals[cached->exports[i].index];
        }
        cached->executed = true;
        vm.chunk = prevChunk;
        vm.ip = prevIp;
        runtimeStackCount--;
        return res;
    }

    char* diskPath = NULL;
    long mtime = 0;
    bool fromEmbedded = false;
    char* source = load_module_with_fallback(path, &diskPath, &mtime, &fromEmbedded);
    if (!source) {
        RUNTIME_ERROR("Module '%s' not found", path);
        runtimeStackCount--;
        return INTERPRET_RUNTIME_ERROR;
    }

    int startGlobals = vm.variableCount;
    char* cacheFile = cache_path_for(path);
    Chunk* chunk = NULL;
    if (cacheFile) {
        long cached_mtime;
        chunk = readChunkFromFile(cacheFile, &cached_mtime);
        if (chunk && cached_mtime != mtime) { freeChunk(chunk); free(chunk); chunk = NULL; }
    }
    ASTNode* ast = NULL;
    if (!chunk) {
        ast = parse_module_source(source, path);
        if (!ast) {
            free(source);
            if (cacheFile) free(cacheFile);
            RUNTIME_ERROR("Parsing failed for module.");
            runtimeStackCount--;
            return INTERPRET_COMPILE_ERROR;
        }

        chunk = compile_module_ast(ast, path);
        if (!chunk) {
            free(source);
            if (cacheFile) free(cacheFile);
            RUNTIME_ERROR("Compilation failed for module.");
            runtimeStackCount--;
            return INTERPRET_COMPILE_ERROR;
        }
        if (cacheFile) writeChunkToFile(chunk, cacheFile, mtime);
    }

    Module mod;
    mod.module_name = strdup(path);
    const char* base = strrchr(path, '/');
    base = base ? base + 1 : path;
    size_t len = strlen(base);
    if (len > 5 && strcmp(base + len - 5, ".orus") == 0) len -= 5;
    mod.name = (char*)malloc(len + 1);
    memcpy(mod.name, base, len);
    mod.name[len] = '\0';
    mod.bytecode = chunk;
    mod.export_count = 0;
    mod.executed = true;
    mod.disk_path = diskPath;
    mod.mtime = mtime;
    mod.from_embedded = fromEmbedded;

    Chunk* prevChunk = vm.chunk;
    uint8_t* prevIp = vm.ip;
    InterpretResult result;
    freeRegisterChunk(&vm.regChunk);
    initRegisterChunk(&vm.regChunk);
    chunkToRegisterIR(chunk, &vm.regChunk);
    initRegisterVM(&vm.regVM, &vm.regChunk);
    runRegisterVM(&vm.regVM);
    result = INTERPRET_OK;

    for (int i = startGlobals; i < vm.variableCount && mod.export_count < UINT8_MAX; i++) {
        Export ex;
        ex.name = vm.variableNames[i].name ? vm.variableNames[i].name->chars : NULL;
        if (ex.name && vm.publicGlobals[i]) {
            ex.name = strdup(ex.name);
            ex.value = vm.globals[i];
            ex.index = i;
            mod.exports[mod.export_count++] = ex;
        }
    }

    register_module(&mod);
    vm.chunk = prevChunk;
    vm.ip = prevIp;

    runtimeStackCount--;
    free(source);
    if (cacheFile) free(cacheFile);
    return result;
}

/**
 * Public API to interpret a module by path.
 *
 * @param path Path of module to execute.
 * @return     Interpretation result code.
 */
InterpretResult interpret_module(const char* path) {
    return interpretModule(path);
}

/**
 * Execute bytecode instructions in the current chunk.
 *
 * @return Interpretation result code.
 */
static InterpretResult run() {
    #define READ_BYTE() (*vm.ip++)
    #define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    
    // Define and reset loop safety counter at the start of each VM run
    // static int absolute_loop_count = 0; // Tracks iterations across OP_LOOP
    // absolute_loop_count = 0;

    InterpretResult result = INTERPRET_OK;

    for (;;) {
        if (vm.trace) traceExecution();
        vm.instruction_count++;
        if (vm.instruction_count % 10000 == 0 && !vm.gcPaused) {
            collectGarbage();
        }
        vm.currentLine = get_line(vm.chunk, (int)(vm.ip - vm.chunk->code));
        vm.currentColumn = get_column(vm.chunk, (int)(vm.ip - vm.chunk->code));
        uint8_t instruction = READ_BYTE();

        switch (instruction) {
            case OP_PRINT: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = vmPop(&vm);
                printValue(value);
                putchar('\n');
                break;
            }

            case OP_PRINT_NO_NL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = vmPop(&vm);
                printValue(value);
                fflush(stdout);
                break;
            }

            case OP_PRINT_I32: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_I32 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int32_t v = AS_I32(vmPop(&vm));
                printf("%d\n", v);
                break;
            }

            case OP_PRINT_I32_NO_NL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_I32 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int32_t v = AS_I32(vmPop(&vm));
                printf("%d", v);
                fflush(stdout);
                break;
            }

            case OP_PRINT_I64: {
                if (vm.stackI64Top <= vm.stackI64) {
                    RUNTIME_ERROR("Stack underflow in PRINT_I64 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int64_t v = vmPopI64(&vm);
                printf("%lld\n", (long long)v);
                break;
            }

            case OP_PRINT_I64_NO_NL: {
                if (vm.stackI64Top <= vm.stackI64) {
                    RUNTIME_ERROR("Stack underflow in PRINT_I64 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int64_t v = vmPopI64(&vm);
                printf("%lld", (long long)v);
                fflush(stdout);
                break;
            }

            case OP_PRINT_U32: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_U32 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint32_t v = AS_U32(vmPop(&vm));
                printf("%u\n", v);
                break;
            }

            case OP_PRINT_U32_NO_NL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_U32 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint32_t v = AS_U32(vmPop(&vm));
                printf("%u", v);
                fflush(stdout);
                break;
            }

            case OP_PRINT_U64: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_U64 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint64_t v = AS_U64(vmPop(&vm));
                printf("%llu\n", (unsigned long long)v);
                break;
            }

            case OP_PRINT_U64_NO_NL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_U64 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                uint64_t v = AS_U64(vmPop(&vm));
                printf("%llu", (unsigned long long)v);
                fflush(stdout);
                break;
            }

            case OP_PRINT_F64: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_F64 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double v = AS_F64(vmPop(&vm));
                printf("%g\n", v);
                break;
            }

            case OP_PRINT_F64_NO_NL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_F64 operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                double v = AS_F64(vmPop(&vm));
                printf("%g", v);
                fflush(stdout);
                break;
            }

            case OP_PRINT_BOOL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_BOOL operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                bool v = AS_BOOL(vmPop(&vm));
                printf("%s\n", v ? "true" : "false");
                break;
            }

            case OP_PRINT_BOOL_NO_NL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_BOOL operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                bool v = AS_BOOL(vmPop(&vm));
                printf("%s", v ? "true" : "false");
                fflush(stdout);
                break;
            }

            case OP_PRINT_STRING: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_STRING operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjString* s = AS_STRING(vmPop(&vm));
                printf("%s\n", s->chars);
                break;
            }

            case OP_PRINT_STRING_NO_NL: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in PRINT_STRING operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjString* s = AS_STRING(vmPop(&vm));
                printf("%s", s->chars);
                fflush(stdout);
                break;
            }

            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                vmPush(&vm, constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                uint32_t index = (uint32_t)(READ_BYTE() << 16);
                index |= (uint32_t)(READ_BYTE() << 8);
                index |= READ_BYTE();
                Value constant = vm.chunk->constants.values[index];
                vmPush(&vm, constant);
                break;
            }
            case OP_I64_CONST: {
                Value constant = READ_CONSTANT();
                vmPushI64(&vm, AS_I64(constant));
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
            case OP_ADD_I64:
            {
                int64_t b = vmPopI64(&vm);
                int64_t a = vmPopI64(&vm);
                int64_t r;
                if (__builtin_add_overflow(a, b, &r)) {
                    handleOverflow("i64 overflow");
                }
                vmPushI64(&vm, r);
                break;
            }
            case OP_SUBTRACT_I64:
            {
                int64_t b = vmPopI64(&vm);
                int64_t a = vmPopI64(&vm);
                int64_t r;
                if (__builtin_sub_overflow(a, b, &r)) {
                    handleOverflow("i64 overflow");
                }
                vmPushI64(&vm, r);
                break;
            }
            case OP_MULTIPLY_I64:
            {
                int64_t b = vmPopI64(&vm);
                int64_t a = vmPopI64(&vm);
                int64_t r;
                if (__builtin_mul_overflow(a, b, &r)) {
                    handleOverflow("i64 overflow");
                }
                vmPushI64(&vm, r);
                break;
            }
            case OP_DIVIDE_I64:
            {
                int64_t b = vmPopI64(&vm);
                int64_t a = vmPopI64(&vm);
                if (b == 0) {
                    vmRuntimeError("Division by zero.");
                    result = INTERPRET_RUNTIME_ERROR;
                } else {
                    vmPushI64(&vm, a / b);
                }
                break;
            }
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
            case OP_ADD_U64:
                binaryOpU64(&vm, '+', &result);
                break;
            case OP_SUBTRACT_U64:
                binaryOpU64(&vm, '-', &result);
                break;
            case OP_MULTIPLY_U64:
                binaryOpU64(&vm, '*', &result);
                break;
            case OP_DIVIDE_U64:
                binaryOpU64(&vm, '/', &result);
                break;
            case OP_MODULO_I32:
                moduloOpI32(&vm, &result);
                break;
            case OP_MODULO_I64:
                moduloOpI64(&vm, &result);
                break;
            case OP_MODULO_U32:
                moduloOpU32(&vm, &result);
                break;
            case OP_MODULO_U64:
                moduloOpU64(&vm, &result);
                break;

            case OP_BIT_AND_I32:
                bitwiseOpI32(&vm, '&', &result);
                break;
            case OP_BIT_AND_I64:
                bitwiseOpI64(&vm, '&', &result);
                break;
            case OP_BIT_AND_U32:
                bitwiseOpU32(&vm, '&', &result);
                break;
            case OP_BIT_OR_I32:
                bitwiseOpI32(&vm, '|', &result);
                break;
            case OP_BIT_OR_I64:
                bitwiseOpI64(&vm, '|', &result);
                break;
            case OP_BIT_OR_U32:
                bitwiseOpU32(&vm, '|', &result);
                break;
            case OP_BIT_XOR_I32:
                bitwiseOpI32(&vm, '^', &result);
                break;
            case OP_BIT_XOR_I64:
                bitwiseOpI64(&vm, '^', &result);
                break;
            case OP_BIT_XOR_U32:
                bitwiseOpU32(&vm, '^', &result);
                break;
            case OP_BIT_NOT_I32:
                bitwiseNotI32(&vm, &result);
                break;
            case OP_BIT_NOT_I64:
                bitwiseNotI64(&vm, &result);
                break;
            case OP_BIT_NOT_U32:
                bitwiseNotU32(&vm, &result);
                break;
            case OP_SHIFT_LEFT_I32:
                shiftLeftI32(&vm, &result);
                break;
            case OP_SHIFT_LEFT_I64:
                shiftLeftI64(&vm, &result);
                break;
            case OP_SHIFT_LEFT_U32:
                shiftLeftU32(&vm, &result);
                break;
            case OP_SHIFT_RIGHT_I32:
                shiftRightI32(&vm, &result);
                break;
            case OP_SHIFT_RIGHT_I64:
                shiftRightI64(&vm, &result);
                break;
            case OP_SHIFT_RIGHT_U32:
                shiftRightU32(&vm, &result);
                break;

            // Comparison operations
            case OP_EQUAL:
                compareOpAny(&vm, '=', &result);
                break;
            case OP_NOT_EQUAL:
                compareOpAny(&vm, '!', &result);
                break;
            case OP_EQUAL_I64: {
                int64_t b = vmPopI64(&vm);
                int64_t a = vmPopI64(&vm);
                vmPush(&vm, BOOL_VAL(a == b));
                break;
            }
            case OP_NOT_EQUAL_I64: {
                int64_t b = vmPopI64(&vm);
                int64_t a = vmPopI64(&vm);
                vmPush(&vm, BOOL_VAL(a != b));
                break;
            }
            case OP_LESS_I32:
                compareOpI32(&vm, '<', &result);
                break;
            case OP_LESS_I64:
                compareOpI64(&vm, '<', &result);
                break;
            case OP_LESS_U32:
                compareOpU32(&vm, '<', &result);

                break;
            case OP_LESS_U64:
                compareOpU64(&vm, '<', &result);

                break;
            case OP_LESS_F64:
                compareOpF64(&vm, '<', &result);
                break;
            case OP_LESS_EQUAL_I32:
                compareOpI32(&vm, 'L', &result);
                break;
            case OP_LESS_EQUAL_I64:
                compareOpI64(&vm, 'L', &result);
                break;
            case OP_LESS_EQUAL_U32:
                compareOpU32(&vm, 'L', &result);
                break;
            case OP_LESS_EQUAL_U64:
                compareOpU64(&vm, 'L', &result);
                break;
            case OP_LESS_EQUAL_F64:
                compareOpF64(&vm, 'L', &result);
                break;
            case OP_LESS_EQUAL_GENERIC:
                compareOpGeneric(&vm, 'L', &result);
                break;
            case OP_LESS_GENERIC:
                compareOpGeneric(&vm, '<', &result);
                break;
            case OP_GREATER_I32:
                compareOpI32(&vm, '>', &result);
                break;
            case OP_GREATER_I64:
                compareOpI64(&vm, '>', &result);
                break;
            case OP_GREATER_U32:
                compareOpU32(&vm, '>', &result);
                break;
            case OP_GREATER_U64:
                compareOpU64(&vm, '>', &result);
                break;
            case OP_GREATER_F64:
                compareOpF64(&vm, '>', &result);
                break;
            case OP_GREATER_GENERIC:
                compareOpGeneric(&vm, '>', &result);
                break;
            case OP_GREATER_EQUAL_I32:
                compareOpI32(&vm, 'G', &result);
                break;
            case OP_GREATER_EQUAL_I64:
                compareOpI64(&vm, 'G', &result);
                break;
            case OP_GREATER_EQUAL_U32:
                compareOpU32(&vm, 'G', &result);
                break;
            case OP_GREATER_EQUAL_U64:
                compareOpU64(&vm, 'G', &result);
                break;
            case OP_GREATER_EQUAL_F64:
                compareOpF64(&vm, 'G', &result);
                break;
            case OP_GREATER_EQUAL_GENERIC:
                compareOpGeneric(&vm, 'G', &result);
                break;
            case OP_ADD_GENERIC:
                binaryOpGeneric(&vm, '+', &result);
                break;
            case OP_SUBTRACT_GENERIC:
                binaryOpGeneric(&vm, '-', &result);
                break;
            case OP_MULTIPLY_GENERIC:
                binaryOpGeneric(&vm, '*', &result);
                break;
            case OP_DIVIDE_GENERIC:
                binaryOpGeneric(&vm, '/', &result);
                break;
            case OP_MODULO_GENERIC:
                moduloOpGeneric(&vm, &result);
                break;
            case OP_NEGATE_GENERIC:
                negateGeneric(&vm, &result);
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
                int32_t value = AS_I32(vmPop(&vm));
                vmPush(&vm, I32_VAL(-value));
                break;
            }
            case OP_NEGATE_I64: {
                int64_t value = vmPopI64(&vm);
                vmPushI64(&vm, -value);
                break;
            }
            case OP_INC_I64: {
                int64_t value = vmPopI64(&vm);
                int64_t resultVal;
                if (__builtin_add_overflow(value, 1, &resultVal)) {
                    handleOverflow("i64 overflow");
                }
                vmPushI64(&vm, resultVal);
                break;
            }
            case OP_ITER_NEXT_I64: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in ITER_NEXT_I64.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value iterVal = vmPop(&vm);
                if (!IS_RANGE_ITERATOR(iterVal)) {
                    RUNTIME_ERROR("ITER_NEXT_I64 expects range iterator.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjRangeIterator* it = AS_RANGE_ITERATOR(iterVal);
                if (it->current >= it->end) {
                    vmPush(&vm, BOOL_VAL(false));
                } else {
                    int64_t v = it->current++;
                    vmPushI64(&vm, v);
                    vmPush(&vm, BOOL_VAL(true));
                }
                break;
            }
            case OP_GC_PAUSE: {
                pauseGC();
                break;
            }
            case OP_GC_RESUME: {
                resumeGC();
                break;
            }
            case OP_NEGATE_U32: {
                uint32_t value = AS_U32(vmPop(&vm));
                vmPush(&vm, U32_VAL(-value));
                break;
            }
            case OP_NEGATE_U64: {
                uint64_t value = AS_U64(vmPop(&vm));
                vmPush(&vm, U64_VAL(-value));
                break;
            }
            case OP_NEGATE_F64: {
                double value = AS_F64(vmPop(&vm));
                vmPush(&vm, F64_VAL(-value));
                break;
            }
            case OP_ADD_NUMERIC:
                binaryOpNumeric(&vm, '+', &result);
                break;
            case OP_SUBTRACT_NUMERIC:
                binaryOpNumeric(&vm, '-', &result);
                break;
            case OP_MULTIPLY_NUMERIC:
                binaryOpNumeric(&vm, '*', &result);
                break;
            case OP_DIVIDE_NUMERIC:
                binaryOpNumeric(&vm, '/', &result);
                break;
            case OP_NEGATE_NUMERIC:
                negateNumeric(&vm, &result);
                break;
            case OP_MODULO_NUMERIC:
                moduloOpNumeric(&vm, &result);
                break;
            case OP_I32_TO_F64: {
                Value value = vmPop(&vm);
                InterpretResult convResult = INTERPRET_OK;
                double floatValue = convertToF64(&vm, value, &convResult);
                if (convResult != INTERPRET_OK) {
                    RUNTIME_ERROR("Failed to convert value to float.");
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
                    RUNTIME_ERROR("Failed to convert value to float.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPush(&vm, F64_VAL(floatValue));
                break;
            }
            case OP_I32_TO_U32: {
                int32_t value = AS_I32(vmPop(&vm));
                vmPush(&vm, U32_VAL((uint32_t)value));
                break;
            }
            case OP_U32_TO_I32: {
                uint32_t value = AS_U32(vmPop(&vm));
                vmPush(&vm, I32_VAL((int32_t)value));
                break;
            }
            case OP_I32_TO_I64: {
                int32_t value = AS_I32(vmPop(&vm));
                vmPushI64(&vm, (int64_t)value);
                break;
            }
            case OP_U32_TO_I64: {
                uint32_t value = AS_U32(vmPop(&vm));
                vmPushI64(&vm, (int64_t)value);
                break;
            }
            case OP_I64_TO_I32: {
                int64_t value = vmPopI64(&vm);
                vmPush(&vm, I32_VAL((int32_t)value));
                break;
            }
            case OP_I64_TO_U32: {
                int64_t value = vmPopI64(&vm);
                vmPush(&vm, U32_VAL((uint32_t)value));
                break;
            }
            case OP_I32_TO_U64: {
                int32_t value = AS_I32(vmPop(&vm));
                vmPush(&vm, U64_VAL((uint64_t)value));
                break;
            }
            case OP_U32_TO_U64: {
                uint32_t value = AS_U32(vmPop(&vm));
                vmPush(&vm, U64_VAL((uint64_t)value));
                break;
            }
            case OP_U64_TO_I32: {
                uint64_t value = AS_U64(vmPop(&vm));
                vmPush(&vm, I32_VAL((int32_t)value));
                break;
            }
            case OP_U64_TO_U32: {
                uint64_t value = AS_U64(vmPop(&vm));
                vmPush(&vm, U32_VAL((uint32_t)value));
                break;
            }
            case OP_U64_TO_F64: {
                uint64_t value = AS_U64(vmPop(&vm));
                vmPush(&vm, F64_VAL((double)value));
                break;
            }
            case OP_F64_TO_U64: {
                double value = AS_F64(vmPop(&vm));
                vmPush(&vm, U64_VAL((uint64_t)value));
                break;
            }
            case OP_F64_TO_I32: {
                double value = AS_F64(vmPop(&vm));
                vmPush(&vm, I32_VAL((int32_t)value));
                break;
            }
            case OP_F64_TO_U32: {
                double value = AS_F64(vmPop(&vm));
                vmPush(&vm, U32_VAL((uint32_t)value));
                break;
            }
            case OP_I64_TO_U64: {
                int64_t value = vmPopI64(&vm);
                vmPush(&vm, U64_VAL((uint64_t)value));
                break;
            }
            case OP_U64_TO_I64: {
                uint64_t value = AS_U64(vmPop(&vm));
                vmPushI64(&vm, (int64_t)value);
                break;
            }
            case OP_I64_TO_F64: {
                int64_t value = vmPopI64(&vm);
                vmPush(&vm, F64_VAL((double)value));
                break;
            }
            case OP_F64_TO_I64: {
                double value = AS_F64(vmPop(&vm));
                vmPushI64(&vm, (int64_t)value);
                break;
            }
            case OP_I32_TO_BOOL: {
                int32_t value = AS_I32(vmPop(&vm));
                vmPush(&vm, BOOL_VAL(value != 0));
                break;
            }
            case OP_U32_TO_BOOL: {
                uint32_t value = AS_U32(vmPop(&vm));
                vmPush(&vm, BOOL_VAL(value != 0));
                break;
            }
            case OP_I64_TO_BOOL: {
                int64_t value = vmPopI64(&vm);
                vmPush(&vm, BOOL_VAL(value != 0));
                break;
            }
            case OP_U64_TO_BOOL: {
                uint64_t value = AS_U64(vmPop(&vm));
                vmPush(&vm, BOOL_VAL(value != 0));
                break;
            }
            case OP_BOOL_TO_I32: {
                bool value = AS_BOOL(vmPop(&vm));
                vmPush(&vm, I32_VAL(value ? 1 : 0));
                break;
            }
            case OP_BOOL_TO_U32: {
                bool value = AS_BOOL(vmPop(&vm));
                vmPush(&vm, U32_VAL(value ? 1 : 0));
                break;
            }
            case OP_BOOL_TO_I64: {
                bool value = AS_BOOL(vmPop(&vm));
                vmPushI64(&vm, value ? 1 : 0);
                break;
            }
            case OP_BOOL_TO_U64: {
                bool value = AS_BOOL(vmPop(&vm));
                vmPush(&vm, U64_VAL(value ? 1 : 0));
                break;
            }
            case OP_BOOL_TO_F64: {
                bool value = AS_BOOL(vmPop(&vm));
                vmPush(&vm, F64_VAL(value ? 1.0 : 0.0));
                break;
            }
            case OP_F64_TO_BOOL: {
                double value = AS_F64(vmPop(&vm));
                vmPush(&vm, BOOL_VAL(value != 0.0));
                break;
            }
            case OP_I64_TO_STRING: {
                int64_t value = vmPopI64(&vm);
                Value v = I64_VAL(value);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_U64_TO_STRING: {
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_I32_TO_STRING: {
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_U32_TO_STRING: {
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_F64_TO_STRING: {
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_BOOL_TO_STRING: {
                Value v = vmPop(&vm);
                vmPush(&vm, convertToString(v));
                break;
            }
            case OP_ARRAY_TO_STRING: {
                Value v = vmPop(&vm);
                int capacity = 64;
                int length = 0;
                char* buffer = (char*)malloc(capacity);
                if (!buffer) {
                    RUNTIME_ERROR("Memory allocation failed for array string conversion.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                buffer[0] = '\0';
                if (!appendValueString(v, &buffer, &length, &capacity)) {
                    free(buffer);
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjString* str = allocateString(buffer, length);
                free(buffer);
                vmPush(&vm, STRING_VAL(str));
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
                Value returnValue = NIL_VAL;
                bool hasValue = vm.stackTop > vm.stack;
                if (hasValue) {
                    returnValue = vmPop(&vm);
                }

                // Determine the declared return type of the current function
                Type* declared = NULL;
                if (vm.frameCount > 0) {
                    uint8_t funcIndex = vm.frames[vm.frameCount - 1].functionIndex;
                    if (vm.globalTypes[funcIndex] &&
                        vm.globalTypes[funcIndex]->kind == TYPE_FUNCTION) {
                        declared = vm.globalTypes[funcIndex]->info.function.returnType;
                    }
                }

                // For functions declared with a void return type, ignore any
                // leftover value and return nil instead.
                if (declared == NULL || declared->kind == TYPE_VOID) {
                    returnValue = NIL_VAL;
                }

                // If we're in a function call, restore the call frame
                if (vm.frameCount > 0) {
                    // Restore the previous call frame
                    CallFrame* frame = &vm.frames[--vm.frameCount];

                    // Restore the instruction pointer and chunk
                    vm.chunk = frame->previousChunk;
                    vm.ip = frame->returnAddress;

                    // Reset the stack to the frame's base and rebuild the i64 stack
                    if (frame->stackOffset >= 0 && frame->stackOffset < vm.stackCapacity) {
                        vm.stackTop = vm.stack + frame->stackOffset;
                    } else {
                        vm.stackTop = vm.stack;
                    }
                    vm.stackI64Top = vm.stackI64;
                    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                        if (IS_I64(*slot)) {
                            *vm.stackI64Top = AS_I64(*slot);
                            vm.stackI64Top++;
                        }
                    }
                    if (IS_I64(returnValue)) {
                        vmPushI64(&vm, AS_I64(returnValue));
                    } else {
                        vmPush(&vm, returnValue);
                    }

                    if (vm.trace) {
#ifdef DEBUG_TRACE_EXECUTION
                        printf("OUTPUT: Function returned: ");
                        printValue(returnValue);
                        printf("\n");
                        fflush(stdout);
#endif
                    }
                } else {
                    // If we're not in a function call, optionally push the return value back
                    if (IS_I64(returnValue)) {
                        vmPushI64(&vm, AS_I64(returnValue));
                    } else {
                        vmPush(&vm, returnValue);
                    }

                    return INTERPRET_OK;
                }


                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint8_t index = READ_BYTE();

                // Check for stack underflow
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in DEFINE_GLOBAL.");
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
                    RUNTIME_ERROR("Attempt to access undefined variable.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value value = vm.globals[index];
                if (vm.globalTypes[index] && vm.globalTypes[index]->kind == TYPE_I64) {
                    vmPushI64(&vm, AS_I64(value));
                } else {
                    vmPush(&vm, value);
                }

                break;
            }
            case OP_SET_GLOBAL: {
                uint8_t index = READ_BYTE();
                if (index >= vm.variableCount || vm.globalTypes[index] == NULL) {
                    RUNTIME_ERROR("Attempt to assign to undefined variable.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                // Peek at the value instead of popping it - this is safer for stack management
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in SET_GLOBAL operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value value = vmPeek(&vm, 0);

                // Enhanced debug information for variables
                if (index < vm.variableCount && vm.variableNames[index].name != NULL) {
                    char varName[256] = {0};
                    strncpy(varName, vm.variableNames[index].name->chars,
                            vm.variableNames[index].length);
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
                            case VAL_U64:
                                // fprintf(stderr, "[U64:%llu]", (unsigned long long)AS_U64(value));
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

                // Runtime type enforcement
                if (!checkValueAgainstType(value, vm.globalTypes[index])) {
                    RUNTIME_ERROR("Type mismatch for variable assignment.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Store the value in the global variable
                vm.globals[index] = value;

                break;
            }
            case OP_IMPORT: {
                uint8_t constantIndex = READ_BYTE();
                Value pathVal = vm.chunk->constants.values[constantIndex];
                if (!IS_STRING(pathVal)) {
                    RUNTIME_ERROR("Import path must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjString* pathStr = AS_STRING(pathVal);
                bool already = false;
                for (int i = 0; i < vm.moduleCount; i++) {
                    if (strcmp(vm.loadedModules[i]->chars, pathStr->chars) == 0) {
                        already = true;
                        break;
                    }
                }
                if (already) {
                    RUNTIME_ERROR("Module '%s' already executed.", pathStr->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                InterpretResult modRes = interpretModule(pathStr->chars);
                if (modRes != INTERPRET_OK) return modRes;
                if (vm.moduleCount < UINT8_MAX)
                    vm.loadedModules[vm.moduleCount++] = pathStr;
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
                    RUNTIME_ERROR("Condition must be a boolean.");
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
                    RUNTIME_ERROR("Condition must be a boolean.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (AS_BOOL(condition)) {
                    vm.ip += offset;
                }

                break;
            }
            case OP_JUMP_IF_LT_I64: {
                uint16_t offset = READ_SHORT();
                if (vm.stackI64Top - vm.stackI64 < 2) {
                    RUNTIME_ERROR("Stack underflow in JUMP_IF_LT_I64.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int64_t b = vmPopI64(&vm);
                int64_t a = vmPopI64(&vm);
                if (a >= b) {
                    vm.ip += offset;
                }

                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                
                // Use the global loop counter defined at the start of run()
                // absolute_loop_count++;
                
                // If a loop iteration limit is defined, guard against runaway
                // loops. The check is compiled out when the limit is disabled.
#ifdef LOOP_ITERATION_LIMIT
                if (absolute_loop_count > LOOP_ITERATION_LIMIT) {
                    fprintf(stderr, "ERROR: Loop iteration limit exceeded (%d)."
                            " Forced termination to prevent infinite loop.\n",
                            LOOP_ITERATION_LIMIT);
                    return INTERPRET_RUNTIME_ERROR;
                }
#endif
                vm.ip -= offset;

                break;
            }
            case OP_BREAK: {
                // Break is handled at compile time by emitting a jump
                // This opcode should never be executed
                RUNTIME_ERROR("Unexpected OP_BREAK.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_CONTINUE: {
                // Continue is handled at compile time by emitting a loop
                // This opcode should never be executed
                RUNTIME_ERROR("Unexpected OP_CONTINUE.");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_SETUP_EXCEPT: {
                uint16_t offset = READ_SHORT();
                uint8_t varIndex = READ_BYTE();
                if (vm.tryFrameCount >= TRY_MAX) {
                    RUNTIME_ERROR("Too many nested try blocks.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm.tryFrames[vm.tryFrameCount].handler = vm.ip + offset;
                vm.tryFrames[vm.tryFrameCount].varIndex = varIndex;
                vm.tryFrames[vm.tryFrameCount].stackDepth = (int)(vm.stackTop - vm.stack);
                vm.tryFrameCount++;
                break;
            }
            case OP_POP_EXCEPT: {
                if (vm.tryFrameCount > 0) vm.tryFrameCount--;
                break;
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
                    RUNTIME_ERROR("Can only index arrays.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int idx;
                if (IS_I32(indexVal)) {
                    idx = AS_I32(indexVal);
                } else if (IS_U32(indexVal)) {
                    idx = (int)AS_U32(indexVal);
                } else {
                    RUNTIME_ERROR("Array index must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = AS_ARRAY(arrayVal);
                if (idx < 0 || idx >= arr->length) {
                    RUNTIME_ERROR("Array index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value elem = arr->elements[idx];
                if (IS_I64(elem)) {
                    vmPushI64(&vm, AS_I64(elem));
                } else {
                    vmPush(&vm, elem);
                }
                break;
            }
            case OP_ARRAY_SET: {
                Value value = vmPop(&vm);
                Value indexVal = vmPop(&vm);
                Value arrayVal = vmPop(&vm);
                if (!IS_ARRAY(arrayVal)) {
                    RUNTIME_ERROR("Can only index arrays.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int idx;
                if (IS_I32(indexVal)) {
                    idx = AS_I32(indexVal);
                } else if (IS_U32(indexVal)) {
                    idx = (int)AS_U32(indexVal);
                } else {
                    RUNTIME_ERROR("Array index must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = AS_ARRAY(arrayVal);
                if (idx < 0 || idx >= arr->length) {
                    RUNTIME_ERROR("Array index out of bounds.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                arr->elements[idx] = value;
                if (IS_I64(value)) {
                    vmPushI64(&vm, AS_I64(value));
                } else {
                    vmPush(&vm, value);
                }
                break;
            }
            case OP_ARRAY_PUSH: {
                Value value = vmPop(&vm);
                Value arrayVal = vmPop(&vm);
                if (!IS_ARRAY(arrayVal)) {
                    RUNTIME_ERROR("Can only push to arrays.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = AS_ARRAY(arrayVal);
                arrayPush(&vm, arr, value);
                vmPush(&vm, arrayVal);
                break;
            }
            case OP_ARRAY_POP: {
                Value arrayVal = vmPop(&vm);
                if (!IS_ARRAY(arrayVal)) {
                    RUNTIME_ERROR("Can only pop from arrays.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjArray* arr = AS_ARRAY(arrayVal);
                Value v = arrayPop(arr);
                if (IS_I64(v)) {
                    vmPushI64(&vm, AS_I64(v));
                } else {
                    vmPush(&vm, v);
                }
                break;
            }
            case OP_ARRAY_RESERVE: {
                Value capVal = vmPop(&vm);
                Value arrayVal = vmPop(&vm);
                if (!IS_ARRAY(arrayVal)) {
                    RUNTIME_ERROR("First argument to reserve() must be array.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!(IS_I32(capVal) || IS_I64(capVal) || IS_U32(capVal) ||
                      IS_U64(capVal))) {
                    RUNTIME_ERROR("reserve() expects integer capacity.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int cap = IS_I32(capVal)  ? AS_I32(capVal)  :
                          IS_I64(capVal)  ? (int)AS_I64(capVal)  :
                          IS_U32(capVal)  ? (int)AS_U32(capVal) :
                                            (int)AS_U64(capVal);
                if (cap > 0) {
                    ObjArray* arr = AS_ARRAY(arrayVal);
                    if (cap > arr->capacity) {
                        int oldCap = arr->capacity;
                        arr->capacity = cap;
                        arr->elements =
                            GROW_ARRAY(Value, arr->elements, oldCap, arr->capacity);
                        vm.bytesAllocated += sizeof(Value) * (arr->capacity - oldCap);
                    }
                }
                vmPush(&vm, arrayVal);
                break;
            }
            case OP_LEN: {
                Value val = vmPop(&vm);
                if (IS_ARRAY(val)) {
                    vmPush(&vm, I32_VAL(AS_ARRAY(val)->length));
                } else if (IS_STRING(val)) {
                    vmPush(&vm, I32_VAL(AS_STRING(val)->length));
                } else {
                    RUNTIME_ERROR("len() expects array or string.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_LEN_ARRAY: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in LEN_ARRAY operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value val = vmPop(&vm);
                vmPush(&vm, I32_VAL(AS_ARRAY(val)->length));
                break;
            }
            case OP_LEN_STRING: {
                if (vm.stackTop <= vm.stack) {
                    RUNTIME_ERROR("Stack underflow in LEN_STRING operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value val = vmPop(&vm);
                vmPush(&vm, I32_VAL(AS_STRING(val)->length));
                break;
            }
            case OP_SUBSTRING: {
                Value lenVal = vmPop(&vm);
                Value startVal = vmPop(&vm);
                Value strVal = vmPop(&vm);

                if (!IS_STRING(strVal) || !IS_I32(startVal) || !IS_I32(lenVal)) {
                    RUNTIME_ERROR("substring() expects (string, i32, i32).");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjString* str = AS_STRING(strVal);
                int start = AS_I32(startVal);
                int length = AS_I32(lenVal);

                if (start < 0) start = 0;
                if (start > str->length) start = str->length;
                if (length < 0) length = 0;
                if (start + length > str->length) length = str->length - start;

                ObjString* result = allocateString(str->chars + start, length);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_SLICE: {
                Value endVal = vmPop(&vm);
                Value startVal = vmPop(&vm);
                Value arrayVal = vmPop(&vm);

                if (!IS_ARRAY(arrayVal) ||
                    !(IS_I32(startVal) || IS_U32(startVal) || IS_NIL(startVal)) ||
                    !(IS_I32(endVal) || IS_U32(endVal) || IS_NIL(endVal))) {
                    RUNTIME_ERROR("slice expects (array, i32?, i32?).");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjArray* src = AS_ARRAY(arrayVal);
                int start = 0;
                int end = src->length;
                if (!IS_NIL(startVal)) {
                    start = IS_I32(startVal) ? AS_I32(startVal) : (int)AS_U32(startVal);
                }
                if (!IS_NIL(endVal)) {
                    end = IS_I32(endVal) ? AS_I32(endVal) : (int)AS_U32(endVal);
                }
                if (start < 0) start = 0;
                if (end > src->length) end = src->length;
                if (start > end) start = end;
                int len = end - start;

                ObjArray* result = allocateArray(len);
                result->length = len;
                for (int i = 0; i < len; i++) {
                    result->elements[i] = src->elements[start + i];
                }
                vmPush(&vm, ARRAY_VAL(result));
                break;
            }
            case OP_TYPE_OF_I32: {
                ObjString* result = allocateString("i32", 3);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_TYPE_OF_I64: {
                ObjString* result = allocateString("i64", 3);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_TYPE_OF_U32: {
                ObjString* result = allocateString("u32", 3);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_TYPE_OF_U64: {
                ObjString* result = allocateString("u64", 3);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_TYPE_OF_F64: {
                ObjString* result = allocateString("f64", 3);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_TYPE_OF_BOOL: {
                ObjString* result = allocateString("bool", 4);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_TYPE_OF_STRING: {
                ObjString* result = allocateString("string", 6);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_TYPE_OF_ARRAY: {
                ObjString* result = allocateString("array", 5);
                vmPush(&vm, STRING_VAL(result));
                break;
            }
            case OP_CALL: {
                uint8_t globalIndex = READ_BYTE();
                uint8_t argCount = READ_BYTE();

                // Global must contain a function index
                if (globalIndex >= vm.variableCount || !IS_I32(vm.globals[globalIndex])) {
                    RUNTIME_ERROR("Attempt to call a non-function.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int32_t funcIndex = AS_I32(vm.globals[globalIndex]);
                if (funcIndex < 0 || funcIndex >= vm.functionCount) {
                    RUNTIME_ERROR("Invalid function index.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Function* fn = &vm.functions[funcIndex];
                if (argCount != fn->arity) {
                    RUNTIME_ERROR("Function called with wrong number of arguments.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Check call stack limit
                if (vm.frameCount >= FRAMES_MAX) {
                    RUNTIME_ERROR("Stack overflow.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Set up new call frame
                CallFrame* frame = &vm.frames[vm.frameCount++];
                frame->returnAddress = vm.ip;
                frame->previousChunk = vm.chunk;
                
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

                // Switch to the function's chunk and jump to its body
                vm.chunk = fn->chunk;
                vm.ip = fn->chunk->code + fn->start;

                break;
            }
            case OP_CALL_NATIVE: {
                uint8_t index = READ_BYTE();
                uint8_t argCount = READ_BYTE();

                if (index >= vm.nativeFunctionCount) {
                    RUNTIME_ERROR("Invalid native function index.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                NativeFunction* nf = &vm.nativeFunctions[index];
                if (nf->arity != -1 && argCount != nf->arity) {
                    RUNTIME_ERROR("Native function called with wrong number of arguments.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                int i64ArgsCount = 0;
                for (int i = 0; i < argCount; i++) {
                    Value* slot = vm.stackTop - argCount + i;
                    if (IS_I64(*slot)) i64ArgsCount++;
                }
                Value result = nf->function(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount;
                vm.stackI64Top -= i64ArgsCount;
                if (IS_ERROR(vm.lastError)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPush(&vm, result);
                break;
            }
            case OP_FORMAT_PRINT: {
                // Ensure stack has at least format string and argument count
                if (vm.stackTop - vm.stack < 2) {
                    RUNTIME_ERROR("FORMAT_PRINT expects format string and argument count "
                        "on stack.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value countValue = vm.stackTop[-1];
                if (!IS_I32(countValue)) {
                    RUNTIME_ERROR("Argument count must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int argCount = AS_I32(countValue);

                if (vm.stackTop - vm.stack < argCount + 2) {
                    RUNTIME_ERROR("Not enough arguments for string interpolation: missing argument values for format string.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value formatValue = vm.stackTop[-argCount-2];

                if (!IS_STRING(formatValue)) {
                    RUNTIME_ERROR("Format string must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjString* formatStr = AS_STRING(formatValue);

                // Allocate buffer
                int resultCapacity = formatStr->length * 2;
                char* resultBuffer = (char*)malloc(resultCapacity);
                if (!resultBuffer) {
                    RUNTIME_ERROR("Memory allocation failed for print buffer.");
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
                            SrcLocation location = {vm.filePath, vm.currentLine, vm.currentColumn};
                            runtimeError(ERROR_TYPE, location,
                                "Too few arguments for string interpolation: format string has %d placeholder%s but only %d argument%s provided",
                                argIndex + 1, argIndex + 1 == 1 ? "" : "s",
                                argCount, argCount == 1 ? "" : "s");
                            free(resultBuffer);
                            return INTERPRET_RUNTIME_ERROR;
                        }

                        Value arg = vm.stack[(int)(vm.stackTop - vm.stack) -
                                             argCount - 1 + argIndex];

                        char valueStr[100];
                        int valueLen = 0;

                        switch (arg.type) {
                            case VAL_I32:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "%d", AS_I32(arg));
                                break;
                            case VAL_I64:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "%lld", (long long)AS_I64(arg));
                                break;
                            case VAL_U32:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "%u", AS_U32(arg));
                                break;
                            case VAL_U64:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "%llu", (unsigned long long)AS_U64(arg));
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
                                // Suppress printing of nil values in
                                // formatted output.
                                valueLen = 0;
                                break;
                            case VAL_STRING: {
                                valueLen = AS_STRING(arg)->length;
                                if (resultLength + valueLen >= resultCapacity) {
                                    resultCapacity =
                                        (resultLength + valueLen) * 2;
                                    resultBuffer = (char*)realloc(
                                        resultBuffer, resultCapacity);
                                    if (!resultBuffer) {
                                        RUNTIME_ERROR("Memory reallocation failed for "
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
                            case VAL_ARRAY: {
                                if (!appendValueString(arg, &resultBuffer,
                                                      &resultLength,
                                                      &resultCapacity)) {
                                    free(resultBuffer);
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                valueLen = 0;
                                break;
                            }
                            case VAL_ERROR:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "Error(%d): %s",
                                                    AS_ERROR(arg)->type,
                                                    AS_ERROR(arg)->message->chars);
                                break;
                            case VAL_RANGE_ITERATOR:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "<range %lld..%lld>",
                                                    (long long)AS_RANGE_ITERATOR(arg)->current,
                                                    (long long)AS_RANGE_ITERATOR(arg)->end);
                                break;
                        }

                        if (valueLen > 0) {
                            if (resultLength + valueLen >= resultCapacity) {
                                resultCapacity = (resultLength + valueLen) * 2;
                                resultBuffer = (char*)realloc(resultBuffer,
                                                              resultCapacity);
                                if (!resultBuffer) {
                                    RUNTIME_ERROR("Memory reallocation failed for value "
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
                                RUNTIME_ERROR("Memory reallocation failed while copying "
                                    "character.");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        resultBuffer[resultLength++] =
                            formatStr->chars[formatIndex++];
                    }
                }

                if (argIndex < argCount) {
                    SrcLocation location = {vm.filePath, vm.currentLine, vm.currentColumn};
                    runtimeError(ERROR_TYPE, location, 
                        "Too many arguments for string interpolation: format string has %d placeholder%s but %d argument%s provided",
                        argIndex, argIndex == 1 ? "" : "s",
                        argCount, argCount == 1 ? "" : "s");

                    free(resultBuffer);
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Null-terminate
                if (resultLength + 1 >= resultCapacity) {
                    resultCapacity = (resultLength + 1) * 2;
                    resultBuffer = (char*)realloc(resultBuffer, resultCapacity);
                    if (!resultBuffer) {
                        RUNTIME_ERROR("Memory reallocation failed during final "
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

            case OP_FORMAT_PRINT_NO_NL: {
                if (vm.stackTop - vm.stack < 2) {
                    RUNTIME_ERROR("FORMAT_PRINT expects format string and argument count on stack.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value countValue = vm.stackTop[-1];
                if (!IS_I32(countValue)) {
                    RUNTIME_ERROR("Argument count must be an integer.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int argCount = AS_I32(countValue);
                if (vm.stackTop - vm.stack < argCount + 2) {
                    RUNTIME_ERROR("Not enough arguments for string interpolation: missing argument values for format string.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value formatValue = vm.stackTop[-argCount-2];
                if (!IS_STRING(formatValue)) {
                    RUNTIME_ERROR("Format string must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjString* formatStr = AS_STRING(formatValue);
                int resultCapacity = formatStr->length * 2;
                char* resultBuffer = (char*)malloc(resultCapacity);
                if (!resultBuffer) {
                    RUNTIME_ERROR("Memory allocation failed for print buffer.");
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
                            SrcLocation location = {vm.filePath, vm.currentLine, vm.currentColumn};
                            runtimeError(ERROR_TYPE, location,
                                "Too few arguments for string interpolation: format string has %d placeholder%s but only %d argument%s provided",
                                argIndex + 1, argIndex + 1 == 1 ? "" : "s",
                                argCount, argCount == 1 ? "" : "s");
                            free(resultBuffer);
                            return INTERPRET_RUNTIME_ERROR;
                        }

                        Value arg = vm.stack[(int)(vm.stackTop - vm.stack) - argCount - 1 + argIndex];

                        char valueStr[100];
                        int valueLen = 0;

                        switch (arg.type) {
                            case VAL_I32:
                                valueLen = snprintf(valueStr, sizeof(valueStr), "%d", AS_I32(arg));
                                break;
                            case VAL_I64:
                                valueLen = snprintf(valueStr, sizeof(valueStr), "%lld", (long long)AS_I64(arg));
                                break;
                            case VAL_U32:
                                valueLen = snprintf(valueStr, sizeof(valueStr), "%u", AS_U32(arg));
                                break;
                            case VAL_U64:
                                valueLen = snprintf(valueStr, sizeof(valueStr), "%llu", (unsigned long long)AS_U64(arg));
                                break;
                            case VAL_F64:
                                valueLen = snprintf(valueStr, sizeof(valueStr), "%g", AS_F64(arg));
                                break;
                            case VAL_BOOL:
                                valueLen = snprintf(valueStr, sizeof(valueStr), "%s", AS_BOOL(arg) ? "true" : "false");
                                break;
                            case VAL_NIL:
                                // Suppress printing of nil values in formatted output.
                                valueLen = 0;
                                break;
                            case VAL_STRING: {
                                valueLen = AS_STRING(arg)->length;
                                if (resultLength + valueLen >= resultCapacity) {
                                    resultCapacity = (resultLength + valueLen) * 2;
                                    resultBuffer = (char*)realloc(resultBuffer, resultCapacity);
                                    if (!resultBuffer) {
                                        RUNTIME_ERROR("Memory reallocation failed for string argument.");
                                        return INTERPRET_RUNTIME_ERROR;
                                    }
                                }
                                memcpy(resultBuffer + resultLength, AS_STRING(arg)->chars, valueLen);
                                resultLength += valueLen;
                                valueLen = 0;
                                break;
                            }
                            case VAL_ARRAY: {
                                if (!appendValueString(arg, &resultBuffer, &resultLength, &resultCapacity)) {
                                    free(resultBuffer);
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                valueLen = 0;
                                break;
                            }
                            case VAL_ERROR:
                                valueLen = snprintf(valueStr, sizeof(valueStr), "Error(%d): %s", AS_ERROR(arg)->type, AS_ERROR(arg)->message->chars);
                                break;
                            case VAL_RANGE_ITERATOR:
                                valueLen = snprintf(valueStr, sizeof(valueStr),
                                                    "<range %lld..%lld>",
                                                    (long long)AS_RANGE_ITERATOR(arg)->current,
                                                    (long long)AS_RANGE_ITERATOR(arg)->end);
                                break;
                        }

                        if (valueLen > 0) {
                            if (resultLength + valueLen >= resultCapacity) {
                                resultCapacity = (resultLength + valueLen) * 2;
                                resultBuffer = (char*)realloc(resultBuffer, resultCapacity);
                                if (!resultBuffer) {
                                    RUNTIME_ERROR("Memory reallocation failed for value conversion.");
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                            }
                            memcpy(resultBuffer + resultLength, valueStr, valueLen);
                            resultLength += valueLen;
                        }

                        formatIndex += 2;
                        argIndex++;

                    } else {
                        if (resultLength + 1 >= resultCapacity) {
                            resultCapacity = (resultLength + 1) * 2;
                            resultBuffer = (char*)realloc(resultBuffer, resultCapacity);
                            if (!resultBuffer) {
                                RUNTIME_ERROR("Memory reallocation failed while copying character.");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        resultBuffer[resultLength++] = formatStr->chars[formatIndex++];
                    }
                }

                if (argIndex < argCount) {
                    SrcLocation location = {vm.filePath, vm.currentLine, vm.currentColumn};
                    runtimeError(ERROR_TYPE, location,
                        "Too many arguments for string interpolation: format string has %d placeholder%s but %d argument%s provided",
                        argIndex, argIndex == 1 ? "" : "s",
                        argCount, argCount == 1 ? "" : "s");

                    free(resultBuffer);
                    return INTERPRET_RUNTIME_ERROR;
                }

                if (resultLength + 1 >= resultCapacity) {
                    resultCapacity = (resultLength + 1) * 2;
                    resultBuffer = (char*)realloc(resultBuffer, resultCapacity);
                    if (!resultBuffer) {
                        RUNTIME_ERROR("Memory reallocation failed during final null-termination.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                resultBuffer[resultLength] = '\0';

                printf("%s", resultBuffer);
                fflush(stdout);

                for (int i = 0; i < argCount + 2; i++) {
                    vmPop(&vm);
                }

                free(resultBuffer);
                break;
            }
            case OP_AND: {
                // Check for stack underflow
                if (vm.stackTop - vm.stack < 2) {
                    RUNTIME_ERROR("Stack underflow in AND operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Peek at the two values without popping
                Value right = vmPeek(&vm, 0);  // Top of stack
                Value left = vmPeek(&vm, 1);   // Second from top

                // Ensure both values are booleans
                if (!IS_BOOL(left) || !IS_BOOL(right)) {
                    RUNTIME_ERROR("AND operation requires boolean operands.");
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
                    RUNTIME_ERROR("Stack underflow in OR operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Peek at the two values without popping
                Value right = vmPeek(&vm, 0);  // Top of stack
                Value left = vmPeek(&vm, 1);   // Second from top

                // Ensure both values are booleans
                if (!IS_BOOL(left) || !IS_BOOL(right)) {
                    RUNTIME_ERROR("OR operation requires boolean operands.");
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
            case OP_NOT: {
                if (vm.stackTop - vm.stack < 1) {
                    RUNTIME_ERROR("Stack underflow in NOT operation.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = vmPop(&vm);
                if (!IS_BOOL(value)) {
                    RUNTIME_ERROR("NOT operation requires boolean operand.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vmPush(&vm, BOOL_VAL(!AS_BOOL(value)));
                break;
            }
            default:
                RUNTIME_ERROR("Unknown opcode: %d", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
        if (result != INTERPRET_OK) {
            if (result == INTERPRET_RUNTIME_ERROR && vm.tryFrameCount > 0) {
                TryFrame frame = vm.tryFrames[--vm.tryFrameCount];
                vm.stackTop = vm.stack + frame.stackDepth;
                vm.globals[frame.varIndex] = vm.lastError;
                vm.ip = frame.handler;
                result = INTERPRET_OK;
                continue;
            }
            return result;
        }
    }

    #undef READ_BYTE
    #undef READ_SHORT
    #undef READ_CONSTANT

    return result;
}

/**
 * Execute the given chunk starting at instruction 0.
 *
 * @param chunk Chunk to run.
 * @return      Interpretation result code.
 */
InterpretResult runChunk(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = chunk->code;
    vm.instruction_count = 0;

    if (vm.trace) {
#ifdef DEBUG_TRACE_EXECUTION
        disassembleChunk(chunk, "chunk to execute");
#endif
    }

    InterpretResult result = run();
    vm.chunk = NULL;
    return result;
}

/**
 * Push a value onto the VM stack.
 *
 * @param value Value to push.
 */
void push(Value value) {
    vmPush(&vm, value);
}

/**
 * Pop and return the top value from the VM stack.
 */
Value pop() {
    return vmPop(&vm);
}

/**
 * Compile and execute a source string.
 *
 * @param source Source code to run.
 * @return       Interpretation result code.
 */
InterpretResult interpret(const char* source) {
    ASTNode* ast;
    if (!parse(source, "<exec>", &ast)) {
        return INTERPRET_COMPILE_ERROR;
    }

    Chunk chunk;
    initChunk(&chunk);
    Compiler compiler;
    initCompiler(&compiler, &chunk, "<exec>", source);

    vm.astRoot = ast;

    bool success = compile(ast, &compiler, false);
    vm.astRoot = NULL;

    if (!success) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    InterpretResult result = runChunk(&chunk);
    freeChunk(&chunk);
    return result;
}
