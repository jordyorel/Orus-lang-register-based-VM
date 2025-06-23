/* DEBUG_TRACE_EXECUTION enables verbose register VM tracing.
 * Keep undefined for normal execution to avoid polluting program output. */
/* #define DEBUG_TRACE_EXECUTION */
#include "../../include/reg_vm.h"
#include "../../include/vm.h"
#include "../../include/memory.h"
#include "../../include/vm_ops.h"
#include "../../include/value.h"
#include "../../include/modules.h"
#include "../../include/builtins.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Enable to print array index details during execution */
/* #define DEBUG_ARRAY_INDEX */

typedef struct {
    Value registers[REGISTER_COUNT];
    int64_t i64_cache[REGISTER_COUNT];
    double f64_cache[REGISTER_COUNT];
    bool i64_valid[REGISTER_COUNT];
    bool f64_valid[REGISTER_COUNT];
} RegisterState;

typedef struct {
    Value lastError;
    TryFrame* tryFrames;
    int tryFrameCount;
    int maxTryFrames;
} VMErrorState;

typedef struct {
    RegisterChunk* chunk;
    RegisterInstr* instructionPointer;
    RegisterState registerState;
    VMErrorState errorState;
    unsigned long instructionCount;
    bool garbageCollectionPaused;
} RegisterVM2;

typedef enum {
    VM_OK,
    VM_RUNTIME_ERROR,
    VM_STACK_OVERFLOW,
    VM_OUT_OF_MEMORY
} VMResult;

typedef VMResult (*InstructionHandler)(RegisterVM2* machine, RegisterInstr* instruction);

static bool appendStringDynamic(const char* src, char** buffer,
                                int* length, int* capacity) {
    int srcLen = (int)strlen(src);
    if (*length + srcLen >= *capacity) {
        *capacity = (*length + srcLen) * 2;
        char* newBuf = (char*)realloc(*buffer, *capacity);
        if (!newBuf) {
            vmRuntimeError("Memory reallocation failed during string append.");
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
                if (!appendValueString(arr->elements[i], buffer, length, capacity))
                    return false;
                if (i < arr->length - 1) {
                    if (!appendStringDynamic(", ", buffer, length, capacity))
                        return false;
                }
            }
            if (!appendStringDynamic("]", buffer, length, capacity)) return false;
            return true;
        }
        case VAL_ERROR:
            snprintf(tmp, sizeof(tmp), "Error(%d): %s", AS_ERROR(value)->type,
                     AS_ERROR(value)->message->chars);
            return appendStringDynamic(tmp, buffer, length, capacity);
        default:
            return appendStringDynamic("unknown", buffer, length, capacity);
    }
}

#define SYNC_I64_REG(r) do { regs[(r)] = I64_VAL(i64_regs[(r)]); } while (0)
#define SYNC_F64_REG(r) do { regs[(r)] = F64_VAL(f64_regs[(r)]); } while (0)

extern VM vm;

static bool pushRegisterFrame(RegisterVM* rvm, RegisterInstr* ret,
                              RegisterChunk* prev, uint8_t destReg) {
    if (vm.regFrameCount >= FRAMES_MAX) {
        vmRuntimeError("Register stack overflow.");
        return false;
    }
    RegisterFrame* frame = &vm.regFrames[vm.regFrameCount++];
    frame->returnAddress = ret;
    frame->previousChunk = prev;
    frame->retReg = destReg;
    memcpy(&frame->vm, rvm, sizeof(RegisterVM));
    return true;
}

static bool popRegisterFrame(RegisterVM* rvm, uint8_t* destReg) {
    if (vm.regFrameCount == 0) {
        vmRuntimeError("Register stack underflow.");
        return false;
    }
    RegisterFrame* frame = &vm.regFrames[--vm.regFrameCount];
    if (destReg) *destReg = frame->retReg;
    memcpy(rvm, &frame->vm, sizeof(RegisterVM));
    rvm->ip = frame->returnAddress;
    rvm->chunk = frame->previousChunk;
    return true;
}

void initRegisterVM(RegisterVM* rvm, RegisterChunk* chunk) {
    rvm->chunk = chunk;
    rvm->ip = chunk->code;
    memset(rvm->i64_regs, 0, sizeof(rvm->i64_regs));
    memset(rvm->f64_regs, 0, sizeof(rvm->f64_regs));
    memset(rvm->registers, 0, sizeof(rvm->registers));
}

void freeRegisterVM(RegisterVM* rvm) {
    (void)rvm;
}

Value runRegisterVM(RegisterVM* rvm) {
#if defined(__GNUC__) || defined(__clang__)
    RegisterInstr* ip = rvm->ip;
    Value* regs = rvm->registers;
    int64_t* i64_regs = rvm->i64_regs;
    double*  f64_regs = rvm->f64_regs;
    static long long loop_guard = 0;
    static void* dispatch[] = {
        &&op_NOP,
        &&op_MOV,
        &&op_LOAD_CONST,
        &&op_ADD_RR,
        &&op_SUB_RR,
        &&op_MUL_RR,
        &&op_DIV_RR,
        &&op_EQ_I64,
        &&op_NE_I64,
        &&op_LT_I64,
        &&op_LE_I64,
        &&op_GT_I64,
        &&op_GE_I64,
        &&op_JUMP,
        &&op_JZ,
        &&op_CALL,
        &&op_ADD_I32,
        &&op_SUB_I32,
        &&op_MUL_I32,
        &&op_DIV_I32,
        &&op_ADD_U32,
        &&op_SUB_U32,
        &&op_MUL_U32,
        &&op_DIV_U32,
        &&op_ADD_U64,
        &&op_SUB_U64,
        &&op_MUL_U64,
        &&op_DIV_U64,
        &&op_NEG_I32,
        &&op_NEG_U32,
        &&op_NEG_U64,
        &&op_AND,
        &&op_OR,
        &&op_NOT,
        &&op_BIT_AND_I32,
        &&op_BIT_AND_U32,
        &&op_BIT_OR_I32,
        &&op_BIT_OR_U32,
        &&op_BIT_XOR_I32,
        &&op_BIT_XOR_U32,
        &&op_BIT_NOT_I32,
        &&op_BIT_NOT_U32,
        &&op_SHIFT_LEFT_I32,
        &&op_SHIFT_RIGHT_I32,
        &&op_SHIFT_LEFT_U32,
        &&op_SHIFT_RIGHT_U32,
        &&op_I32_TO_BOOL,
        &&op_U32_TO_BOOL,
        &&op_BOOL_TO_I32,
        &&op_BOOL_TO_U32,
        &&op_BOOL_TO_F64,
        &&op_F64_TO_BOOL,
        &&op_I32_TO_F64,
        &&op_U32_TO_F64,
        &&op_I32_TO_U32,
        &&op_U32_TO_I32,
        &&op_I32_TO_I64,
        &&op_U32_TO_I64,
        &&op_I64_TO_I32,
        &&op_I64_TO_U32,
        &&op_I32_TO_U64,
        &&op_U32_TO_U64,
        &&op_U64_TO_I32,
        &&op_U64_TO_U32,
        &&op_U64_TO_F64,
        &&op_F64_TO_U64,
        &&op_F64_TO_I32,
        &&op_F64_TO_U32,
        &&op_I64_TO_F64,
        &&op_F64_TO_I64,
        &&op_I32_TO_STRING,
        &&op_U32_TO_STRING,
        &&op_F64_TO_STRING,
        &&op_BOOL_TO_STRING,
        &&op_ARRAY_TO_STRING,
        &&op_PRINT,
        &&op_PRINT_NO_NL,
        &&op_LOAD_GLOBAL,
        &&op_STORE_GLOBAL,
        &&op_ADD_F64,
        &&op_SUB_F64,
        &&op_MUL_F64,
        &&op_DIV_F64,
        &&op_MOD_I64,
        &&op_BIT_AND_I64,
        &&op_BIT_OR_I64,
        &&op_BIT_XOR_I64,
        &&op_BIT_NOT_I64,
        &&op_SHL_I64,
        &&op_SHR_I64,
        &&op_MAKE_ARRAY,
        &&op_ARRAY_GET,
        &&op_ARRAY_SET,
        &&op_ARRAY_PUSH,
        &&op_ARRAY_POP,
        &&op_LEN,
        &&op_I64_TO_STRING,
        &&op_ARRAY_RESERVE,
        &&op_CONCAT,
        &&op_TYPE_OF_I32,
        &&op_TYPE_OF_I64,
        &&op_TYPE_OF_U32,
        &&op_TYPE_OF_U64,
        &&op_TYPE_OF_F64,
        &&op_TYPE_OF_BOOL,
        &&op_TYPE_OF_STRING,
        &&op_TYPE_OF_ARRAY,
        &&op_GC_PAUSE,
        &&op_GC_RESUME,
        &&op_ADD_I64,
        &&op_ADD_NUMERIC,
        &&op_BOOL_TO_I64,
        &&op_BOOL_TO_U64,
        &&op_BREAK,
        &&op_CALL_NATIVE,
        &&op_CONSTANT,
        &&op_CONSTANT_LONG,
        &&op_CONTINUE,
        &&op_DEFINE_GLOBAL,
        &&op_DIVIDE_F64,
        &&op_DIVIDE_I32,
        &&op_DIVIDE_I64,
        &&op_DIVIDE_NUMERIC,
        &&op_DIVIDE_U32,
        &&op_DIVIDE_U64,
        &&op_EQUAL,
        &&op_EQUAL_I64,
        &&op_FORMAT_PRINT,
        &&op_FORMAT_PRINT_NO_NL,
        &&op_GET_GLOBAL,
        &&op_GREATER_EQUAL_F64,
        &&op_GREATER_EQUAL_I32,
        &&op_GREATER_EQUAL_I64,
        &&op_GREATER_EQUAL_U32,
        &&op_GREATER_EQUAL_U64,
        &&op_GREATER_F64,
        &&op_GREATER_I32,
        &&op_GREATER_I64,
        &&op_GREATER_U32,
        &&op_GREATER_U64,
        &&op_I64_CONST,
        &&op_I64_TO_BOOL,
        &&op_I64_TO_U64,
        &&op_IMPORT,
        &&op_INC_I64,
        &&op_ITER_NEXT_I64,
        &&op_JUMP_IF_FALSE,
        &&op_JUMP_IF_LT_I64,
        &&op_JUMP_IF_TRUE,
        &&op_LEN_ARRAY,
        &&op_LEN_STRING,
        &&op_LESS_EQUAL_F64,
        &&op_LESS_EQUAL_I32,
        &&op_LESS_EQUAL_I64,
        &&op_LESS_EQUAL_U32,
        &&op_LESS_EQUAL_U64,
        &&op_LESS_F64,
        &&op_LESS_I32,
        &&op_LESS_I64,
        &&op_LESS_U32,
        &&op_LESS_U64,
        &&op_LOOP,
        &&op_MODULO_I32,
        &&op_MODULO_I64,
        &&op_MODULO_NUMERIC,
        &&op_MODULO_U32,
        &&op_MODULO_U64,
        &&op_MULTIPLY_F64,
        &&op_MULTIPLY_I32,
        &&op_MULTIPLY_I64,
        &&op_MULTIPLY_NUMERIC,
        &&op_MULTIPLY_U32,
        &&op_MULTIPLY_U64,
        &&op_NEGATE_F64,
        &&op_NEGATE_I32,
        &&op_NEGATE_I64,
        &&op_NEGATE_NUMERIC,
        &&op_NEGATE_U32,
        &&op_NEGATE_U64,
        &&op_NIL,
        &&op_NOT_EQUAL,
        &&op_NOT_EQUAL_I64,
        &&op_POP,
        &&op_POP_EXCEPT,
        &&op_PRINT_BOOL,
        &&op_PRINT_BOOL_NO_NL,
        &&op_PRINT_F64,
        &&op_PRINT_F64_NO_NL,
        &&op_PRINT_I32,
        &&op_PRINT_I32_NO_NL,
        &&op_PRINT_I64,
        &&op_PRINT_I64_NO_NL,
        &&op_PRINT_STRING,
        &&op_PRINT_STRING_NO_NL,
        &&op_PRINT_U32,
        &&op_PRINT_U32_NO_NL,
        &&op_PRINT_U64,
        &&op_PRINT_U64_NO_NL,
        &&op_RETURN,
        &&op_SETUP_EXCEPT,
        &&op_SET_GLOBAL,
        &&op_SHIFT_LEFT_I64,
        &&op_SHIFT_RIGHT_I64,
        &&op_SLICE,
        &&op_SUBSTRING,
        &&op_SUBTRACT_F64,
        &&op_SUBTRACT_I32,
        &&op_SUBTRACT_I64,
        &&op_SUBTRACT_NUMERIC,
        &&op_SUBTRACT_U32,
        &&op_SUBTRACT_U64,
        &&op_U64_TO_BOOL,
        &&op_U64_TO_I64,
        &&op_U64_TO_STRING,
        &&op_EQ_F64,
        &&op_NE_F64,
        &&op_RANGE,
        &&op_SUM,
        &&op_MIN,
        &&op_MAX,
        &&op_IS_TYPE,
        &&op_INPUT,
        &&op_INT,
        &&op_FLOAT,
        &&op_TIMESTAMP,
        &&op_SORTED,
        &&op_MODULE_NAME,
        &&op_MODULE_PATH,
        &&op_NATIVE_POW,
        &&op_NATIVE_SQRT,
        &&op_CALL_BUILTIN_SLICE,
        &&op_SPILL_REG,
        &&op_UNSPILL_REG,
        /* Modern Array Operations - Phase 5.1.2 */
        &&op_ARRAY_NEW,
        &&op_ARRAY_LEN,
        &&op_ARRAY_INSERT,
        &&op_ARRAY_REMOVE,
        &&op_ARRAY_SLICE,
        &&op_ARRAY_CONCAT,
        &&op_ARRAY_REVERSE,
        &&op_ARRAY_SORT,
    };
#define DISPATCH()                                                         \
    do {                                                                  \
        if (++loop_guard > 10000000000LL) {                               \
            fprintf(stderr, "[VM] Infinite loop guard triggered.\n");    \
            exit(1);                                                      \
        }                                                                 \
        if (IS_ERROR(vm.lastError)) {                                     \
            if (vm.tryFrameCount > 0) {                                   \
                TryFrame frame = vm.tryFrames[--vm.tryFrameCount];         \
                vm.stackTop = vm.stack + frame.stackDepth;                 \
                vm.globals[frame.varIndex] = vm.lastError;                 \
                ip = (RegisterInstr*)frame.handler;                        \
                vm.lastError = NIL_VAL;                                    \
            } else {                                                       \
                rvm->ip = ip;                                              \
                return NIL_VAL;                                            \
            }                                                             \
        }                                                                 \
        vm.instruction_count++;                                           \
        if (vm.instruction_count % 10000 == 0 && !vm.gcPaused)             \
            collectGarbage();                                             \
        goto *dispatch[ip->opcode];                                       \
    } while (0)

    DISPATCH();

op_NOP:
    ip++;
    DISPATCH();

op_MOV:
    regs[ip->dst] = regs[ip->src1];
    if (IS_I64(regs[ip->dst])) i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++;
    DISPATCH();

op_LOAD_CONST:
    regs[ip->dst] = rvm->chunk->constants.values[ip->src1];
    if (IS_I64(regs[ip->dst])) i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++;
    DISPATCH();

op_ADD_RR: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    regs[ip->dst] = I64_VAL(AS_I64(a) + AS_I64(b));
    ip++;
    DISPATCH();
}

op_SUB_RR: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    regs[ip->dst] = I64_VAL(AS_I64(a) - AS_I64(b));
    ip++;
    DISPATCH();
}

op_MUL_RR: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    regs[ip->dst] = I64_VAL(AS_I64(a) * AS_I64(b));
    ip++;
    DISPATCH();
}

