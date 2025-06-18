#include "../../include/reg_vm.h"
#include "../../include/vm.h"
#include "../../include/memory.h"
#include "../../include/vm_ops.h"
#include "../../include/vm_ops.h"
#include "../../include/value.h"
#include <string.h>
#include <stdio.h>

extern VM vm;

void initRegisterVM(RegisterVM* rvm, RegisterChunk* chunk) {
    rvm->chunk = chunk;
    rvm->ip = chunk->code;
    memset(rvm->registers, 0, sizeof(rvm->registers));
}

void freeRegisterVM(RegisterVM* rvm) {
    (void)rvm;
}

Value runRegisterVM(RegisterVM* rvm) {
#if defined(__GNUC__) || defined(__clang__)
    RegisterInstr* ip = rvm->ip;
    Value* regs = rvm->registers;

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
        &&op_ADD_GENERIC,
        &&op_ADD_I64,
        &&op_ADD_NUMERIC,
        &&op_BOOL_TO_I64,
        &&op_BOOL_TO_U64,
        &&op_BREAK,
        &&op_CALL_NATIVE,
        &&op_CONSTANT,
        &&op_CONSTANT_LONG,
        &&op_CONTINUE,
        &&op_EQUAL,
        &&op_NOT_EQUAL,
        &&op_JUMP_IF_FALSE,
        &&op_JUMP_IF_TRUE,
        &&op_LOOP,
        &&op_POP,
        &&op_RETURN,
    };

#define DISPATCH()                                             \
    do {                                                      \
        vm.instruction_count++;                               \
        if (vm.instruction_count % 10000 == 0 && !vm.gcPaused) \
            collectGarbage();                                 \
        goto *dispatch[ip->opcode];                           \
    } while (0)

    DISPATCH();

op_NOP:
    ip++;
    DISPATCH();

op_MOV:
    regs[ip->dst] = regs[ip->src1];
    ip++;
    DISPATCH();

op_LOAD_CONST:
    regs[ip->dst] = rvm->chunk->constants.values[ip->src1];
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
        regs[ip->dst] = NIL_VAL;
    } else {
        regs[ip->dst] = I64_VAL(AS_I64(a) / AS_I64(b));
    }
    ip++;
    DISPATCH();
}

op_EQ_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a == b);
    ip++;
    DISPATCH();
}

op_NE_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a != b);
    ip++;
    DISPATCH();
}

op_LT_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a < b);
    ip++;
    DISPATCH();
}

op_LE_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a <= b);
    ip++;
    DISPATCH();
}

op_GT_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a > b);
    ip++;
    DISPATCH();
}

op_GE_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = BOOL_VAL(a >= b);
    ip++;
    DISPATCH();
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
    ip++;
    DISPATCH();

op_STORE_GLOBAL:
    vm.globals[ip->dst] = regs[ip->src1];
    ip++;
    DISPATCH();

op_ADD_F64: {
    double a = AS_F64(regs[ip->src1]);
    double b = AS_F64(regs[ip->src2]);
    regs[ip->dst] = F64_VAL(a + b);
    ip++; DISPATCH();
}

op_SUB_F64: {
    double a = AS_F64(regs[ip->src1]);
    double b = AS_F64(regs[ip->src2]);
    regs[ip->dst] = F64_VAL(a - b);
    ip++; DISPATCH();
}

op_MUL_F64: {
    double a = AS_F64(regs[ip->src1]);
    double b = AS_F64(regs[ip->src2]);
    regs[ip->dst] = F64_VAL(a * b);
    ip++; DISPATCH();
}

op_DIV_F64: {
    double a = AS_F64(regs[ip->src1]);
    double b = AS_F64(regs[ip->src2]);
    regs[ip->dst] = F64_VAL(b == 0.0 ? 0.0 : a / b);
    ip++; DISPATCH();
}

