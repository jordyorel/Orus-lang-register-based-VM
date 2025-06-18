#include "../../include/reg_vm.h"
#include "../../include/vm.h"
#include "../../include/memory.h"
#include <string.h>

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
        &&op_EQ_I64,
        &&op_NE_I64,
        &&op_LT_I64,
        &&op_LE_I64,
        &&op_GT_I64,
        &&op_GE_I64,
        &&op_JUMP,
        &&op_JZ,
        &&op_CALL,
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
        }
    }
    return NIL_VAL;
#endif
}