op_DIV_RR: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (AS_I64(b) == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = I64_VAL(AS_I64(a) / AS_I64(b));
    ip++;
    DISPATCH();
}

op_EQ_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(i64_regs[s1] == i64_regs[s2]);
    ip++; DISPATCH();
}

op_NE_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(i64_regs[s1] != i64_regs[s2]);
    ip++; DISPATCH();
}

op_LT_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(i64_regs[s1] < i64_regs[s2]);
    ip++; DISPATCH();
}

op_LE_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(i64_regs[s1] <= i64_regs[s2]);
    ip++; DISPATCH();
}

op_GT_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(i64_regs[s1] > i64_regs[s2]);
    ip++; DISPATCH();
}

op_GE_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(i64_regs[s1] >= i64_regs[s2]);
    ip++; DISPATCH();
}

op_PRINT:
    printValue(regs[ip->src1]);
    putchar('\n');
    ip++;
    DISPATCH();

op_PRINT_NO_NL:
    printValue(regs[ip->src1]);
    fflush(stdout);
    ip++;
    DISPATCH();

op_LOAD_GLOBAL:
    regs[ip->dst] = vm.globals[ip->src1];
    if (IS_I64(regs[ip->dst])) i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++;
    DISPATCH();

op_STORE_GLOBAL:
    vm.globals[ip->dst] = regs[ip->src1];
    ip++;
    DISPATCH();

op_ADD_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    f64_regs[dest] = f64_regs[s1] + f64_regs[s2];
    SYNC_F64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] f64_regs[R%d] = %f\n", dest, f64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_SUB_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    f64_regs[dest] = f64_regs[s1] - f64_regs[s2];
    SYNC_F64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] f64_regs[R%d] = %f\n", dest, f64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_MUL_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    f64_regs[dest] = f64_regs[s1] * f64_regs[s2];
    SYNC_F64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] f64_regs[R%d] = %f\n", dest, f64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_DIV_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    double b = f64_regs[s2];
    if (b == 0.0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    f64_regs[dest] = f64_regs[s1] / b;
    SYNC_F64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] f64_regs[R%d] = %f\n", dest, f64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_EQ_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(f64_regs[s1] == f64_regs[s2]);
    ip++; DISPATCH();
}

op_NE_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(f64_regs[s1] != f64_regs[s2]);
    ip++; DISPATCH();
}

op_RANGE:
    regs[ip->dst] = builtin_range(regs[ip->src1], regs[ip->src2]);
    ip++; DISPATCH();

op_SUM:
    regs[ip->dst] = builtin_sum(regs[ip->src1]);
    ip++; DISPATCH();

op_MIN:
    regs[ip->dst] = builtin_min(regs[ip->src1]);
    ip++; DISPATCH();

op_MAX:
    regs[ip->dst] = builtin_max(regs[ip->src1]);
    ip++; DISPATCH();

op_IS_TYPE:
    regs[ip->dst] = builtin_is_type(regs[ip->src1], regs[ip->src2]);
    ip++; DISPATCH();

op_INPUT:
    regs[ip->dst] = builtin_input(regs[ip->src1]);
    ip++; DISPATCH();

op_INT:
    regs[ip->dst] = builtin_int(regs[ip->src1]);
    ip++; DISPATCH();

op_FLOAT:
    regs[ip->dst] = builtin_float(regs[ip->src1]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++; DISPATCH();

op_TIMESTAMP:
    regs[ip->dst] = builtin_timestamp();
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++; DISPATCH();

op_SORTED:
    regs[ip->dst] = builtin_sorted(regs[ip->src1], NIL_VAL, regs[ip->src2]);
    ip++; DISPATCH();

op_MODULE_NAME:
    regs[ip->dst] = builtin_module_name(regs[ip->src1]);
    ip++; DISPATCH();

op_MODULE_PATH:
    regs[ip->dst] = builtin_module_path(regs[ip->src1]);
    ip++; DISPATCH();

op_NATIVE_POW:
    regs[ip->dst] = builtin_native_pow(regs[ip->src1], regs[ip->src2]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++; DISPATCH();

op_NATIVE_SQRT:
    regs[ip->dst] = builtin_native_sqrt(regs[ip->src1]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++; DISPATCH();

op_DIVIDE_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    int64_t b = i64_regs[s2];
    if (b == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    i64_regs[dest] = i64_regs[s1] / b;
    SYNC_I64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", dest, (long long)i64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_GREATER_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a > b);
    ip++; DISPATCH();
}

op_LESS_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a < b);
    ip++; DISPATCH();
}

op_MOD_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    int64_t b = i64_regs[s2];
    if (b == 0) {
        i64_regs[dest] = 0;
    } else {
        int64_t r = i64_regs[s1] % b;
        if (r < 0) r += (b < 0) ? -b : b;
        i64_regs[dest] = r;
    }
    SYNC_I64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", dest, (long long)i64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_BIT_AND_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = I64_VAL(a & b);
    ip++; DISPATCH();
}

op_BIT_OR_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = I64_VAL(a | b);
    ip++; DISPATCH();
}

op_BIT_XOR_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = I64_VAL(a ^ b);
    ip++; DISPATCH();
}

op_BIT_NOT_I64:
    regs[ip->dst] = I64_VAL(~AS_I64(regs[ip->src1]));
    ip++; DISPATCH();

op_SHL_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = I64_VAL(a << b);
    ip++; DISPATCH();
}

op_SHR_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = I64_VAL(a >> b);
    ip++; DISPATCH();
}

op_MAKE_ARRAY: {
    int count = ip->src1;
    ObjArray* arr = allocateArray(count);
    regs[ip->dst] = ARRAY_VAL(arr);
    ip++; DISPATCH();
}

op_ARRAY_GET: {
    Value arrayVal = regs[ip->src1];
    Value indexVal = regs[ip->src2];
    int64_t index = 0;
    if (IS_I64(indexVal))       index = AS_I64(indexVal);
    else if (IS_I32(indexVal))  index = AS_I32(indexVal);
    else if (IS_U32(indexVal))  index = (int64_t)AS_U32(indexVal);
    else if (IS_U64(indexVal))  index = (int64_t)AS_U64(indexVal);
    else {
        vmRuntimeError("Expected integer index.");
        goto HANDLE_RUNTIME_ERROR;
    }

    if (!IS_ARRAY(arrayVal)) {
        vmRuntimeError("Expected array for indexing.");
        goto HANDLE_RUNTIME_ERROR;
    }

    ObjArray* arr = AS_ARRAY(arrayVal);
#ifdef DEBUG_ARRAY_INDEX
    printf("[DEBUG] GET index: i64[%d] = %ld, array length = %d\n",
           ip->src2, index, arr->length);
#endif
    if (index < 0 || index >= arr->length) {
        vmRuntimeError("Array index out of bounds.");
        goto HANDLE_RUNTIME_ERROR;
    }

    regs[ip->dst] = arr->elements[index];
    ip++; DISPATCH();
}

op_ARRAY_SET: {
    Value arrayVal = regs[ip->dst];
    Value indexVal = regs[ip->src1];
    int64_t index = 0;
    if (IS_I64(indexVal))       index = AS_I64(indexVal);
    else if (IS_I32(indexVal))  index = AS_I32(indexVal);
    else if (IS_U32(indexVal))  index = (int64_t)AS_U32(indexVal);
    else if (IS_U64(indexVal))  index = (int64_t)AS_U64(indexVal);
    else {
        vmRuntimeError("Expected integer index.");
        goto HANDLE_RUNTIME_ERROR;
    }
    Value value = regs[ip->src2];

    if (!IS_ARRAY(arrayVal)) {
        vmRuntimeError("Expected array for assignment.");
        goto HANDLE_RUNTIME_ERROR;
    }

    ObjArray* arr = AS_ARRAY(arrayVal);
#ifdef DEBUG_ARRAY_INDEX
    printf("[DEBUG] SET index: i64[%d] = %ld, array length = %d\n",
           ip->src1, index, arr->length);
#endif
    if (index < 0 || index >= arr->length) {
        vmRuntimeError("Array index out of bounds.");
        goto HANDLE_RUNTIME_ERROR;
    }

    arr->elements[index] = value;
    ip++; DISPATCH();
}

op_ARRAY_PUSH:
    if (IS_ARRAY(regs[ip->dst])) {
        arrayPush(&vm, AS_ARRAY(regs[ip->dst]), regs[ip->src2]);
    }
    ip++; DISPATCH();

op_ARRAY_POP:
    if (IS_ARRAY(regs[ip->dst])) {
        regs[ip->dst] = arrayPop(AS_ARRAY(regs[ip->dst]));
    } else {
        regs[ip->dst] = NIL_VAL;
    }
    ip++; DISPATCH();

op_LEN:
    if (IS_ARRAY(regs[ip->src1]))
        regs[ip->dst] = I32_VAL(AS_ARRAY(regs[ip->src1])->length);
    else if (IS_STRING(regs[ip->src1]))
        regs[ip->dst] = I32_VAL(AS_STRING(regs[ip->src1])->length);
    else regs[ip->dst] = I32_VAL(0);
    ip++; DISPATCH();

op_I64_TO_STRING:
    regs[ip->dst] = convertToString(regs[ip->src1]);
    ip++; DISPATCH();

op_JUMP:
    ip = rvm->chunk->code + ip->dst;
    DISPATCH();

op_JZ:
    if ((IS_BOOL(regs[ip->src1]) && !AS_BOOL(regs[ip->src1])) ||
        (IS_I64(regs[ip->src1]) && AS_I64(regs[ip->src1]) == 0)) {
        ip = rvm->chunk->code + ip->dst;
    } else {
        ip++;
    }
    DISPATCH();

op_CALL:
    {
        uint8_t base = ip->dst;
        uint8_t globalIndex = ip->src1;
        uint8_t argc = ip->src2;
        if ((int)globalIndex >= UINT8_COUNT || !IS_I32(vm.globals[globalIndex])) {
            vmRuntimeError("Attempt to call a non-function.");
            return NIL_VAL;
        }
        int funcIndex = AS_I32(vm.globals[globalIndex]);
        if (funcIndex < 0 || funcIndex >= vm.regChunk.functionCount) {
            vmRuntimeError("Invalid function index.");
            return NIL_VAL;
        }
        int target = vm.regChunk.functionOffsets[funcIndex];
        if (target < 0) {
            vmRuntimeError("Missing register offset for function.");
            return NIL_VAL;
        }
        if (!pushRegisterFrame(rvm, ip + 1, rvm->chunk, base)) {
            return NIL_VAL;
        }
        rvm->chunk = &vm.regChunk;
        rvm->ip = rvm->chunk->code + target;
        for (int i = 0; i < argc && (i < REGISTER_COUNT); i++) {
            rvm->registers[i] = regs[base + i];
            rvm->i64_regs[i] = i64_regs[base + i];
            rvm->f64_regs[i] = f64_regs[base + i];
        }
        for (int r = argc; r < REGISTER_COUNT; r++) {
            rvm->registers[r] = NIL_VAL;
            rvm->i64_regs[r] = 0;
            rvm->f64_regs[r] = 0.0;
        }
        ip = rvm->ip;
        regs = rvm->registers;
        i64_regs = rvm->i64_regs;
        f64_regs = rvm->f64_regs;
    }
    DISPATCH();

op_ADD_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a + b);
    ip++;
    DISPATCH();
}

op_SUB_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a - b);
    ip++;
    DISPATCH();
}

op_MUL_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a * b);
    ip++;
    DISPATCH();
}

op_DIV_I32: {
    int32_t b = AS_I32(regs[ip->src2]);
    int32_t a = AS_I32(regs[ip->src1]);
    if (b == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = I32_VAL(a / b);
    ip++;
    DISPATCH();
}

op_ADD_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a + b);
    ip++;
    DISPATCH();
}

op_SUB_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a - b);
    ip++;
    DISPATCH();
}

op_MUL_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a * b);
    ip++;
    DISPATCH();
}

op_DIV_U32: {
    uint32_t b = AS_U32(regs[ip->src2]);
    uint32_t a = AS_U32(regs[ip->src1]);
    if (b == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = U32_VAL(a / b);
    ip++;
    DISPATCH();
}

op_ADD_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = U64_VAL(a + b);
    ip++;
    DISPATCH();
}

op_SUB_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = U64_VAL(a - b);
    ip++;
    DISPATCH();
}

op_MUL_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = U64_VAL(a * b);
    ip++;
    DISPATCH();
}

op_DIV_U64: {
    uint64_t b = AS_U64(regs[ip->src2]);
    uint64_t a = AS_U64(regs[ip->src1]);
    if (b == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = U64_VAL(a / b);
    ip++;
    DISPATCH();
}

op_NEG_I32:
    regs[ip->dst] = I32_VAL(-AS_I32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_NEG_U32:
    regs[ip->dst] = U32_VAL(-AS_U32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_NEG_U64:
    regs[ip->dst] = U64_VAL(-AS_U64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_AND: {
    bool a = AS_BOOL(regs[ip->src1]);
    bool b = AS_BOOL(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a && b);
    ip++;
    DISPATCH();
}

op_OR: {
    bool a = AS_BOOL(regs[ip->src1]);
    bool b = AS_BOOL(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a || b);
    ip++;
    DISPATCH();
}

op_NOT:
    regs[ip->dst] = BOOL_VAL(!AS_BOOL(regs[ip->src1]));
    ip++;
    DISPATCH();

op_BIT_AND_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a & b);
    ip++;
    DISPATCH();
}

op_BIT_AND_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a & b);
    ip++;
    DISPATCH();
}

op_BIT_OR_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a | b);
    ip++;
    DISPATCH();
}

op_BIT_OR_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a | b);
    ip++;
    DISPATCH();
}

op_BIT_XOR_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a ^ b);
    ip++;
    DISPATCH();
}

op_BIT_XOR_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a ^ b);
    ip++;
    DISPATCH();
}

op_BIT_NOT_I32:
    regs[ip->dst] = I32_VAL(~AS_I32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_BIT_NOT_U32:
    regs[ip->dst] = U32_VAL(~AS_U32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_SHIFT_LEFT_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a << b);
    ip++;
    DISPATCH();
}

op_SHIFT_RIGHT_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a >> b);
    ip++;
    DISPATCH();
}

op_SHIFT_LEFT_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a << b);
    ip++;
    DISPATCH();
}

op_SHIFT_RIGHT_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a >> b);
    ip++;
    DISPATCH();
}

op_I32_TO_BOOL:
    regs[ip->dst] = BOOL_VAL(AS_I32(regs[ip->src1]) != 0);
    ip++;
    DISPATCH();

op_U32_TO_BOOL:
    regs[ip->dst] = BOOL_VAL(AS_U32(regs[ip->src1]) != 0);
    ip++;
    DISPATCH();

op_BOOL_TO_I32:
    regs[ip->dst] = I32_VAL(AS_BOOL(regs[ip->src1]) ? 1 : 0);
    ip++;
    DISPATCH();

op_BOOL_TO_U32:
    regs[ip->dst] = U32_VAL(AS_BOOL(regs[ip->src1]) ? 1u : 0u);
    ip++;
    DISPATCH();

op_BOOL_TO_F64:
    regs[ip->dst] = F64_VAL(AS_BOOL(regs[ip->src1]) ? 1.0 : 0.0);
    ip++;
    DISPATCH();

op_F64_TO_BOOL:
    regs[ip->dst] = BOOL_VAL(AS_F64(regs[ip->src1]) != 0.0);
    ip++;
    DISPATCH();

