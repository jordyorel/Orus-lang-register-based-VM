#include "../../include/reg_vm.h"
#include "../../include/vm.h"
#include "../../include/memory.h"
#include "../../include/vm_ops.h"
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