op_MOD_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = b == 0 ? NIL_VAL : I64_VAL(a % b);
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
    ObjArray* arr = AS_ARRAY(regs[ip->src1]);
    int idx = (int)AS_I64(regs[ip->src2]);
    regs[ip->dst] = (idx >=0 && idx < arr->length) ? arr->elements[idx] : NIL_VAL;
    ip++; DISPATCH();
}

op_ARRAY_SET:
    AS_ARRAY(regs[ip->dst])->elements[ip->src1] = regs[ip->src2];
    ip++; DISPATCH();

op_ARRAY_PUSH:
    arrayPush(&vm, AS_ARRAY(regs[ip->dst]), regs[ip->src2]);
    ip++; DISPATCH();

op_ARRAY_POP:
    regs[ip->dst] = arrayPop(AS_ARRAY(regs[ip->dst]));
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
    rvm->ip = ip + 1;
    return regs[ip->src1];

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
    regs[ip->dst] = I32_VAL(b == 0 ? 0 : a / b);
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
    regs[ip->dst] = U32_VAL(b == 0 ? 0 : a / b);
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
    regs[ip->dst] = U64_VAL(b == 0 ? 0 : a / b);
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
    ip++;
    DISPATCH();

op_U32_TO_I64:
    regs[ip->dst] = I64_VAL((int64_t)AS_U32(regs[ip->src1]));
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
    ip++;
    DISPATCH();

op_F64_TO_I64:
    regs[ip->dst] = I64_VAL((int64_t)AS_F64(regs[ip->src1]));
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

op_ADD_GENERIC: {
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

op_ADD_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = I64_VAL(a + b);
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

op_SUBTRACT_GENERIC: {
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

op_MULTIPLY_GENERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32: regs[ip->dst] = I32_VAL(AS_I32(a) * AS_I32(b)); break;
            case VAL_I64: regs[ip->dst] = I64_VAL(AS_I64(a) * AS_I64(b)); break;
            case VAL_U32: regs[ip->dst] = U32_VAL(AS_U32(a) * AS_U32(b)); break;
            case VAL_U64: regs[ip->dst] = U64_VAL(AS_U64(a) * AS_U64(b)); break;
            case VAL_F64: regs[ip->dst] = F64_VAL(AS_F64(a) * AS_F64(b)); break;
            default: regs[ip->dst] = NIL_VAL; break;
        }
    }
    ip++; DISPATCH();
}

op_DIVIDE_GENERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32: {
                int32_t bv = AS_I32(b); regs[ip->dst] = bv == 0 ? NIL_VAL : I32_VAL(AS_I32(a) / bv); break; }
            case VAL_I64: {
                int64_t bv = AS_I64(b); regs[ip->dst] = bv == 0 ? NIL_VAL : I64_VAL(AS_I64(a) / bv); break; }
            case VAL_U32: {
                uint32_t bv = AS_U32(b); regs[ip->dst] = bv == 0 ? NIL_VAL : U32_VAL(AS_U32(a) / bv); break; }
            case VAL_U64: {
                uint64_t bv = AS_U64(b); regs[ip->dst] = bv == 0 ? NIL_VAL : U64_VAL(AS_U64(a) / bv); break; }
            case VAL_F64: {
                double bv = AS_F64(b); regs[ip->dst] = bv == 0.0 ? NIL_VAL : F64_VAL(AS_F64(a) / bv); break; }
            default: regs[ip->dst] = NIL_VAL; break;
        }
    }
    ip++; DISPATCH();
}