op_I32_TO_F64:
    regs[ip->dst] = F64_VAL((double)AS_I32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_U32_TO_F64:
    regs[ip->dst] = F64_VAL((double)AS_U32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_I32_TO_U32:
    regs[ip->dst] = U32_VAL((uint32_t)AS_I32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_U32_TO_I32:
    regs[ip->dst] = I32_VAL((int32_t)AS_U32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_I32_TO_I64:
    regs[ip->dst] = I64_VAL((int64_t)AS_I32(regs[ip->src1]));
    i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    ip++;
    DISPATCH();

op_U32_TO_I64:
    regs[ip->dst] = I64_VAL((int64_t)AS_U32(regs[ip->src1]));
    i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    ip++;
    DISPATCH();

op_I64_TO_I32:
    regs[ip->dst] = I32_VAL((int32_t)AS_I64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_I64_TO_U32:
    regs[ip->dst] = U32_VAL((uint32_t)AS_I64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_I32_TO_U64:
    regs[ip->dst] = U64_VAL((uint64_t)AS_I32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_U32_TO_U64:
    regs[ip->dst] = U64_VAL((uint64_t)AS_U32(regs[ip->src1]));
    ip++;
    DISPATCH();

op_U64_TO_I32:
    regs[ip->dst] = I32_VAL((int32_t)AS_U64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_U64_TO_U32:
    regs[ip->dst] = U32_VAL((uint32_t)AS_U64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_U64_TO_F64:
    regs[ip->dst] = F64_VAL((double)AS_U64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_F64_TO_U64:
    regs[ip->dst] = U64_VAL((uint64_t)AS_F64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_F64_TO_I32:
    regs[ip->dst] = I32_VAL((int32_t)AS_F64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_F64_TO_U32:
    regs[ip->dst] = U32_VAL((uint32_t)AS_F64(regs[ip->src1]));
    ip++;
    DISPATCH();

op_I64_TO_F64:
    regs[ip->dst] = F64_VAL((double)AS_I64(regs[ip->src1]));
    f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++;
    DISPATCH();

op_F64_TO_I64:
    regs[ip->dst] = I64_VAL((int64_t)AS_F64(regs[ip->src1]));
    i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    ip++;
    DISPATCH();

op_I32_TO_STRING:
    regs[ip->dst] = convertToString(regs[ip->src1]);
    ip++;
    DISPATCH();

op_U32_TO_STRING:
    regs[ip->dst] = convertToString(regs[ip->src1]);
    ip++;
    DISPATCH();

op_F64_TO_STRING:
    regs[ip->dst] = convertToString(regs[ip->src1]);
    ip++;
    DISPATCH();

op_BOOL_TO_STRING:
    regs[ip->dst] = convertToString(regs[ip->src1]);
    ip++;
    DISPATCH();

op_ARRAY_TO_STRING:
    regs[ip->dst] = convertToString(regs[ip->src1]);
    ip++;
    DISPATCH();

op_ARRAY_RESERVE: {
    ObjArray* arr = AS_ARRAY(regs[ip->dst]);
    int cap = (int)AS_I64(regs[ip->src2]);
    if (cap > arr->capacity) {
        int oldCap = arr->capacity;
        arr->capacity = cap;
        arr->elements = GROW_ARRAY(Value, arr->elements, oldCap, arr->capacity);
        vm.bytesAllocated += sizeof(Value) * (arr->capacity - oldCap);
    }
    ip++; DISPATCH();
}

op_CONCAT: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (!IS_STRING(a)) a = convertToString(a);
    if (!IS_STRING(b)) b = convertToString(b);
    int len = AS_STRING(a)->length + AS_STRING(b)->length;
    char* chars = (char*)malloc(len + 1);
    memcpy(chars, AS_STRING(a)->chars, AS_STRING(a)->length);
    memcpy(chars + AS_STRING(a)->length, AS_STRING(b)->chars, AS_STRING(b)->length);
    chars[len] = '\0';
    ObjString* result = allocateString(chars, len);
    free(chars);
    regs[ip->dst] = STRING_VAL(result);
    ip++; DISPATCH();
}

op_TYPE_OF_I32:
    regs[ip->dst] = STRING_VAL(allocateString("i32", 3));
    ip++; DISPATCH();

op_TYPE_OF_I64:
    regs[ip->dst] = STRING_VAL(allocateString("i64", 3));
    ip++; DISPATCH();

op_TYPE_OF_U32:
    regs[ip->dst] = STRING_VAL(allocateString("u32", 3));
    ip++; DISPATCH();

op_TYPE_OF_U64:
    regs[ip->dst] = STRING_VAL(allocateString("u64", 3));
    ip++; DISPATCH();

op_TYPE_OF_F64:
    regs[ip->dst] = STRING_VAL(allocateString("f64", 3));
    ip++; DISPATCH();

op_TYPE_OF_BOOL:
    regs[ip->dst] = STRING_VAL(allocateString("bool", 4));
    ip++; DISPATCH();

op_TYPE_OF_STRING:
    regs[ip->dst] = STRING_VAL(allocateString("string", 6));
    ip++; DISPATCH();

op_TYPE_OF_ARRAY:
    regs[ip->dst] = STRING_VAL(allocateString("array", 5));
    ip++; DISPATCH();

op_GC_PAUSE:
    pauseGC();
    ip++; DISPATCH();

op_GC_RESUME:
    resumeGC();
    ip++; DISPATCH();


op_ADD_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    i64_regs[dest] = i64_regs[s1] + i64_regs[s2];
    SYNC_I64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", dest, (long long)i64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_MULTIPLY_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    i64_regs[dest] = i64_regs[s1] * i64_regs[s2];
    SYNC_I64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", dest, (long long)i64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_ADD_NUMERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32: regs[ip->dst] = I32_VAL(AS_I32(a) + AS_I32(b)); break;
            case VAL_I64: regs[ip->dst] = I64_VAL(AS_I64(a) + AS_I64(b)); break;
            case VAL_U32: regs[ip->dst] = U32_VAL(AS_U32(a) + AS_U32(b)); break;
            case VAL_U64: regs[ip->dst] = U64_VAL(AS_U64(a) + AS_U64(b)); break;
            case VAL_F64: regs[ip->dst] = F64_VAL(AS_F64(a) + AS_F64(b)); break;
            default: regs[ip->dst] = NIL_VAL; break;
        }
    }
    ip++; DISPATCH();
}





op_MODULO_I32:
    {
        int32_t b = AS_I32(regs[ip->src2]);
        int32_t a = AS_I32(regs[ip->src1]);
        regs[ip->dst] = b == 0 ? NIL_VAL : I32_VAL(a % b);
    }
    ip++; DISPATCH();

op_MODULO_I64:
    {
        int64_t b = AS_I64(regs[ip->src2]);
        int64_t a = AS_I64(regs[ip->src1]);
        regs[ip->dst] = b == 0 ? NIL_VAL : I64_VAL(a % b);
    }
    ip++; DISPATCH();

op_MODULO_U32:
    {
        uint32_t b = AS_U32(regs[ip->src2]);
        uint32_t a = AS_U32(regs[ip->src1]);
        regs[ip->dst] = b == 0 ? NIL_VAL : U32_VAL(a % b);
    }
    ip++; DISPATCH();

op_MODULO_U64:
    {
        uint64_t b = AS_U64(regs[ip->src2]);
        uint64_t a = AS_U64(regs[ip->src1]);
        regs[ip->dst] = b == 0 ? NIL_VAL : U64_VAL(a % b);
    }
    ip++; DISPATCH();


op_NEGATE_I64:
    i64_regs[ip->dst] = -i64_regs[ip->src1];
    SYNC_I64_REG(ip->dst);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", ip->dst, (long long)i64_regs[ip->dst]);
#endif
    ip++; DISPATCH();

op_BOOL_TO_I64:
    i64_regs[ip->dst] = regs[ip->src1].type == VAL_BOOL && AS_BOOL(regs[ip->src1]) ? 1 : 0;
    SYNC_I64_REG(ip->dst);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", ip->dst, (long long)i64_regs[ip->dst]);
#endif
    ip++; DISPATCH();

op_BOOL_TO_U64:
    i64_regs[ip->dst] = regs[ip->src1].type == VAL_BOOL && AS_BOOL(regs[ip->src1]) ? 1 : 0;
    SYNC_I64_REG(ip->dst);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", ip->dst, (long long)i64_regs[ip->dst]);
#endif
    ip++; DISPATCH();

op_BREAK:
    ip++; DISPATCH();

op_CALL_NATIVE: {
    NativeFunction* nf = &vm.nativeFunctions[ip->src1];
    uint8_t argc = ip->src2;
    Value* args = &regs[ip->dst];
    Value result = nf->function(argc, args);
    regs[ip->dst] = result;
    if (IS_I64(result)) i64_regs[ip->dst] = AS_I64(result);
    if (IS_F64(result)) f64_regs[ip->dst] = AS_F64(result);
    ip++; DISPATCH();
}

op_CONSTANT:
    regs[ip->dst] = rvm->chunk->constants.values[ip->src1];
    if (IS_I64(regs[ip->dst])) i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++; DISPATCH();

op_CONSTANT_LONG:
    regs[ip->dst] = rvm->chunk->constants.values[ip->src1];
    if (IS_I64(regs[ip->dst])) i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++; DISPATCH();

op_CONTINUE:
    ip++; DISPATCH();

op_EQUAL:
    regs[ip->dst] = BOOL_VAL(valuesEqual(regs[ip->src1], regs[ip->src2]));
    ip++; DISPATCH();

op_NOT_EQUAL:
    regs[ip->dst] = BOOL_VAL(!valuesEqual(regs[ip->src1], regs[ip->src2]));
    ip++; DISPATCH();

op_NOT_EQUAL_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a != b);
    ip++; DISPATCH();
}

op_JUMP_IF_FALSE:
    if ((IS_BOOL(regs[ip->src1]) && !AS_BOOL(regs[ip->src1])) ||
        (IS_I64(regs[ip->src1]) && AS_I64(regs[ip->src1]) == 0)) {
        ip = rvm->chunk->code + ip->dst;
    } else {
        ip++;
    }
    DISPATCH();

op_JUMP_IF_TRUE:
    if ((IS_BOOL(regs[ip->src1]) && AS_BOOL(regs[ip->src1])) ||
        (IS_I64(regs[ip->src1]) && AS_I64(regs[ip->src1]) != 0)) {
        ip = rvm->chunk->code + ip->dst;
    } else {
        ip++;
    }
    DISPATCH();

op_LOOP:
    ip = rvm->chunk->code + ip->dst;
    DISPATCH();

op_POP:
    regs[ip->dst] = NIL_VAL;
    ip++; DISPATCH();

op_RETURN:
    {
        Value ret = regs[ip->src1];
        if (vm.regFrameCount == 0) {
            rvm->ip = ip + 1;
            return ret;
        }
        uint8_t dest;
        popRegisterFrame(rvm, &dest);
        regs = rvm->registers;
        i64_regs = rvm->i64_regs;
        f64_regs = rvm->f64_regs;
        ip = rvm->ip;
        if (!IS_NIL(ret)) {
            regs[dest] = ret;
            if (IS_I64(ret)) i64_regs[dest] = AS_I64(ret);
            if (IS_F64(ret)) f64_regs[dest] = AS_F64(ret);
        }
    }
    DISPATCH();

op_NIL:
    regs[ip->dst] = NIL_VAL;
    ip++; DISPATCH();

op_POP_EXCEPT:
    if (vm.tryFrameCount > 0) vm.tryFrameCount--;
    ip++; DISPATCH();

op_PRINT_BOOL:
    printf("%s\n", AS_BOOL(regs[ip->src1]) ? "true" : "false");
    ip++; DISPATCH();

op_PRINT_BOOL_NO_NL:
    printf("%s", AS_BOOL(regs[ip->src1]) ? "true" : "false");
    fflush(stdout);
    ip++; DISPATCH();

op_PRINT_F64:
    printf("%g\n", AS_F64(regs[ip->src1]));
    ip++; DISPATCH();

op_PRINT_F64_NO_NL:
    printf("%g", AS_F64(regs[ip->src1]));
    fflush(stdout);
    ip++; DISPATCH();

op_PRINT_I32:
    printf("%d\n", AS_I32(regs[ip->src1]));
    ip++; DISPATCH();

op_PRINT_I32_NO_NL:
    printf("%d", AS_I32(regs[ip->src1]));
    fflush(stdout);
    ip++; DISPATCH();

op_PRINT_I64:
    printf("%lld\n", (long long)AS_I64(regs[ip->src1]));
    ip++; DISPATCH();

op_PRINT_I64_NO_NL:
    printf("%lld", (long long)AS_I64(regs[ip->src1]));
    fflush(stdout);
    ip++; DISPATCH();

op_PRINT_STRING:
    printf("%s\n", AS_STRING(regs[ip->src1])->chars);
    ip++; DISPATCH();

op_PRINT_STRING_NO_NL:
    printf("%s", AS_STRING(regs[ip->src1])->chars);
    fflush(stdout);
    ip++; DISPATCH();

op_PRINT_U32:
    printf("%u\n", AS_U32(regs[ip->src1]));
    ip++; DISPATCH();

op_PRINT_U32_NO_NL:
    printf("%u", AS_U32(regs[ip->src1]));
    fflush(stdout);
    ip++; DISPATCH();

op_PRINT_U64:
    printf("%llu\n", (unsigned long long)AS_U64(regs[ip->src1]));
    ip++; DISPATCH();

op_PRINT_U64_NO_NL:
    printf("%llu", (unsigned long long)AS_U64(regs[ip->src1]));
    fflush(stdout);
    ip++; DISPATCH();

op_FORMAT_PRINT:
    {
        uint8_t base = ip->dst;
        uint8_t argc = ip->src1;
        #ifdef DEBUG_TRACE_EXECUTION
        printf("[DEBUG] FORMAT_PRINT: base=%d, argc=%d, regs[%d] type=%d\n", 
               base, argc, base, regs[base].type);
        #endif
        if (!IS_STRING(regs[base])) {
            vmRuntimeError("Format string must be a string.");
            goto HANDLE_RUNTIME_ERROR;
        }
        ObjString* formatStr = AS_STRING(regs[base]);
        int resultCapacity = formatStr->length * 2;
        char* resultBuffer = (char*)malloc(resultCapacity);
        if (!resultBuffer) {
            vmRuntimeError("Memory allocation failed for print buffer.");
            goto HANDLE_RUNTIME_ERROR;
        }
        int resultLength = 0;
        int formatIndex = 0;
        int argIndex = 0;
        while (formatIndex < formatStr->length) {
            if (formatIndex + 1 < formatStr->length &&
                formatStr->chars[formatIndex] == '{' &&
                formatStr->chars[formatIndex + 1] == '}') {
                if (argIndex >= argc) {
                    vmRuntimeError("Too few arguments for string interpolation.");
                    free(resultBuffer);
                    goto HANDLE_RUNTIME_ERROR;
                }
                Value arg = regs[base + 1 + argIndex];
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
                        valueLen = 0;
                        break;
                    case VAL_STRING: {
                        valueLen = AS_STRING(arg)->length;
                        if (resultLength + valueLen >= resultCapacity) {
                            resultCapacity = (resultLength + valueLen) * 2;
                            char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
                            if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed for string argument."); goto HANDLE_RUNTIME_ERROR; }
                            resultBuffer = newBuf;
                        }
                        memcpy(resultBuffer + resultLength, AS_STRING(arg)->chars, valueLen);
                        resultLength += valueLen;
                        valueLen = 0;
                        break;
                    }
                    case VAL_ARRAY: {
                        if (!appendValueString(arg, &resultBuffer, &resultLength, &resultCapacity)) {
                            free(resultBuffer);
                            goto HANDLE_RUNTIME_ERROR;
                        }
                        valueLen = 0;
                        break;
                    }
                    case VAL_ERROR:
                        valueLen = snprintf(valueStr, sizeof(valueStr), "Error(%d): %s", AS_ERROR(arg)->type, AS_ERROR(arg)->message->chars);
                        break;
                    case VAL_RANGE_ITERATOR:
                        valueLen = snprintf(valueStr, sizeof(valueStr), "<range %lld..%lld>", (long long)AS_RANGE_ITERATOR(arg)->current, (long long)AS_RANGE_ITERATOR(arg)->end);
                        break;
                    case VAL_ENUM: {
                        ObjEnum* enumObj = AS_ENUM(arg);
                        if (enumObj->dataCount > 0) {
                            valueLen = snprintf(valueStr, sizeof(valueStr), "%s::%d", enumObj->typeName->chars, enumObj->variantIndex);
                        } else {
                            valueLen = snprintf(valueStr, sizeof(valueStr), "%s::%d", enumObj->typeName->chars, enumObj->variantIndex);
                        }
                        break;
                    }
                }
                if (valueLen > 0) {
                    if (resultLength + valueLen >= resultCapacity) {
                        resultCapacity = (resultLength + valueLen) * 2;
                        char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
                        if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed for value conversion."); goto HANDLE_RUNTIME_ERROR; }
                        resultBuffer = newBuf;
                    }
                    memcpy(resultBuffer + resultLength, valueStr, valueLen);
                    resultLength += valueLen;
                }
                formatIndex += 2;
                argIndex++;
            } else {
                if (resultLength + 1 >= resultCapacity) {
                    resultCapacity = (resultLength + 1) * 2;
                    char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
                    if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed while copying character."); goto HANDLE_RUNTIME_ERROR; }
                    resultBuffer = newBuf;
                }
                resultBuffer[resultLength++] = formatStr->chars[formatIndex++];
            }
        }
        if (argIndex < argc) {
            vmRuntimeError("Too many arguments for string interpolation.");
            free(resultBuffer);
            goto HANDLE_RUNTIME_ERROR;
        }
        if (resultLength + 1 >= resultCapacity) {
            resultCapacity = (resultLength + 1) * 2;
            char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
            if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed during final null-termination."); goto HANDLE_RUNTIME_ERROR; }
            resultBuffer = newBuf;
        }
        resultBuffer[resultLength] = '\0';
        printf("%s\n", resultBuffer);
        fflush(stdout);
        free(resultBuffer);
        ip++; DISPATCH();
    }

op_FORMAT_PRINT_NO_NL:
    {
        uint8_t base = ip->dst;
        uint8_t argc = ip->src1;
        if (!IS_STRING(regs[base])) {
            vmRuntimeError("Format string must be a string.");
            goto HANDLE_RUNTIME_ERROR;
        }
        ObjString* formatStr = AS_STRING(regs[base]);
        int resultCapacity = formatStr->length * 2;
        char* resultBuffer = (char*)malloc(resultCapacity);
        if (!resultBuffer) {
            vmRuntimeError("Memory allocation failed for print buffer.");
            goto HANDLE_RUNTIME_ERROR;
        }
        int resultLength = 0;
        int formatIndex = 0;
        int argIndex = 0;
        while (formatIndex < formatStr->length) {
            if (formatIndex + 1 < formatStr->length &&
                formatStr->chars[formatIndex] == '{' &&
                formatStr->chars[formatIndex + 1] == '}') {
                if (argIndex >= argc) {
                    vmRuntimeError("Too few arguments for string interpolation.");
                    free(resultBuffer);
                    goto HANDLE_RUNTIME_ERROR;
                }
                Value arg = regs[base + 1 + argIndex];
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
                        valueLen = 0;
                        break;
                    case VAL_STRING: {
                        valueLen = AS_STRING(arg)->length;
                        if (resultLength + valueLen >= resultCapacity) {
                            resultCapacity = (resultLength + valueLen) * 2;
                            char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
                            if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed for string argument."); goto HANDLE_RUNTIME_ERROR; }
                            resultBuffer = newBuf;
                        }
                        memcpy(resultBuffer + resultLength, AS_STRING(arg)->chars, valueLen);
                        resultLength += valueLen;
                        valueLen = 0;
                        break;
                    }
                    case VAL_ARRAY: {
                        if (!appendValueString(arg, &resultBuffer, &resultLength, &resultCapacity)) {
                            free(resultBuffer);
                            goto HANDLE_RUNTIME_ERROR;
                        }
                        valueLen = 0;
                        break;
                    }
                    case VAL_ERROR:
                        valueLen = snprintf(valueStr, sizeof(valueStr), "Error(%d): %s", AS_ERROR(arg)->type, AS_ERROR(arg)->message->chars);
                        break;
                    case VAL_RANGE_ITERATOR:
                        valueLen = snprintf(valueStr, sizeof(valueStr), "<range %lld..%lld>", (long long)AS_RANGE_ITERATOR(arg)->current, (long long)AS_RANGE_ITERATOR(arg)->end);
                        break;
                    case VAL_ENUM: {
                        ObjEnum* enumObj = AS_ENUM(arg);
                        if (enumObj->dataCount > 0) {
                            valueLen = snprintf(valueStr, sizeof(valueStr), "%s::%d", enumObj->typeName->chars, enumObj->variantIndex);
                        } else {
                            valueLen = snprintf(valueStr, sizeof(valueStr), "%s::%d", enumObj->typeName->chars, enumObj->variantIndex);
                        }
                        break;
                    }
                }
                if (valueLen > 0) {
                    if (resultLength + valueLen >= resultCapacity) {
                        resultCapacity = (resultLength + valueLen) * 2;
                        char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
                        if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed for value conversion."); goto HANDLE_RUNTIME_ERROR; }
                        resultBuffer = newBuf;
                    }
                    memcpy(resultBuffer + resultLength, valueStr, valueLen);
                    resultLength += valueLen;
                }
                formatIndex += 2;
                argIndex++;
            } else {
                if (resultLength + 1 >= resultCapacity) {
                    resultCapacity = (resultLength + 1) * 2;
                    char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
                    if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed while copying character."); goto HANDLE_RUNTIME_ERROR; }
                    resultBuffer = newBuf;
                }
                resultBuffer[resultLength++] = formatStr->chars[formatIndex++];
            }
        }
        if (argIndex < argc) {
            vmRuntimeError("Too many arguments for string interpolation.");
            free(resultBuffer);
            goto HANDLE_RUNTIME_ERROR;
        }
        if (resultLength + 1 >= resultCapacity) {
            resultCapacity = (resultLength + 1) * 2;
            char* newBuf = (char*)realloc(resultBuffer, resultCapacity);
            if (!newBuf) { free(resultBuffer); vmRuntimeError("Memory reallocation failed during final null-termination."); goto HANDLE_RUNTIME_ERROR; }
            resultBuffer = newBuf;
        }
        resultBuffer[resultLength] = '\0';
        printf("%s", resultBuffer);
        fflush(stdout);
        free(resultBuffer);
        ip++; DISPATCH();
    }

op_SETUP_EXCEPT:
    if (vm.tryFrameCount < TRY_MAX) {
        vm.tryFrames[vm.tryFrameCount].handler =
            (uint8_t*)(rvm->chunk->code + ip->dst);
        vm.tryFrames[vm.tryFrameCount].varIndex = ip->src1;
        vm.tryFrames[vm.tryFrameCount].stackDepth =
            (int)(vm.stackTop - vm.stack);
        vm.tryFrameCount++;
    } else {
        vmRuntimeError("Too many nested try blocks.");
    }
    ip++; DISPATCH();

op_SET_GLOBAL:
    vm.globals[ip->dst] = regs[ip->src1];
    ip++; DISPATCH();

op_SHIFT_LEFT_I64:
    regs[ip->dst] = I64_VAL(AS_I64(regs[ip->src1]) << AS_I64(regs[ip->src2]));
    i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    ip++; DISPATCH();

op_SHIFT_RIGHT_I64:
    regs[ip->dst] = I64_VAL(AS_I64(regs[ip->src1]) >> AS_I64(regs[ip->src2]));
    i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    ip++; DISPATCH();

op_SLICE:
    ip++; DISPATCH();

op_SUBSTRING:
    ip++; DISPATCH();

op_SUBTRACT_F64:
    regs[ip->dst] = F64_VAL(AS_F64(regs[ip->src1]) - AS_F64(regs[ip->src2]));
    ip++; DISPATCH();


op_SUBTRACT_I32:
    regs[ip->dst] = I32_VAL(AS_I32(regs[ip->src1]) - AS_I32(regs[ip->src2]));
    ip++; DISPATCH();

op_SUBTRACT_I64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    i64_regs[dest] = i64_regs[s1] - i64_regs[s2];
    SYNC_I64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", dest, (long long)i64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_SUBTRACT_NUMERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32: regs[ip->dst] = I32_VAL(AS_I32(a) - AS_I32(b)); break;
            case VAL_I64: regs[ip->dst] = I64_VAL(AS_I64(a) - AS_I64(b)); break;
            case VAL_U32: regs[ip->dst] = U32_VAL(AS_U32(a) - AS_U32(b)); break;
            case VAL_U64: regs[ip->dst] = U64_VAL(AS_U64(a) - AS_U64(b)); break;
            case VAL_F64: regs[ip->dst] = F64_VAL(AS_F64(a) - AS_F64(b)); break;
            default: regs[ip->dst] = NIL_VAL; break;
        }
    }
    ip++; DISPATCH();
}

op_SUBTRACT_U32:
    regs[ip->dst] = U32_VAL(AS_U32(regs[ip->src1]) - AS_U32(regs[ip->src2]));
    ip++; DISPATCH();

op_SUBTRACT_U64:
    regs[ip->dst] = U64_VAL(AS_U64(regs[ip->src1]) - AS_U64(regs[ip->src2]));
    ip++; DISPATCH();

op_U64_TO_BOOL:
    regs[ip->dst] = BOOL_VAL(AS_U64(regs[ip->src1]) != 0);
    ip++; DISPATCH();

op_U64_TO_I64:
    regs[ip->dst] = I64_VAL((int64_t)AS_U64(regs[ip->src1]));
    i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    ip++; DISPATCH();

op_U64_TO_STRING:
    regs[ip->dst] = convertToString(regs[ip->src1]);
    ip++; DISPATCH();

op_DEFINE_GLOBAL:
    vm.globals[ip->dst] = regs[ip->src1];
    ip++; DISPATCH();

op_GET_GLOBAL:
    regs[ip->dst] = vm.globals[ip->src1];
    ip++; DISPATCH();

op_I64_CONST:
    regs[ip->dst] = rvm->chunk->constants.values[ip->src1];
    if (IS_I64(regs[ip->dst])) i64_regs[ip->dst] = AS_I64(regs[ip->dst]);
    if (IS_F64(regs[ip->dst])) f64_regs[ip->dst] = AS_F64(regs[ip->dst]);
    ip++; DISPATCH();

op_I64_TO_BOOL:
    regs[ip->dst] = BOOL_VAL(AS_I64(regs[ip->src1]) != 0);
    ip++; DISPATCH();

op_I64_TO_U64:
    regs[ip->dst] = U64_VAL((uint64_t)AS_I64(regs[ip->src1]));
    i64_regs[ip->dst] = AS_I64(regs[ip->src1]);
    ip++; DISPATCH();

op_IMPORT:
    {
        uint8_t constantIndex = ip->src1;
        Value pathVal = rvm->chunk->constants.values[constantIndex];
        if (!IS_STRING(pathVal)) {
            vmRuntimeError("Import path must be a string.");
            return NIL_VAL;
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
            vmRuntimeError("Module already executed.");
            return NIL_VAL;
        }
        InterpretResult modRes = interpret_module(pathStr->chars);
        if (modRes != INTERPRET_OK) return NIL_VAL;
        if (vm.moduleCount < UINT8_MAX)
            vm.loadedModules[vm.moduleCount++] = pathStr;
        ip++; DISPATCH();
    }

op_INC_I64:
    i64_regs[ip->dst] += 1;
    SYNC_I64_REG(ip->dst);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] i64_regs[R%d] = %lld\n", ip->dst, (long long)i64_regs[ip->dst]);
