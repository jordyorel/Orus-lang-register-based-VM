#include "../../include/reg_vm.h"
#include <string.h>

void initRegisterVM(RegisterVM* vm, RegisterChunk* chunk) {
    vm->chunk = chunk;
    vm->ip = chunk->code;
    memset(vm->registers, 0, sizeof(vm->registers));
}

void freeRegisterVM(RegisterVM* vm) {
    (void)vm;
}

Value runRegisterVM(RegisterVM* vm) {
#if defined(__GNUC__) || defined(__clang__)
    RegisterInstr* ip = vm->ip;
    Value* regs = vm->registers;

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

#define DISPATCH() goto *dispatch[ip->opcode]

    DISPATCH();

op_NOP:
    ip++;
    DISPATCH();

op_MOV:
    regs[ip->dst] = regs[ip->src1];
    ip++;
    DISPATCH();

op_LOAD_CONST:
    regs[ip->dst] = vm->chunk->constants.values[ip->src1];
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
    ip = vm->chunk->code + ip->dst;
    DISPATCH();

op_JZ:
    if ((IS_BOOL(regs[ip->src1]) && !AS_BOOL(regs[ip->src1])) ||
        (IS_I64(regs[ip->src1]) && AS_I64(regs[ip->src1]) == 0)) {
        ip = vm->chunk->code + ip->dst;
    } else {
        ip++;
    }
    DISPATCH();

op_CALL:
    vm->ip = ip + 1;
    return regs[ip->src1];

#else
    while (true) {
        RegisterInstr instr = *vm->ip++;
        switch (instr.opcode) {
            case ROP_NOP:
                break;
            case ROP_MOV:
                vm->registers[instr.dst] = vm->registers[instr.src1];
                break;
            case ROP_LOAD_CONST:
                vm->registers[instr.dst] = vm->chunk->constants.values[instr.src1];
                break;
            case ROP_ADD_RR: {
                Value a = vm->registers[instr.src1];
                Value b = vm->registers[instr.src2];
                vm->registers[instr.dst] = I64_VAL(AS_I64(a) + AS_I64(b));
                break;
            }
            case ROP_SUB_RR: {
                Value a = vm->registers[instr.src1];
                Value b = vm->registers[instr.src2];
                vm->registers[instr.dst] = I64_VAL(AS_I64(a) - AS_I64(b));
                break;
            }
            case ROP_EQ_I64: {
                int64_t a = AS_I64(vm->registers[instr.src1]);
                int64_t b = AS_I64(vm->registers[instr.src2]);
                vm->registers[instr.dst] = BOOL_VAL(a == b);
                break;
            }
            case ROP_NE_I64: {
                int64_t a = AS_I64(vm->registers[instr.src1]);
                int64_t b = AS_I64(vm->registers[instr.src2]);
                vm->registers[instr.dst] = BOOL_VAL(a != b);
                break;
            }
            case ROP_LT_I64: {
                int64_t a = AS_I64(vm->registers[instr.src1]);
                int64_t b = AS_I64(vm->registers[instr.src2]);
                vm->registers[instr.dst] = BOOL_VAL(a < b);
                break;
            }
            case ROP_LE_I64: {
                int64_t a = AS_I64(vm->registers[instr.src1]);
                int64_t b = AS_I64(vm->registers[instr.src2]);
                vm->registers[instr.dst] = BOOL_VAL(a <= b);
                break;
            }
            case ROP_GT_I64: {
                int64_t a = AS_I64(vm->registers[instr.src1]);
                int64_t b = AS_I64(vm->registers[instr.src2]);
                vm->registers[instr.dst] = BOOL_VAL(a > b);
                break;
            }
            case ROP_GE_I64: {
                int64_t a = AS_I64(vm->registers[instr.src1]);
                int64_t b = AS_I64(vm->registers[instr.src2]);
                vm->registers[instr.dst] = BOOL_VAL(a >= b);
                break;
            }
            case ROP_JUMP:
                vm->ip = vm->chunk->code + instr.dst;
                break;
            case ROP_JZ:
                if ((IS_BOOL(vm->registers[instr.src1]) && !AS_BOOL(vm->registers[instr.src1])) ||
                    (IS_I64(vm->registers[instr.src1]) && AS_I64(vm->registers[instr.src1]) == 0)) {
                    vm->ip = vm->chunk->code + instr.dst;
                }
                break;
            case ROP_CALL:
                /* Simple call to native stub for now */
                return vm->registers[instr.src1];
        }
    }
    return NIL_VAL;
#endif
}