op_MODULO_GENERIC: {
    Value a = regs[ip->src1];
    Value b = regs[ip->src2];
    if (a.type != b.type) {
        regs[ip->dst] = NIL_VAL;
    } else {
        switch (a.type) {
            case VAL_I32: {
                int32_t bv = AS_I32(b); regs[ip->dst] = bv == 0 ? NIL_VAL : I32_VAL(AS_I32(a) % bv); break; }
            case VAL_I64: {
                int64_t bv = AS_I64(b); regs[ip->dst] = bv == 0 ? NIL_VAL : I64_VAL(AS_I64(a) % bv); break; }
            case VAL_U32: {
                uint32_t bv = AS_U32(b); regs[ip->dst] = bv == 0 ? NIL_VAL : U32_VAL(AS_U32(a) % bv); break; }
            case VAL_U64: {
                uint64_t bv = AS_U64(b); regs[ip->dst] = bv == 0 ? NIL_VAL : U64_VAL(AS_U64(a) % bv); break; }
            default: regs[ip->dst] = NIL_VAL; break;
        }
    }
    ip++; DISPATCH();
}

op_NEGATE_GENERIC: {
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

op_SUBTRACT_I64: {
    int64_t a = AS_I64(regs[ip->src1]);
    int64_t b = AS_I64(regs[ip->src2]);
    regs[ip->dst] = I64_VAL(a - b);
    ip++; DISPATCH();
}

op_NEGATE_I64:
    regs[ip->dst] = I64_VAL(-AS_I64(regs[ip->src1]));
    ip++; DISPATCH();

op_BOOL_TO_I64:
    regs[ip->dst] = I64_VAL(AS_BOOL(regs[ip->src1]) ? 1 : 0);
    ip++; DISPATCH();

op_BOOL_TO_U64:
    regs[ip->dst] = U64_VAL(AS_BOOL(regs[ip->src1]) ? 1u : 0u);
    ip++; DISPATCH();

op_BREAK:
    ip++; DISPATCH();

op_CALL_NATIVE:
    ip++; DISPATCH();

op_CONSTANT:
    regs[ip->dst] = rvm->chunk->constants.values[ip->src1];
    ip++; DISPATCH();

op_CONSTANT_LONG:
    regs[ip->dst] = rvm->chunk->constants.values[ip->src1];
    ip++; DISPATCH();

op_CONTINUE:
    ip++; DISPATCH();

op_EQUAL:
    regs[ip->dst] = BOOL_VAL(valuesEqual(regs[ip->src1], regs[ip->src2]));
    ip++; DISPATCH();

op_NOT_EQUAL:
    regs[ip->dst] = BOOL_VAL(!valuesEqual(regs[ip->src1], regs[ip->src2]));
    ip++; DISPATCH();

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
    rvm->ip = ip + 1;
    return regs[ip->src1];

#else
    while (true) {
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
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    rvm->registers[instr.dst] = I64_VAL(AS_I64(a) / AS_I64(b));
                }
                break;
            }
            case ROP_EQ_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a == b);
                break;
            }
            case ROP_NE_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a != b);
                break;
            }
            case ROP_EQUAL:
                rvm->registers[instr.dst] = BOOL_VAL(valuesEqual(rvm->registers[instr.src1],
                                                                rvm->registers[instr.src2]));
                break;
            case ROP_NOT_EQUAL:
                rvm->registers[instr.dst] = BOOL_VAL(!valuesEqual(rvm->registers[instr.src1],
                                                                 rvm->registers[instr.src2]));
                break;
            case ROP_LT_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a < b);
                break;
            }
            case ROP_LE_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a <= b);
                break;
            }
            case ROP_GT_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a > b);
                break;
            }
            case ROP_GE_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = BOOL_VAL(a >= b);
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
                rvm->registers[instr.dst] = F64_VAL(b == 0.0 ? 0.0 : a / b);
                break;
            }
            case ROP_MOD_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = b == 0 ? NIL_VAL : I64_VAL(a % b);
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
                ObjArray* arr = AS_ARRAY(rvm->registers[instr.src1]);
                int idx = (int)AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = (idx >=0 && idx < arr->length) ? arr->elements[idx] : NIL_VAL;
                break;
            }
            case ROP_ARRAY_SET:
                AS_ARRAY(rvm->registers[instr.dst])->elements[instr.src1] = rvm->registers[instr.src2];
                break;
            case ROP_ARRAY_PUSH:
                arrayPush(&vm, AS_ARRAY(rvm->registers[instr.dst]), rvm->registers[instr.src2]);
                break;
            case ROP_ARRAY_POP:
                rvm->registers[instr.dst] = arrayPop(AS_ARRAY(rvm->registers[instr.dst]));
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
            case ROP_ADD_GENERIC: {
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
            case ROP_ADD_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a + b);
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
            case ROP_SUBTRACT_GENERIC: {
                Value a = rvm->registers[instr.src1];
                Value b = rvm->registers[instr.src2];
                if (a.type != b.type) {
                    rvm->registers[instr.dst] = NIL_VAL;
                } else {
                    switch (a.type) {
                        case VAL_I32:
                            rvm->registers[instr.dst] = I32_VAL(AS_I32(a) - AS_I32(b));
                            break;
                        case VAL_I64:
                            rvm->registers[instr.dst] = I64_VAL(AS_I64(a) - AS_I64(b));
                            break;
                        case VAL_U32:
                            rvm->registers[instr.dst] = U32_VAL(AS_U32(a) - AS_U32(b));
                            break;
                        case VAL_U64:
                            rvm->registers[instr.dst] = U64_VAL(AS_U64(a) - AS_U64(b));
                            break;
                        case VAL_F64:
                            rvm->registers[instr.dst] = F64_VAL(AS_F64(a) - AS_F64(b));
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
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : I32_VAL(AS_I32(a) / bv);
                            break; }
                        case VAL_I64: {
                            int64_t bv = AS_I64(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : I64_VAL(AS_I64(a) / bv);
                            break; }
                        case VAL_U32: {
                            uint32_t bv = AS_U32(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : U32_VAL(AS_U32(a) / bv);
                            break; }
                        case VAL_U64: {
                            uint64_t bv = AS_U64(b);
                            rvm->registers[instr.dst] = bv == 0 ? NIL_VAL : U64_VAL(AS_U64(a) / bv);
                            break; }
                        case VAL_F64: {
                            double bv = AS_F64(b);
                            rvm->registers[instr.dst] = bv == 0.0 ? NIL_VAL : F64_VAL(AS_F64(a) / bv);
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
            case ROP_SUBTRACT_I64: {
                int64_t a = AS_I64(rvm->registers[instr.src1]);
                int64_t b = AS_I64(rvm->registers[instr.src2]);
                rvm->registers[instr.dst] = I64_VAL(a - b);
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
            case ROP_CALL_NATIVE:
                break;
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
            case ROP_RETURN:
                return rvm->registers[instr.src1];
            case ROP_JUMP:
                rvm->ip = rvm->chunk->code + instr.dst;
                break;
            case ROP_JZ:
                if ((IS_BOOL(rvm->registers[instr.src1]) && !AS_BOOL(rvm->registers[instr.src1])) ||
                    (IS_I64(rvm->registers[instr.src1]) && AS_I64(rvm->registers[instr.src1]) == 0)) {
                    rvm->ip = rvm->chunk->code + instr.dst;
                }
                break;
            case ROP_CALL:
                /* Simple call to native stub for now */
                return rvm->registers[instr.src1];
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
                rvm->registers[instr.dst] = I32_VAL(b == 0 ? 0 : a / b);
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
                rvm->registers[instr.dst] = U32_VAL(b == 0 ? 0 : a / b);
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
            case ROP_DIV_U64: {
                uint64_t b = AS_U64(rvm->registers[instr.src2]);
                uint64_t a = AS_U64(rvm->registers[instr.src1]);
                rvm->registers[instr.dst] = U64_VAL(b == 0 ? 0 : a / b);
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
                break;
            case ROP_U32_TO_I64:
                rvm->registers[instr.dst] = I64_VAL((int64_t)AS_U32(rvm->registers[instr.src1]));
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
        }
    }
    return NIL_VAL;
#endif
}
