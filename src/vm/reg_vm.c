#include "../../include/reg_vm.h"
#include "../../include/vm.h"
#include "../../include/memory.h"
#include "../../include/vm_ops.h"
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