#endif
    ip++; DISPATCH();

op_ITER_NEXT_I64:
    ip++; DISPATCH();

op_JUMP_IF_LT_I64:
    if (i64_regs[ip->src1] < i64_regs[ip->src2])
        ip = rvm->chunk->code + ip->dst;
    else
        ip++;
    DISPATCH();

op_LEN_ARRAY:
    regs[ip->dst] = I32_VAL(AS_ARRAY(regs[ip->src1])->length);
    ip++; DISPATCH();

op_LEN_STRING:
    regs[ip->dst] = I32_VAL(AS_STRING(regs[ip->src1])->length);
    ip++; DISPATCH();

op_LESS_EQUAL_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(f64_regs[s1] <= f64_regs[s2]);
    ip++; DISPATCH();
}


op_LESS_EQUAL_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a <= b);
    ip++; DISPATCH();
}

op_LESS_EQUAL_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a <= b);
    ip++; DISPATCH();
}

op_LESS_EQUAL_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a <= b);
    ip++; DISPATCH();
}

op_LESS_EQUAL_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a <= b);
    ip++; DISPATCH();
}

op_LESS_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(f64_regs[s1] < f64_regs[s2]);
    ip++; DISPATCH();
}


op_LESS_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a < b);
    ip++; DISPATCH();
}

op_LESS_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a < b);
    ip++; DISPATCH();
}

op_LESS_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a < b);
    ip++; DISPATCH();
}

op_GREATER_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a > b);
    ip++; DISPATCH();
}

op_GREATER_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a > b);
    ip++; DISPATCH();
}

op_GREATER_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a > b);
    ip++; DISPATCH();
}

op_GREATER_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(f64_regs[s1] > f64_regs[s2]);
    ip++; DISPATCH();
}

op_GREATER_EQUAL_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a >= b);
    ip++; DISPATCH();
}

op_GREATER_EQUAL_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a >= b);
    ip++; DISPATCH();
}

op_GREATER_EQUAL_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a >= b);
    ip++; DISPATCH();
}

op_GREATER_EQUAL_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a >= b);
    ip++; DISPATCH();
}

op_GREATER_EQUAL_F64: {
    uint8_t dest = ip->dst;
    uint8_t s1 = ip->src1;
    uint8_t s2 = ip->src2;
    regs[dest] = BOOL_VAL(f64_regs[s1] >= f64_regs[s2]);
    ip++; DISPATCH();
}



op_EQUAL_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a == b);
    ip++; DISPATCH();
}

op_MODULO_NUMERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32: {
                int32_t bv = AS_I32(b);
                regs[ip->dst] = bv == 0 ? NIL_VAL : I32_VAL(AS_I32(a) % bv);
                break; }
            case VAL_I64: {
                int64_t bv = AS_I64(b);
                regs[ip->dst] = bv == 0 ? NIL_VAL : I64_VAL(AS_I64(a) % bv);
                break; }
            case VAL_U32: {
                uint32_t bv = AS_U32(b);
                regs[ip->dst] = bv == 0 ? NIL_VAL : U32_VAL(AS_U32(a) % bv);
                break; }
            case VAL_U64: {
                uint64_t bv = AS_U64(b);
                regs[ip->dst] = bv == 0 ? NIL_VAL : U64_VAL(AS_U64(a) % bv);
                break; }
            default:
                regs[ip->dst] = NIL_VAL;
        }
    }
    ip++; DISPATCH();
}

op_NEGATE_F64: {
    uint8_t dest = ip->dst;
    uint8_t src = ip->src1;
    f64_regs[dest] = -f64_regs[src];
    SYNC_F64_REG(dest);
#ifdef DEBUG_TRACE_EXECUTION
    printf("[Debug] f64_regs[R%d] = %f\n", dest, f64_regs[dest]);
#endif
    ip++; DISPATCH();
}

op_NEGATE_I32:
    regs[ip->dst] = I32_VAL(-AS_I32(regs[ip->src1]));
    ip++; DISPATCH();

