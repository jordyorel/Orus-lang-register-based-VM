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
}