op_NEGATE_U32:
    regs[ip->dst] = U32_VAL(-AS_U32(regs[ip->src1]));
    ip++; DISPATCH();

op_NEGATE_U64:
    regs[ip->dst] = U64_VAL(-AS_U64(regs[ip->src1]));
    ip++; DISPATCH();

op_NEGATE_NUMERIC: {
    Value a = regs[ip->src1];
    switch (a.type) {
        case VAL_I32: regs[ip->dst] = I32_VAL(-AS_I32(a)); break;
        case VAL_I64: regs[ip->dst] = I64_VAL(-AS_I64(a)); break;
        case VAL_U32: regs[ip->dst] = U32_VAL(-AS_U32(a)); break;
        case VAL_U64: regs[ip->dst] = U64_VAL(-AS_U64(a)); break;
        case VAL_F64: regs[ip->dst] = F64_VAL(-AS_F64(a)); break;
        default: regs[ip->dst] = NIL_VAL; break;
    }
    ip++; DISPATCH();
}

op_MULTIPLY_F64: {
    double a = AS_F64(regs[ip->src1]);
    double b = AS_F64(regs[ip->src2]);
    regs[ip->dst] = F64_VAL(a * b);
    ip++; DISPATCH();
}

op_MULTIPLY_I32: {
    int32_t a = AS_I32(regs[ip->src1]);
    int32_t b = AS_I32(regs[ip->src2]);
    regs[ip->dst] = I32_VAL(a * b);
    ip++; DISPATCH();
}

op_MULTIPLY_U32: {
    uint32_t a = AS_U32(regs[ip->src1]);
    uint32_t b = AS_U32(regs[ip->src2]);
    regs[ip->dst] = U32_VAL(a * b);
    ip++; DISPATCH();
}

op_MULTIPLY_U64: {
    uint64_t a = AS_U64(regs[ip->src1]);
    uint64_t b = AS_U64(regs[ip->src2]);
    regs[ip->dst] = U64_VAL(a * b);
    ip++; DISPATCH();
}

op_MULTIPLY_NUMERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32:
                regs[ip->dst] = I32_VAL(AS_I32(a) * AS_I32(b));
                break;
            case VAL_I64:
                regs[ip->dst] = I64_VAL(AS_I64(a) * AS_I64(b));
                break;
            case VAL_U32:
                regs[ip->dst] = U32_VAL(AS_U32(a) * AS_U32(b));
                break;
            case VAL_U64:
                regs[ip->dst] = U64_VAL(AS_U64(a) * AS_U64(b));
                break;
            case VAL_F64:
                regs[ip->dst] = F64_VAL(AS_F64(a) * AS_F64(b));
                break;
            default:
                regs[ip->dst] = NIL_VAL;
        }
    }
    ip++; DISPATCH();
}

op_DIVIDE_F64: {
    double a = AS_F64(regs[ip->src1]);
    double b = AS_F64(regs[ip->src2]);
    if (b == 0.0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = F64_VAL(a / b);
    ip++; DISPATCH();
}

op_DIVIDE_I32: {
    int32_t b = AS_I32(regs[ip->src2]);
    int32_t a = AS_I32(regs[ip->src1]);
    if (b == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = I32_VAL(a / b);
    ip++; DISPATCH();
}

op_DIVIDE_U32: {
    uint32_t b = AS_U32(regs[ip->src2]);
    uint32_t a = AS_U32(regs[ip->src1]);
    if (b == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = U32_VAL(a / b);
    ip++; DISPATCH();
}

op_DIVIDE_U64: {
    uint64_t b = AS_U64(regs[ip->src2]);
    uint64_t a = AS_U64(regs[ip->src1]);
    if (b == 0) {
        vmRuntimeError("Division by zero.");
        DISPATCH();
    }
    regs[ip->dst] = U64_VAL(a / b);
    ip++; DISPATCH();
}

op_DIVIDE_NUMERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32: {
                int32_t bv = AS_I32(b);
                if (bv == 0) { vmRuntimeError("Division by zero."); DISPATCH(); }
                regs[ip->dst] = I32_VAL(AS_I32(a) / bv);
                break; }
            case VAL_I64: {
                int64_t bv = AS_I64(b);
                if (bv == 0) { vmRuntimeError("Division by zero."); DISPATCH(); }
                regs[ip->dst] = I64_VAL(AS_I64(a) / bv);
                break; }
            case VAL_U32: {
                uint32_t bv = AS_U32(b);
                if (bv == 0) { vmRuntimeError("Division by zero."); DISPATCH(); }
                regs[ip->dst] = U32_VAL(AS_U32(a) / bv);
                break; }
            case VAL_U64: {
                uint64_t bv = AS_U64(b);
                if (bv == 0) { vmRuntimeError("Division by zero."); DISPATCH(); }
                regs[ip->dst] = U64_VAL(AS_U64(a) / bv);
                break; }
            case VAL_F64: {
                double bv = AS_F64(b);
                if (bv == 0.0) { vmRuntimeError("Division by zero."); DISPATCH(); }
                regs[ip->dst] = F64_VAL(AS_F64(a) / bv);
                break; }
            default:
                regs[ip->dst] = NIL_VAL;
        }
    }
    ip++; DISPATCH();
}

/* Missing implementations for opcodes that were in enum but not dispatch table */
op_CALL_BUILTIN_SLICE:
    // TODO: Implement call builtin slice
    ip++; DISPATCH();

op_SPILL_REG:
    // TODO: Implement spill register
    ip++; DISPATCH();

op_UNSPILL_REG:
    // TODO: Implement unspill register
    ip++; DISPATCH();

/* Modern Array Operations - Phase 5.1.2 */
op_ARRAY_NEW: {
    int capacity = ip->src1;
    ObjArray* array = allocateArray(capacity);
    regs[ip->dst] = ARRAY_VAL(array);
    ip++; DISPATCH();
}

op_ARRAY_LEN:
    if (IS_ARRAY(regs[ip->src1])) {
        regs[ip->dst] = I32_VAL(AS_ARRAY(regs[ip->src1])->length);
    } else {
        regs[ip->dst] = I32_VAL(0);
    }
    ip++; DISPATCH();

op_ARRAY_INSERT:
    // TODO: Implement array insert
    ip++; DISPATCH();

op_ARRAY_REMOVE:
    // TODO: Implement array remove
    ip++; DISPATCH();

op_ARRAY_SLICE:
    // TODO: Implement array slice
    ip++; DISPATCH();

op_ARRAY_CONCAT:
    // TODO: Implement array concat
    ip++; DISPATCH();

op_ARRAY_REVERSE:
    // TODO: Implement array reverse
    ip++; DISPATCH();

op_ARRAY_SORT:
    // TODO: Implement array sort
    ip++; DISPATCH();

#else
    while (true) {
        if (++loop_guard > 10000000000LL) {
            fprintf(stderr, "[VM] Infinite loop guard triggered.\n");
            exit(1);
        }
        RegisterInstr instr = *vm->ip++;
        vm.instruction_count++;
        if (vm.instruction_count % 10000 == 0 && !vm.gcPaused) {
            collectGarbage();
        }
        switch (instr.opcode) {
            case ROP_NOP:
                break;
            case ROP_MOV:
                rvm->registers[instr.dst] = rvm->registers[instr.src1];
                break;
            case ROP_LOAD_CONST:
                rvm->registers[instr.dst] = rvm->chunk->constants.values[instr.src1];
                break;
            case ROP_ADD_RR: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                rvm->registers[instr.dst] = I64_VAL(AS_I64(a) + AS_I64(b));
                break;
            }
            case ROP_SUB_RR: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                rvm->registers[instr.dst] = I64_VAL(AS_I64(a) - AS_I64(b));
                break;
            }
            case ROP_MUL_RR: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                rvm->registers[instr.dst] = I64_VAL(AS_I64(a) * AS_I64(b));
                break;
            }
            case ROP_DIV_RR: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (AS_I64(b) == 0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                rvm->registers[instr.dst] = I64_VAL(AS_I64(a) / AS_I64(b));
                break;
            }
            case ROP_EQ_I64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->i64_regs[instr.src1] == rvm->i64_regs[instr.src2]);
                break;
            }
            case ROP_NE_I64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->i64_regs[instr.src1] != rvm->i64_regs[instr.src2]);
                break;
            }
            case ROP_EQUAL:
                rvm->registers[instr.dst] = BOOL_VAL(valuesEqual(rvm->registers[instr.src1],
                                                                rvm->registers[instr.src2]));
                break;
            case ROP_EQUAL_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a == b);
                break;
            }
            case ROP_NOT_EQUAL:
                rvm->registers[instr.dst] = BOOL_VAL(!valuesEqual(rvm->registers[instr.src1],
                                                                 rvm->registers[instr.src2]));
                break;
            case ROP_NOT_EQUAL_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a != b);
                break;
            }
            case ROP_EQ_F64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->f64_regs[instr.src1] == rvm->f64_regs[instr.src2]);
                break;
            }
            case ROP_NE_F64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->f64_regs[instr.src1] != rvm->f64_regs[instr.src2]);
                break;
            }
            case ROP_LT_I64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->i64_regs[instr.src1] < rvm->i64_regs[instr.src2]);
                break;
            }
            case ROP_LE_I64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->i64_regs[instr.src1] <= rvm->i64_regs[instr.src2]);
                break;
            }
            case ROP_GT_I64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->i64_regs[instr.src1] > rvm->i64_regs[instr.src2]);
                break;
            }
            case ROP_GE_I64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->i64_regs[instr.src1] >= rvm->i64_regs[instr.src2]);
                break;
            }
            case ROP_GREATER_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a > b);
                break;
            }
            case ROP_GREATER_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a > b);
                break;
            }
            case ROP_GREATER_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a > b);
                break;
            }
            case ROP_GREATER_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a > b);
                break;
            }
            case ROP_GREATER_F64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->f64_regs[instr.src1] > rvm->f64_regs[instr.src2]);
                break;
            }
            case ROP_GREATER_EQUAL_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a >= b);
                break;
            }
            case ROP_GREATER_EQUAL_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a >= b);
                break;
            }
            case ROP_GREATER_EQUAL_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a >= b);
                break;
            }
            case ROP_GREATER_EQUAL_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a >= b);
                break;
            }
            case ROP_GREATER_EQUAL_F64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->f64_regs[instr.src1] >= rvm->f64_regs[instr.src2]);
                break;
            }
            case ROP_LESS_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a < b);
                break;
            }
            case ROP_LESS_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a < b);
                break;
            }
            case ROP_LESS_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a < b);
                break;
            }
            case ROP_LESS_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a < b);
                break;
            }
            case ROP_LESS_F64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->f64_regs[instr.src1] < rvm->f64_regs[instr.src2]);
                break;
            }
            case ROP_LESS_EQUAL_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a <= b);
                break;
            }
            case ROP_LESS_EQUAL_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a <= b);
                break;
            }
            case ROP_LESS_EQUAL_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a <= b);
                break;
            }
            case ROP_LESS_EQUAL_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a <= b);
                break;
            }
            case ROP_LESS_EQUAL_F64: {
                rvm->registers[instr.dst] = BOOL_VAL(rvm->f64_regs[instr.src1] <= rvm->f64_regs[instr.src2]);
                break;
            }
            case ROP_PRINT:
                printValue(rvm->registers[instr.src1]);
                putchar('\n');
                break;
            case ROP_PRINT_NO_NL:
                printValue(rvm->registers[instr.src1]);
                fflush(stdout);
                break;
            case ROP_LOAD_GLOBAL:
                rvm->registers[instr.dst] = vm.globals[instr.src1];
                if (IS_I64(rvm->registers[instr.dst]))
                    rvm->i64_regs[instr.dst] = AS_I64(rvm->registers[instr.dst]);
                if (IS_F64(rvm->registers[instr.dst]))
                    rvm->f64_regs[instr.dst] = AS_F64(rvm->registers[instr.dst]);
                break;
            case ROP_STORE_GLOBAL:
                vm.globals[instr.dst] = rvm->registers[instr.src1];
                break;
            case ROP_ADD_F64: {
                double a = AS_F64(rvm->registers[instr.src1]);
                double b = AS_F64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = F64_VAL(a + b);
                break;
            }
            case ROP_SUB_F64: {
                double a = AS_F64(rvm->registers[instr.src1]);
                double b = AS_F64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = F64_VAL(a - b);
                break;
            }
            case ROP_MUL_F64: {
                double a = AS_F64(rvm->registers[instr.src1]);
                double b = AS_F64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = F64_VAL(a * b);
                break;
            }
            case ROP_DIV_F64: {
                double a = AS_F64(rvm->registers[instr.src1]);
                double b = AS_F64(rvm->registers[instr.src2]);
                if (b == 0.0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                rvm->registers[instr.dst] = F64_VAL(a / b);
                break;
            }
            case ROP_MOD_I64: {
                int64_t b = rvm->i64_regs[instr.src2];
                if (b == 0) {
                    rvm->i64_regs[instr.dst] = 0;
                    rvm->registers[instr.dst] = I64_VAL(0);
                } else {
                    int64_t r = rvm->i64_regs[instr.src1] % b;
                    if (r < 0) r += (b < 0) ? -b : b;
                    rvm->i64_regs[instr.dst] = r;
                    rvm->registers[instr.dst] = I64_VAL(r);
                }
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] i64_regs[R%d] = %lld\n", instr.dst, (long long)rvm->i64_regs[instr.dst]);
#endif
                break;
            }
            case ROP_BIT_AND_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a & b);
                break;
            }
            case ROP_BIT_OR_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a | b);
                break;
            }
            case ROP_BIT_XOR_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a ^ b);
                break;
            }
            case ROP_BIT_NOT_I64:
                rvm->registers[instr.dst] = I64_VAL(~AS_I64(rvm->registers[instr.src1]));
                break;
            case ROP_SHL_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a << b);
                break;
            }
            case ROP_SHR_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a >> b);
                break;
            }
            case ROP_MAKE_ARRAY: {
                int count = instr.src1;
                ObjArray* arr = allocateArray(count);
                rvm->registers[instr.dst] = ARRAY_VAL(arr);
                break;
            }
            case ROP_ARRAY_GET: {
                Value arrayVal = rvm->registers[instr.src1];
                Value indexVal = rvm->registers[instr.src2];
                int64_t index = 0;
                if (IS_I64(indexVal))       index = AS_I64(indexVal);
                else if (IS_I32(indexVal))  index = AS_I32(indexVal);
                else if (IS_U32(indexVal))  index = (int64_t)AS_U32(indexVal);
                else if (IS_U64(indexVal))  index = (int64_t)AS_U64(indexVal);
                else {
                    vmRuntimeError("Expected integer index.");
                    goto check_error;
                }

                if (!IS_ARRAY(arrayVal)) {
                    vmRuntimeError("Expected array for indexing.");
                    goto check_error;
                }

                ObjArray* arr = AS_ARRAY(arrayVal);
#ifdef DEBUG_ARRAY_INDEX
                printf("[DEBUG] GET index: i64[%d] = %ld, array length = %d\n",
                       instr.src2, index, arr->length);
#endif
                if (index < 0 || index >= arr->length) {
                    vmRuntimeError("Array index out of bounds.");
                    goto check_error;
                }

                rvm->registers[instr.dst] = arr->elements[index];
                break;
            }
            case ROP_ARRAY_SET: {
                Value arrayVal = rvm->registers[instr.dst];
                Value indexVal = rvm->registers[instr.src1];
                int64_t index = 0;
                if (IS_I64(indexVal))       index = AS_I64(indexVal);
                else if (IS_I32(indexVal))  index = AS_I32(indexVal);
                else if (IS_U32(indexVal))  index = (int64_t)AS_U32(indexVal);
                else if (IS_U64(indexVal))  index = (int64_t)AS_U64(indexVal);
                else {
                    vmRuntimeError("Expected integer index.");
                    goto check_error;
                }
                Value value = rvm->registers[instr.src2];

                if (!IS_ARRAY(arrayVal)) {
                    vmRuntimeError("Expected array for assignment.");
                    goto check_error;
                }

                ObjArray* arr = AS_ARRAY(arrayVal);
#ifdef DEBUG_ARRAY_INDEX
                printf("[DEBUG] SET index: i64[%d] = %ld, array length = %d\n",
                       instr.src1, index, arr->length);
#endif
                if (index < 0 || index >= arr->length) {
                    vmRuntimeError("Array index out of bounds.");
                    goto check_error;
                }

                arr->elements[index] = value;
                break;
            }
            case ROP_ARRAY_PUSH:
                if (IS_ARRAY(rvm->registers[instr.dst])) {
                    arrayPush(&vm, AS_ARRAY(rvm->registers[instr.dst]), rvm->registers[instr.src2]);
                }
                break;
            case ROP_ARRAY_POP:
                if (IS_ARRAY(rvm->registers[instr.dst])) {
                    rvm->registers[instr.dst] = arrayPop(AS_ARRAY(rvm->registers[instr.dst]));
                } else {
                    rvm->registers[instr.dst] = NIL_VAL;
                }
                break;
            case ROP_LEN:
                if (IS_ARRAY(rvm->registers[instr.src1]))
                    rvm->registers[instr.dst] = I32_VAL(AS_ARRAY(rvm->registers[instr.src1])->length);
                else if (IS_STRING(rvm->registers[instr.src1]))
                    rvm->registers[instr.dst] = I32_VAL(AS_STRING(rvm->registers[instr.src1])->length);
                else
                    rvm->registers[instr.dst] = I32_VAL(0);
                break;
            case ROP_I64_TO_STRING:
                rvm->registers[instr.dst] = convertToString(rvm->registers[instr.src1]);
                break;
            case ROP_ARRAY_RESERVE: {
                ObjArray* arr = AS_ARRAY(rvm->registers[instr.dst]);
                int cap = (int)AS_I64(rvm->registers[instr.src2]);
                if (cap > arr->capacity) {
                    int oldCap = arr->capacity;
                    arr->capacity = cap;
                    arr->elements = GROW_ARRAY(Value, arr->elements, oldCap, arr->capacity);
                    vm.bytesAllocated += sizeof(Value) * (arr->capacity - oldCap);
                }
                break;
            }
            case ROP_CONCAT: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (!IS_STRING(a)) a = convertToString(a);
                if (!IS_STRING(b)) b = convertToString(b);
                int len = AS_STRING(a)->length + AS_STRING(b)->length;
                char* chars = (char*)malloc(len + 1);
                memcpy(chars, AS_STRING(a)->chars, AS_STRING(a)->length);
                memcpy(chars + AS_STRING(a)->length, AS_STRING(b)->chars, AS_STRING(b)->length);
                chars[len] = '\0';
                ObjString* result = allocateString(chars, len);
                free(chars);
                rvm->registers[instr.dst] = STRING_VAL(result);
                break;
            }
            case ROP_TYPE_OF_I32:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("i32", 3));
                break;
            case ROP_TYPE_OF_I64:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("i64", 3));
                break;
            case ROP_TYPE_OF_U32:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("u32", 3));
                break;
            case ROP_TYPE_OF_U64:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("u64", 3));
                break;
            case ROP_TYPE_OF_F64:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("f64", 3));
                break;
            case ROP_TYPE_OF_BOOL:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("bool", 4));
                break;
            case ROP_TYPE_OF_STRING:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("string", 6));
                break;
            case ROP_TYPE_OF_ARRAY:
                rvm->registers[instr.dst] = STRING_VAL(allocateString("array", 5));
                break;
            case ROP_GC_PAUSE:
                pauseGC();
                break;
            case ROP_GC_RESUME:
                resumeGC();
                break;
            case ROP_ADD_I64: {
                i64_regs[instr.dst] = i64_regs[instr.src1] + i64_regs[instr.src2];
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] i64_regs[R%d] = %lld\n", instr.dst, (long long)i64_regs[instr.dst]);
#endif
                break;
            }
            case ROP_ADD_NUMERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32:
                            rvm->registers[instr.dst] = I32_VAL(AS_I32(a) + AS_I32(b));
                            break;
                        case VAL_I64:
                            rvm->registers[instr.dst] = I64_VAL(AS_I64(a) + AS_I64(b));
                            break;
                        case VAL_U32:
                            rvm->registers[instr.dst] = U32_VAL(AS_U32(a) + AS_U32(b));
                            break;
                        case VAL_U64:
                            rvm->registers[instr.dst] = U64_VAL(AS_U64(a) + AS_U64(b));
                            break;
                        case VAL_F64:
                            rvm->registers[instr.dst] = F64_VAL(AS_F64(a) + AS_F64(b));
                            break;
                        default:
                            rvm->registers[instr.dst] = NIL_VAL;
                    }
                }
                break;
            }
            case ROP_MULTIPLY_GENERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32:
                            rvm->registers[instr.dst] = I32_VAL(AS_I32(a) * AS_I32(b));
                            break;
                        case VAL_I64:
                            rvm->registers[instr.dst] = I64_VAL(AS_I64(a) * AS_I64(b));
                            break;
                        case VAL_U32:
                            rvm->registers[instr.dst] = U32_VAL(AS_U32(a) * AS_U32(b));
                            break;
                        case VAL_U64:
                            rvm->registers[instr.dst] = U64_VAL(AS_U64(a) * AS_U64(b));
                            break;
                        case VAL_F64:
                            rvm->registers[instr.dst] = F64_VAL(AS_F64(a) * AS_F64(b));
                            break;
                        default:
                            rvm->registers[instr.dst] = NIL_VAL;
                    }
                }
                break;
            }
            case ROP_DIVIDE_GENERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32: {
                            int32_t bv = AS_I32(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = I32_VAL(AS_I32(a) / bv);
                            break; }
                        case VAL_I64: {
                            int64_t bv = AS_I64(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = I64_VAL(AS_I64(a) / bv);
                            break; }
                        case VAL_U32: {
                            uint32_t bv = AS_U32(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = U32_VAL(AS_U32(a) / bv);
                            break; }
                        case VAL_U64: {
                            uint64_t bv = AS_U64(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = U64_VAL(AS_U64(a) / bv);
                            break; }
                        case VAL_F64: {
                            double bv = AS_F64(b);
                            if (bv == 0.0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = F64_VAL(AS_F64(a) / bv);
                            break; }
                        default:
                            rvm->registers[instr.dst] = NIL_VAL;
                    }
                }
                break;
            }
            case ROP_MODULO_GENERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32: {
                            int32_t bv = AS_I32(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : I32_VAL(AS_I32(a) % bv);
                            break; }
                        case VAL_I64: {
                            int64_t bv = AS_I64(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : I64_VAL(AS_I64(a) % bv);
                            break; }
                        case VAL_U32: {
                            uint32_t bv = AS_U32(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : U32_VAL(AS_U32(a) % bv);
                            break; }
                        case VAL_U64: {
                            uint64_t bv = AS_U64(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : U64_VAL(AS_U64(a) % bv);
                            break; }
                        default:
                            rvm->registers[instr.dst] = NIL_VAL;
                    }
                }
                break;
            }
            case ROP_MODULO_I32: {
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                rvm->registers[instr.dst] = b == 0 ? NIL_VAL : I32_VAL(a % b);
                break;
            }
            case ROP_MODULO_U32: {
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                rvm->registers[instr.dst] = b == 0 ? NIL_VAL : U32_VAL(a % b);
                break;
            }
            case ROP_MODULO_U64: {
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                rvm->registers[instr.dst] = b == 0 ? NIL_VAL : U64_VAL(a % b);
                break;
            }
            case ROP_MODULO_I64: {
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                rvm->registers[instr.dst] = b == 0 ? NIL_VAL : I64_VAL(a % b);
                break;
            }
            case ROP_MODULO_NUMERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32: {
                            int32_t bv = AS_I32(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : I32_VAL(AS_I32(a) % bv);
                            break; }
                        case VAL_I64: {
                            int64_t bv = AS_I64(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : I64_VAL(AS_I64(a) % bv);
                            break; }
                        case VAL_U32: {
                            uint32_t bv = AS_U32(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : U32_VAL(AS_U32(a) % bv);
                            break; }
                        case VAL_U64: {
                            uint64_t bv = AS_U64(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : U64_VAL(AS_U64(a) % bv);
                            break; }
                        default:
                            rvm->registers[instr.dst] = NIL_VAL;
                    }
                }
                break;
            }
            case ROP_NEGATE_GENERIC: {
                Value a = rvm->registers[instr.src1];
                switch (a.type) {
                    case VAL_I32:
                        rvm->registers[instr.dst] = I32_VAL(-AS_I32(a));
                        break;
                    case VAL_I64:
                        rvm->registers[instr.dst] = I64_VAL(-AS_I64(a));
                        break;
                    case VAL_U32:
                        rvm->registers[instr.dst] = U32_VAL(-AS_U32(a));
                        break;
                    case VAL_U64:
                        rvm->registers[instr.dst] = U64_VAL(-AS_U64(a));
                        break;
                    case VAL_F64:
                        rvm->registers[instr.dst] = F64_VAL(-AS_F64(a));
                        break;
                    default:
                        rvm->registers[instr.dst] = NIL_VAL;
                }
                break;
            }
            case ROP_NEGATE_F64:
                rvm->f64_regs[instr.dst] = -rvm->f64_regs[instr.src1];
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] f64_regs[R%d] = %f\n", instr.dst, rvm->f64_regs[instr.dst]);
#endif
                break;
            case ROP_NEGATE_I32:
                rvm->registers[instr.dst] = I32_VAL(-AS_I32(rvm->registers[instr.src1]));
                break;
            case ROP_NEGATE_U32:
                rvm->registers[instr.dst] = U32_VAL(-AS_U32(rvm->registers[instr.src1]));
                break;
            case ROP_NEGATE_U64:
                rvm->registers[instr.dst] = U64_VAL(-AS_U64(rvm->registers[instr.src1]));
                break;
            case ROP_NEGATE_NUMERIC: {
                Value a = rvm->registers[instr.src1];
                switch (a.type) {
                    case VAL_I32: rvm->registers[instr.dst] = I32_VAL(-AS_I32(a)); break;
                    case VAL_I64: rvm->registers[instr.dst] = I64_VAL(-AS_I64(a)); break;
                    case VAL_U32: rvm->registers[instr.dst] = U32_VAL(-AS_U32(a)); break;
                    case VAL_U64: rvm->registers[instr.dst] = U64_VAL(-AS_U64(a)); break;
                    case VAL_F64: rvm->registers[instr.dst] = F64_VAL(-AS_F64(a)); break;
                    default: rvm->registers[instr.dst] = NIL_VAL; break;
                }
                break;
            }
            case ROP_SUBTRACT_I64: {
                i64_regs[instr.dst] = i64_regs[instr.src1] - i64_regs[instr.src2];
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] i64_regs[R%d] = %lld\n", instr.dst, (long long)i64_regs[instr.dst]);
#endif
                break;
            }
            case ROP_NEGATE_I64:
                rvm->registers[instr.dst] = I64_VAL(-AS_I64(rvm->registers[instr.src1]));
                break;
            case ROP_BOOL_TO_I64:
                rvm->registers[instr.dst] = I64_VAL(AS_BOOL(rvm->registers[instr.src1]) ? 1 : 0);
                break;
            case ROP_BOOL_TO_U64:
                rvm->registers[instr.dst] = U64_VAL(AS_BOOL(rvm->registers[instr.src1]) ? 1u : 0u);
                break;
            case ROP_BREAK:
                break;
            case ROP_CALL_NATIVE: {
                NativeFunction* nf = &vm.nativeFunctions[instr.src1];
                uint8_t argc = instr.src2;
                Value* args = &rvm->registers[instr.dst];
                Value result = nf->function(argc, args);
                rvm->registers[instr.dst] = result;
                if (IS_I64(result)) rvm->i64_regs[instr.dst] = AS_I64(result);
                if (IS_F64(result)) rvm->f64_regs[instr.dst] = AS_F64(result);
                break;
            }
            case ROP_CONSTANT:
                rvm->registers[instr.dst] = rvm->chunk->constants.values[instr.src1];
                break;
            case ROP_CONSTANT_LONG:
                rvm->registers[instr.dst] = rvm->chunk->constants.values[instr.src1];
                break;
            case ROP_CONTINUE:
                break;
            case ROP_EQUAL:
                rvm->registers[instr.dst] = BOOL_VAL(valuesEqual(rvm->registers[instr.src1], rvm->registers[instr.src2]));
                break;
            case ROP_NOT_EQUAL:
                rvm->registers[instr.dst] = BOOL_VAL(!valuesEqual(rvm->registers[instr.src1], rvm->registers[instr.src2]));
                break;
            case ROP_JUMP_IF_FALSE:
                if ((IS_BOOL(rvm->registers[instr.src1]) && !AS_BOOL(rvm->registers[instr.src1])) ||
                    (IS_I64(rvm->registers[instr.src1]) && AS_I64(rvm->registers[instr.src1]) == 0)) {
                    rvm->ip = rvm->chunk->code + instr.dst;
                }
                break;
            case ROP_JUMP_IF_TRUE:
                if ((IS_BOOL(rvm->registers[instr.src1]) && AS_BOOL(rvm->registers[instr.src1])) ||
                    (IS_I64(rvm->registers[instr.src1]) && AS_I64(rvm->registers[instr.src1]) != 0)) {
                    rvm->ip = rvm->chunk->code + instr.dst;
                }
                break;
            case ROP_LOOP:
                rvm->ip = rvm->chunk->code + instr.dst;
                break;
            case ROP_POP:
                rvm->registers[instr.dst] = NIL_VAL;
                break;
            case ROP_RETURN: {
                Value ret = rvm->registers[instr.src1];
                if (vm.regFrameCount == 0) {
                    return ret;
                }
                uint8_t dest;
                popRegisterFrame(rvm, &dest);
                rvm->registers[dest] = ret;
                if (IS_I64(ret)) rvm->i64_regs[dest] = AS_I64(ret);
                if (IS_F64(ret)) rvm->f64_regs[dest] = AS_F64(ret);
                rvm->ip = rvm->ip; // ensure ip restored
                break;
            }
            case ROP_JUMP:
                rvm->ip = rvm->chunk->code + instr.dst;
                break;
            case ROP_JZ:
                if ((IS_BOOL(rvm->registers[instr.src1]) && !AS_BOOL(rvm->registers[instr.src1])) ||
                    (IS_I64(rvm->registers[instr.src1]) && AS_I64(rvm->registers[instr.src1]) == 0)) {
                    rvm->ip = rvm->chunk->code + instr.dst;
                }
                break;
            case ROP_CALL: {
                uint8_t base = instr.dst;
                uint8_t globalIndex = instr.src1;
                uint8_t argc = instr.src2;
                if ((int)globalIndex >= UINT8_COUNT || !IS_I32(vm.globals[globalIndex])) {
                    vmRuntimeError("Attempt to call a non-function.");
                    return NIL_VAL;
                }
                int funcIndex = AS_I32(vm.globals[globalIndex]);
                if (funcIndex < 0 || funcIndex >= vm.regChunk.functionCount) {
                    vmRuntimeError("Invalid function index.");
                    return NIL_VAL;
                }
                int target = vm.regChunk.functionOffsets[funcIndex];
                if (target < 0) {
                    vmRuntimeError("Missing register offset for function.");
                    return NIL_VAL;
                }
                if (!pushRegisterFrame(rvm, rvm->ip + 1, rvm->chunk, base)) {
                    return NIL_VAL;
                }
                rvm->chunk = &vm.regChunk;
                rvm->ip = rvm->chunk->code + target;
                for (int i = 0; i < argc && i < REGISTER_COUNT; i++) {
                    rvm->registers[i] = rvm->registers[base + i];
                    rvm->i64_regs[i] = rvm->i64_regs[base + i];
                    rvm->f64_regs[i] = rvm->f64_regs[base + i];
                }
                for (int r = argc; r < REGISTER_COUNT; r++) {
                    rvm->registers[r] = NIL_VAL;
                    rvm->i64_regs[r] = 0;
                    rvm->f64_regs[r] = 0.0;
                }
                break;
            }
            case ROP_ADD_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a + b);
                break;
            }
            case ROP_SUB_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a - b);
                break;
            }
            case ROP_MUL_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a * b);
                break;
            }
            case ROP_DIV_I32: {
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                if (b == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                rvm->registers[instr.dst] = I32_VAL(a / b);
                break;
            }
            case ROP_ADD_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a + b);
                break;
            }
            case ROP_SUB_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a - b);
                break;
            }
            case ROP_MUL_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a * b);
                break;
            }
            case ROP_DIV_U32: {
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                if (b == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                rvm->registers[instr.dst] = U32_VAL(a / b);
                break;
            }
            case ROP_ADD_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U64_VAL(a + b);
                break;
            }
            case ROP_SUB_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U64_VAL(a - b);
                break;
            }
            case ROP_MUL_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U64_VAL(a * b);
                break;
            }
            case ROP_MULTIPLY_F64: {
                double a = AS_F64(rvm->registers[instr.src1]);
                double b = AS_F64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = F64_VAL(a * b);
                break;
            }
            case ROP_MULTIPLY_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a * b);
                break;
            }
            case ROP_MULTIPLY_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a * b);
                break;
            }
            case ROP_MULTIPLY_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U64_VAL(a * b);
                break;
            }
            case ROP_MULTIPLY_NUMERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32:
                            rvm->registers[instr.dst] = I32_VAL(AS_I32(a) * AS_I32(b));
                            break;
                        case VAL_I64:
                            rvm->registers[instr.dst] = I64_VAL(AS_I64(a) * AS_I64(b));
                            break;
                        case VAL_U32:
                            rvm->registers[instr.dst] = U32_VAL(AS_U32(a) * AS_U32(b));
                            break;
                        case VAL_U64:
                            rvm->registers[instr.dst] = U64_VAL(AS_U64(a) * AS_U64(b));
                            break;
                        case VAL_F64:
                            rvm->registers[instr.dst] = F64_VAL(AS_F64(a) * AS_F64(b));
                            break;
                        default:
                            rvm->registers[instr.dst] = NIL_VAL;
                    }
                }
                break;
            }
            case ROP_MULTIPLY_I64: {
                i64_regs[instr.dst] = i64_regs[instr.src1] * i64_regs[instr.src2];
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] i64_regs[R%d] = %lld\n", instr.dst, (long long)i64_regs[instr.dst]);
#endif
                break;
            }
            case ROP_DIV_U64: {
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                if (b == 0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                rvm->registers[instr.dst] = U64_VAL(a / b);
                break;
            }
            case ROP_DIVIDE_I64: {
                int64_t b = i64_regs[instr.src2];
                if (b == 0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                i64_regs[instr.dst] = i64_regs[instr.src1] / b;
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] i64_regs[R%d] = %lld\n", instr.dst, (long long)i64_regs[instr.dst]);
#endif
                break;
            }
            case ROP_DIVIDE_F64: {
                double a = AS_F64(rvm->registers[instr.src1]);
                double b = AS_F64(rvm->registers[instr.src2]);
                if (b == 0.0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                rvm->registers[instr.dst] = F64_VAL(a / b);
                break;
            }
            case ROP_DIVIDE_I32: {
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                if (b == 0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                rvm->registers[instr.dst] = I32_VAL(a / b);
                break;
            }
            case ROP_DIVIDE_U32: {
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                if (b == 0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                rvm->registers[instr.dst] = U32_VAL(a / b);
                break;
            }
            case ROP_DIVIDE_U64: {
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                if (b == 0) {
                    vmRuntimeError("Division by zero.");
                    goto check_error;
                }
                rvm->registers[instr.dst] = U64_VAL(a / b);
                break;
            }
            case ROP_DIVIDE_NUMERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32: {
                            int32_t bv = AS_I32(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = I32_VAL(AS_I32(a) / bv);
                            break; }
                        case VAL_I64: {
                            int64_t bv = AS_I64(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = I64_VAL(AS_I64(a) / bv);
                            break; }
                        case VAL_U32: {
                            uint32_t bv = AS_U32(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = U32_VAL(AS_U32(a) / bv);
                            break; }
                        case VAL_U64: {
                            uint64_t bv = AS_U64(b);
                            if (bv == 0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = U64_VAL(AS_U64(a) / bv);
                            break; }
                        case VAL_F64: {
                            double bv = AS_F64(b);
                            if (bv == 0.0) { vmRuntimeError("Division by zero."); goto check_error; }
                            rvm->registers[instr.dst] = F64_VAL(AS_F64(a) / bv);
                            break; }
                        default:
                            rvm->registers[instr.dst] = NIL_VAL;
                    }
                }
                break;
            }
            case ROP_NEG_I32:
                rvm->registers[instr.dst] = I32_VAL(-AS_I32(rvm->registers[instr.src1]));
                break;
            case ROP_NEG_U32:
                rvm->registers[instr.dst] = U32_VAL(-AS_U32(rvm->registers[instr.src1]));
                break;
            case ROP_NEG_U64:
                rvm->registers[instr.dst] = U64_VAL(-AS_U64(rvm->registers[instr.src1]));
                break;
            case ROP_AND: {
                bool a = AS_BOOL(rvm->registers[instr.src1]);
                bool b = AS_BOOL(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a && b);
                break;
            }
            case ROP_OR: {
                bool a = AS_BOOL(rvm->registers[instr.src1]);
                bool b = AS_BOOL(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a || b);
                break;
            }
            case ROP_NOT:
                rvm->registers[instr.dst] = BOOL_VAL(!AS_BOOL(rvm->registers[instr.src1]));
                break;
            case ROP_BIT_AND_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a & b);
                break;
            }
            case ROP_BIT_AND_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a & b);
                break;
            }
            case ROP_BIT_OR_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a | b);
                break;
            }
            case ROP_BIT_OR_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a | b);
                break;
            }
            case ROP_BIT_XOR_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a ^ b);
                break;
            }
            case ROP_BIT_XOR_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a ^ b);
                break;
            }
            case ROP_BIT_NOT_I32:
                rvm->registers[instr.dst] = I32_VAL(~AS_I32(rvm->registers[instr.src1]));
                break;
            case ROP_BIT_NOT_U32:
                rvm->registers[instr.dst] = U32_VAL(~AS_U32(rvm->registers[instr.src1]));
                break;
            case ROP_SHIFT_LEFT_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a << b);
                break;
            }
            case ROP_SHIFT_RIGHT_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a >> b);
                break;
            }
            case ROP_SHIFT_LEFT_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a << b);
                break;
            }
            case ROP_SHIFT_RIGHT_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a >> b);
                break;
            }
            case ROP_I32_TO_BOOL:
                rvm->registers[instr.dst] = BOOL_VAL(AS_I32(rvm->registers[instr.src1]) != 0);
                break;
            case ROP_U32_TO_BOOL:
                rvm->registers[instr.dst] = BOOL_VAL(AS_U32(rvm->registers[instr.src1]) != 0);
                break;
            case ROP_BOOL_TO_I32:
                rvm->registers[instr.dst] = I32_VAL(AS_BOOL(rvm->registers[instr.src1]) ? 1 : 0);
                break;
            case ROP_BOOL_TO_U32:
                rvm->registers[instr.dst] = U32_VAL(AS_BOOL(rvm->registers[instr.src1]) ? 1u : 0u);
                break;
            case ROP_BOOL_TO_F64:
                rvm->registers[instr.dst] = F64_VAL(AS_BOOL(rvm->registers[instr.src1]) ? 1.0 : 0.0);
                break;
            case ROP_F64_TO_BOOL:
                rvm->registers[instr.dst] = BOOL_VAL(AS_F64(rvm->registers[instr.src1]) != 0.0);
                break;
            case ROP_I32_TO_F64:
                rvm->registers[instr.dst] = F64_VAL((double)AS_I32(rvm->registers[instr.src1]));
                break;
            case ROP_U32_TO_F64:
                rvm->registers[instr.dst] = F64_VAL((double)AS_U32(rvm->registers[instr.src1]));
                break;
            case ROP_I32_TO_U32:
                rvm->registers[instr.dst] = U32_VAL((uint32_t)AS_I32(rvm->registers[instr.src1]));
                break;
            case ROP_U32_TO_I32:
                rvm->registers[instr.dst] = I32_VAL((int32_t)AS_U32(rvm->registers[instr.src1]));
                break;
            case ROP_I32_TO_I64:
                rvm->registers[instr.dst] = I64_VAL((int64_t)AS_I32(rvm->registers[instr.src1]));
                rvm->i64_regs[instr.dst] = AS_I32(rvm->registers[instr.src1]);
                break;
            case ROP_U32_TO_I64:
                rvm->registers[instr.dst] = I64_VAL((int64_t)AS_U32(rvm->registers[instr.src1]));
                rvm->i64_regs[instr.dst] = (int64_t)AS_U32(rvm->registers[instr.src1]);
                break;
            case ROP_I64_TO_I32:
                rvm->registers[instr.dst] = I32_VAL((int32_t)AS_I64(rvm->registers[instr.src1]));
                break;
            case ROP_I64_TO_U32:
                rvm->registers[instr.dst] = U32_VAL((uint32_t)AS_I64(rvm->registers[instr.src1]));
                break;
            case ROP_I32_TO_U64:
                rvm->registers[instr.dst] = U64_VAL((uint64_t)AS_I32(rvm->registers[instr.src1]));
                break;
            case ROP_U32_TO_U64:
                rvm->registers[instr.dst] = U64_VAL((uint64_t)AS_U32(rvm->registers[instr.src1]));
                break;
            case ROP_U64_TO_I32:
                rvm->registers[instr.dst] = I32_VAL((int32_t)AS_U64(rvm->registers[instr.src1]));
                break;
            case ROP_U64_TO_U32:
                rvm->registers[instr.dst] = U32_VAL((uint32_t)AS_U64(rvm->registers[instr.src1]));
                break;
            case ROP_U64_TO_F64:
                rvm->registers[instr.dst] = F64_VAL((double)AS_U64(rvm->registers[instr.src1]));
                break;
            case ROP_F64_TO_U64:
                rvm->registers[instr.dst] = U64_VAL((uint64_t)AS_F64(rvm->registers[instr.src1]));
                break;
            case ROP_F64_TO_I32:
                rvm->registers[instr.dst] = I32_VAL((int32_t)AS_F64(rvm->registers[instr.src1]));
                break;
            case ROP_F64_TO_U32:
                rvm->registers[instr.dst] = U32_VAL((uint32_t)AS_F64(rvm->registers[instr.src1]));
                break;
            case ROP_I64_TO_F64:
                rvm->registers[instr.dst] = F64_VAL((double)AS_I64(rvm->registers[instr.src1]));
                break;
            case ROP_F64_TO_I64:
                rvm->registers[instr.dst] = I64_VAL((int64_t)AS_F64(rvm->registers[instr.src1]));
                break;
            case ROP_I32_TO_STRING:
                rvm->registers[instr.dst] = convertToString(rvm->registers[instr.src1]);
                break;
            case ROP_U32_TO_STRING:
                rvm->registers[instr.dst] = convertToString(rvm->registers[instr.src1]);
                break;
            case ROP_F64_TO_STRING:
                rvm->registers[instr.dst] = convertToString(rvm->registers[instr.src1]);
                break;
            case ROP_BOOL_TO_STRING:
                rvm->registers[instr.dst] = convertToString(rvm->registers[instr.src1]);
                break;
            case ROP_ARRAY_TO_STRING:
                rvm->registers[instr.dst] = convertToString(rvm->registers[instr.src1]);
                break;
            case ROP_NIL:
                rvm->registers[instr.dst] = NIL_VAL;
                break;
            case ROP_POP_EXCEPT:
                if (vm.tryFrameCount > 0) vm.tryFrameCount--;
                break;
            case ROP_PRINT_BOOL:
                printf("%s\n", AS_BOOL(rvm->registers[instr.src1]) ? "true" : "false");
                break;
            case ROP_PRINT_BOOL_NO_NL:
                printf("%s", AS_BOOL(rvm->registers[instr.src1]) ? "true" : "false");
                fflush(stdout);
                break;
            case ROP_PRINT_F64:
                printf("%g\n", AS_F64(rvm->registers[instr.src1]));
                break;
            case ROP_PRINT_F64_NO_NL:
                printf("%g", AS_F64(rvm->registers[instr.src1]));
                fflush(stdout);
                break;
            case ROP_PRINT_I32:
                printf("%d\n", AS_I32(rvm->registers[instr.src1]));
                break;
            case ROP_PRINT_I32_NO_NL:
                printf("%d", AS_I32(rvm->registers[instr.src1]));
                fflush(stdout);
                break;
            case ROP_PRINT_I64:
                printf("%lld\n", (long long)AS_I64(rvm->registers[instr.src1]));
                break;
            case ROP_PRINT_I64_NO_NL:
                printf("%lld", (long long)AS_I64(rvm->registers[instr.src1]));
                fflush(stdout);
                break;
            case ROP_PRINT_STRING:
                printf("%s\n", AS_STRING(rvm->registers[instr.src1])->chars);
                break;
            case ROP_PRINT_STRING_NO_NL:
                printf("%s", AS_STRING(rvm->registers[instr.src1])->chars);
                fflush(stdout);
                break;
            case ROP_PRINT_U32:
                printf("%u\n", AS_U32(rvm->registers[instr.src1]));
                break;
            case ROP_PRINT_U32_NO_NL:
                printf("%u", AS_U32(rvm->registers[instr.src1]));
                fflush(stdout);
                break;
            case ROP_PRINT_U64:
                printf("%llu\n", (unsigned long long)AS_U64(rvm->registers[instr.src1]));
                break;
            case ROP_PRINT_U64_NO_NL:
                printf("%llu", (unsigned long long)AS_U64(rvm->registers[instr.src1]));
                fflush(stdout);
                break;
            case ROP_SETUP_EXCEPT:
                if (vm.tryFrameCount < TRY_MAX) {
                    vm.tryFrames[vm.tryFrameCount].handler =
                        (uint8_t*)(rvm->chunk->code + instr.dst);
                    vm.tryFrames[vm.tryFrameCount].varIndex = instr.src1;
                    vm.tryFrames[vm.tryFrameCount].stackDepth =
                        (int)(vm.stackTop - vm.stack);
                    vm.tryFrameCount++;
                } else {
                    vmRuntimeError("Too many nested try blocks.");
                }
                break;
            case ROP_SET_GLOBAL:
                vm.globals[instr.dst] = rvm->registers[instr.src1];
                break;
            case ROP_SHIFT_LEFT_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a << b);
                break; }
            case ROP_SHIFT_RIGHT_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a >> b);
                break; }
            case ROP_SLICE: {
                // ROP_SLICE: dst = slice(src1, src2, reg255)
                // where src1=array, src2=start, reg255=end (stored by previous MOV)
                Value arrayVal = rvm->registers[instr.src1]; // Array register
                Value startVal = rvm->registers[instr.src2]; // Start index
                Value endVal = rvm->registers[250];          // End index from temp register

                if (!IS_ARRAY(arrayVal) ||
                    !(IS_I32(startVal) || IS_U32(startVal) || IS_NIL(startVal)) ||
                    !(IS_I32(endVal) || IS_U32(endVal) || IS_NIL(endVal))) {
                    vmRuntimeError("slice expects (array, i32?, i32?).");
                    goto check_error;
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
                
                // Ensure the result is properly tagged as an array
                Value resultValue = ARRAY_VAL(result);
                if (!IS_ARRAY(resultValue)) {
                    vmRuntimeError("Failed to create array result in slice operation.");
                    goto check_error;
                }
                
                rvm->registers[instr.dst] = resultValue;
                break;
            }
            case ROP_SUBSTRING:
                break;
            case ROP_SUBTRACT_F64: {
                double a = AS_F64(rvm->registers[instr.src1]);
                double b = AS_F64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = F64_VAL(a - b);
                break; }
            case ROP_SUBTRACT_GENERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32: rvm->registers[instr.dst] = I32_VAL(AS_I32(a) - AS_I32(b)); break;
                        case VAL_I64: rvm->registers[instr.dst] = I64_VAL(AS_I64(a) - AS_I64(b)); break;
                        case VAL_U32: rvm->registers[instr.dst] = U32_VAL(AS_U32(a) - AS_U32(b)); break;
                        case VAL_U64: rvm->registers[instr.dst] = U64_VAL(AS_U64(a) - AS_U64(b)); break;
                        case VAL_F64: rvm->registers[instr.dst] = F64_VAL(AS_F64(a) - AS_F64(b)); break;
                        default: rvm->registers[instr.dst] = NIL_VAL; break;
                    }
                }
                break; }
            case ROP_SUBTRACT_I32: {
                int32_t a = AS_I32(rvm->registers[instr.src1]);
                int32_t b = AS_I32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I32_VAL(a - b);
                break; }
        case ROP_SUBTRACT_I64: {
                i64_regs[instr.dst] = i64_regs[instr.src1] - i64_regs[instr.src2];
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] i64_regs[R%d] = %lld\n", instr.dst, (long long)i64_regs[instr.dst]);
#endif
                break; }
            case ROP_SUBTRACT_NUMERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32: rvm->registers[instr.dst] = I32_VAL(AS_I32(a) - AS_I32(b)); break;
                        case VAL_I64: rvm->registers[instr.dst] = I64_VAL(AS_I64(a) - AS_I64(b)); break;
                        case VAL_U32: rvm->registers[instr.dst] = U32_VAL(AS_U32(a) - AS_U32(b)); break;
                        case VAL_U64: rvm->registers[instr.dst] = U64_VAL(AS_U64(a) - AS_U64(b)); break;
                        case VAL_F64: rvm->registers[instr.dst] = F64_VAL(AS_F64(a) - AS_F64(b)); break;
                        default: rvm->registers[instr.dst] = NIL_VAL; break;
                    }
                }
                break; }
            case ROP_SUBTRACT_U32: {
                uint32_t a = AS_U32(rvm->registers[instr.src1]);
                uint32_t b = AS_U32(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U32_VAL(a - b);
                break; }
            case ROP_SUBTRACT_U64: {
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = U64_VAL(a - b);
                break; }
            case ROP_U64_TO_BOOL:
                rvm->registers[instr.dst] = BOOL_VAL(AS_U64(rvm->registers[instr.src1]) != 0);
                break;
            case ROP_U64_TO_I64:
                rvm->registers[instr.dst] = I64_VAL((int64_t)AS_U64(rvm->registers[instr.src1]));
                break;
            case ROP_U64_TO_STRING:
                rvm->registers[instr.dst] = convertToString(rvm->registers[instr.src1]);
                break;
            case ROP_DEFINE_GLOBAL:
                vm.globals[instr.dst] = rvm->registers[instr.src1];
                break;
            case ROP_GET_GLOBAL:
                rvm->registers[instr.dst] = vm.globals[instr.src1];
                break;
            case ROP_I64_CONST:
                rvm->registers[instr.dst] = rvm->chunk->constants.values[instr.src1];
                break;
            case ROP_I64_TO_BOOL:
                rvm->registers[instr.dst] = BOOL_VAL(AS_I64(rvm->registers[instr.src1]) != 0);
                break;
            case ROP_I64_TO_U64:
                rvm->registers[instr.dst] = U64_VAL((uint64_t)AS_I64(rvm->registers[instr.src1]));
                break;
            case ROP_IMPORT:
                {
                    uint8_t constantIndex = instr.src1;
                    Value pathVal = rvm->chunk->constants.values[constantIndex];
                    if (!IS_STRING(pathVal)) {
                        vmRuntimeError("Import path must be a string.");
                        return NIL_VAL;
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
                        vmRuntimeError("Module already executed.");
                        return NIL_VAL;
                    }
                    InterpretResult modRes = interpret_module(pathStr->chars);
                    if (modRes != INTERPRET_OK) return NIL_VAL;
                    if (vm.moduleCount < UINT8_MAX)
                        vm.loadedModules[vm.moduleCount++] = pathStr;
                }
                break;
            case ROP_INC_I64:
                i64_regs[instr.dst] += 1;
#ifdef DEBUG_TRACE_EXECUTION
                printf("[Debug] i64_regs[R%d] = %lld\n", instr.dst, (long long)i64_regs[instr.dst]);
#endif
                break;
            case ROP_ITER_NEXT_I64:
                break;
            case ROP_JUMP_IF_LT_I64:
                if (i64_regs[instr.src1] < i64_regs[instr.src2])
                    rvm->ip = rvm->chunk->code + instr.dst;
                break;
            case ROP_LEN_ARRAY:
                rvm->registers[instr.dst] = I32_VAL(AS_ARRAY(rvm->registers[instr.src1])->length);
                break;
            case ROP_LEN_STRING:
                rvm->registers[instr.dst] = I32_VAL(AS_STRING(rvm->registers[instr.src1])->length);
                break;
            case ROP_RANGE:
                rvm->registers[instr.dst] = builtin_range(rvm->registers[instr.src1], rvm->registers[instr.src2]);
                break;
            case ROP_SUM:
                rvm->registers[instr.dst] = builtin_sum(rvm->registers[instr.src1]);
                break;
            case ROP_MIN:
                rvm->registers[instr.dst] = builtin_min(rvm->registers[instr.src1]);
                break;
            case ROP_MAX:
                rvm->registers[instr.dst] = builtin_max(rvm->registers[instr.src1]);
                break;
            case ROP_IS_TYPE:
                rvm->registers[instr.dst] = builtin_is_type(rvm->registers[instr.src1], rvm->registers[instr.src2]);
                break;
            case ROP_INPUT:
                rvm->registers[instr.dst] = builtin_input(rvm->registers[instr.src1]);
                break;
            case ROP_INT:
                rvm->registers[instr.dst] = builtin_int(rvm->registers[instr.src1]);
                break;
            case ROP_FLOAT:
                rvm->registers[instr.dst] = builtin_float(rvm->registers[instr.src1]);
                if (IS_F64(rvm->registers[instr.dst])) rvm->f64_regs[instr.dst] = AS_F64(rvm->registers[instr.dst]);
                break;
            case ROP_TIMESTAMP:
                rvm->registers[instr.dst] = builtin_timestamp();
                if (IS_F64(rvm->registers[instr.dst])) rvm->f64_regs[instr.dst] = AS_F64(rvm->registers[instr.dst]);
                break;
            case ROP_SORTED:
                rvm->registers[instr.dst] = builtin_sorted(rvm->registers[instr.src1], NIL_VAL, rvm->registers[instr.src2]);
                break;
            case ROP_MODULE_NAME:
                rvm->registers[instr.dst] = builtin_module_name(rvm->registers[instr.src1]);
                break;
            case ROP_MODULE_PATH:
                rvm->registers[instr.dst] = builtin_module_path(rvm->registers[instr.src1]);
                break;
            case ROP_NATIVE_POW:
                rvm->registers[instr.dst] = builtin_native_pow(rvm->registers[instr.src1], rvm->registers[instr.src2]);
                if (IS_F64(rvm->registers[instr.dst])) rvm->f64_regs[instr.dst] = AS_F64(rvm->registers[instr.dst]);
                break;
            case ROP_NATIVE_SQRT:
                rvm->registers[instr.dst] = builtin_native_sqrt(rvm->registers[instr.src1]);
                if (IS_F64(rvm->registers[instr.dst])) rvm->f64_regs[instr.dst] = AS_F64(rvm->registers[instr.dst]);
                break;
            case ROP_CALL_BUILTIN_SLICE: {
                // Call builtin slice function: slice(array, start, end)
                // Arguments are in registers 0, 1, 2
                // src1 contains the argument count (should be 3)
                Value arrayVal = rvm->registers[0];
                Value startVal = rvm->registers[1];
                Value endVal = rvm->registers[2];
                
                if (!IS_ARRAY(arrayVal) ||
                    !(IS_I32(startVal) || IS_U32(startVal) || IS_NIL(startVal)) ||
                    !(IS_I32(endVal) || IS_U32(endVal) || IS_NIL(endVal))) {
                    vmRuntimeError("slice expects (array, i32?, i32?).");
                    goto check_error;
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
                
                rvm->registers[instr.dst] = ARRAY_VAL(result);
                break;
            }
            case ROP_SPILL_REG: {
                // Spill register to memory slot
                // dst = spill slot index, src1 = register to spill
                int spillSlot = instr.dst;
                int regToSpill = instr.src1;
                
                // For simplicity, use high registers as spill storage
                // In a real implementation, this would use dedicated memory
                int spillReg = 240 + spillSlot; // Use registers 240-255 as spill area
                if (spillReg < REGISTER_COUNT) {
                    rvm->registers[spillReg] = rvm->registers[regToSpill];
                }
                break;
            }
            case ROP_UNSPILL_REG: {
                // Restore register from memory slot
                // dst = register to restore, src1 = spill slot index
                int regToRestore = instr.dst;
                int spillSlot = instr.src1;
                
                int spillReg = 240 + spillSlot;
                if (spillReg < REGISTER_COUNT) {
                    rvm->registers[regToRestore] = rvm->registers[spillReg];
                }
                break;
            }
        }

#endif

HANDLE_RUNTIME_ERROR:
    goto check_error;
check_error:
    if (IS_ERROR(vm.lastError)) {
        if (vm.tryFrameCount > 0) {
            TryFrame frame = vm.tryFrames[--vm.tryFrameCount];
            vm.stackTop = vm.stack + frame.stackDepth;
            vm.globals[frame.varIndex] = vm.lastError;
            vm.lastError = NIL_VAL;
            rvm->ip = (RegisterInstr*)frame.handler;
#if defined(__GNUC__) || defined(__clang__)
            ip = rvm->ip;
            goto *dispatch[ip->opcode];
#else
            continue;
#endif
        } else {
            return NIL_VAL;
        }
    }
    return NIL_VAL;
}

/* --- Refactored register-based VM implementation --- */

static inline void setRegisterI64(RegisterState* state, uint8_t index, int64_t value) {
    state->registers[index] = I64_VAL(value);
    state->i64_cache[index] = value;
    state->i64_valid[index] = true;
    state->f64_valid[index] = false;
}

static inline int64_t getRegisterI64(RegisterState* state, uint8_t index) {
    if (!state->i64_valid[index]) {
        state->i64_cache[index] = AS_I64(state->registers[index]);
        state->i64_valid[index] = true;
    }
    return state->i64_cache[index];
}

static VMResult setVMError(VMErrorState* errorState, const char* message) {
    ObjError* err = allocateError(ERROR_RUNTIME, message, (SrcLocation){NULL, 0, 0});
    if (!err) return VM_OUT_OF_MEMORY;
    errorState->lastError = ERROR_VAL(err);
    return VM_RUNTIME_ERROR;
}

static bool handleVMError(RegisterVM2* machine, VMResult result) {
    if (machine->errorState.tryFrameCount == 0) return false;
    TryFrame frame = machine->errorState.tryFrames[--machine->errorState.tryFrameCount];
    machine->instructionPointer = (RegisterInstr*)frame.handler;
    machine->registerState.registers[frame.varIndex] = machine->errorState.lastError;
    machine->errorState.lastError = NIL_VAL;
    return true;
}

static VMResult handleAddI64(RegisterVM2* machine, RegisterInstr* instruction) {
    int64_t a = getRegisterI64(&machine->registerState, instruction->src1);
    int64_t b = getRegisterI64(&machine->registerState, instruction->src2);
    setRegisterI64(&machine->registerState, instruction->dst, a + b);
    return VM_OK;
}

static VMResult handleDivI64(RegisterVM2* machine, RegisterInstr* instruction) {
    int64_t a = getRegisterI64(&machine->registerState, instruction->src1);
    int64_t b = getRegisterI64(&machine->registerState, instruction->src2);
    if (b == 0) {
        return setVMError(&machine->errorState, "Division by zero");
    }
    setRegisterI64(&machine->registerState, instruction->dst, a / b);
    return VM_OK;
}

static int64_t normalizeArrayIndex(Value indexValue, int length) {
    int64_t index;
    switch (indexValue.type) {
        case VAL_I32: index = AS_I32(indexValue); break;
        case VAL_I64: index = AS_I64(indexValue); break;
        case VAL_U32: index = (int64_t)AS_U32(indexValue); break;
        case VAL_U64:
            if (AS_U64(indexValue) > INT64_MAX) return -1;
            index = (int64_t)AS_U64(indexValue);
            break;
        default:
            return -1;
    }
    if (index < 0) index += length;
    return (index >= 0 && index < length) ? index : -1;
}

static VMResult handleArrayGet(RegisterVM2* machine, RegisterInstr* instruction) {
    Value arrayValue = machine->registerState.registers[instruction->src1];
    Value indexValue = machine->registerState.registers[instruction->src2];
    if (!IS_ARRAY(arrayValue)) {
        return setVMError(&machine->errorState, "Expected array for indexing");
    }
    ObjArray* array = AS_ARRAY(arrayValue);
    int64_t index = normalizeArrayIndex(indexValue, array->length);
    if (index < 0) {
        return setVMError(&machine->errorState, "Array index out of bounds");
    }
    machine->registerState.registers[instruction->dst] = array->elements[index];
    machine->registerState.i64_valid[instruction->dst] = false;
    machine->registerState.f64_valid[instruction->dst] = false;
    return VM_OK;
}

static VMResult unhandledOpcode(RegisterVM2* machine, RegisterInstr* instruction) {
    (void)instruction;
    return setVMError(&machine->errorState, "Invalid or unimplemented opcode");
}

void initRegisterVM2(RegisterVM2* machine, RegisterChunk* chunk) {
    memset(machine, 0, sizeof(RegisterVM2));
    machine->chunk = chunk;
    machine->instructionPointer = chunk->code;
    machine->errorState.tryFrames = vm.tryFrames;
    machine->errorState.tryFrameCount = 0;
    machine->errorState.maxTryFrames = TRY_MAX;
}

VMResult runRegisterVM2(RegisterVM2* machine) {
    static InstructionHandler handlers[256];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 256; i++) handlers[i] = unhandledOpcode;
        handlers[ROP_ADD_I64] = handleAddI64;
        handlers[ROP_DIVIDE_I64] = handleDivI64;
        handlers[ROP_ARRAY_GET] = handleArrayGet;
        initialized = true;
    }

    while (true) {
        RegisterInstr* instruction = machine->instructionPointer++;

        VMResult result = handlers[instruction->opcode](machine, instruction);
        if (result != VM_OK) {
            if (handleVMError(machine, result)) {
                continue;
            }
            return result;
        }

        if (++machine->instructionCount % 10000 == 0) {
            if (!machine->garbageCollectionPaused) collectGarbage();
            if (machine->instructionCount > 100000000) {
                return setVMError(&machine->errorState, "Infinite loop detected");
            }
        }
    }
}

