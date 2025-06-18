#include "../../include/reg_ir.h"
#include "../../include/chunk.h"
#include "../../include/value.h"
#include <string.h>
#include <stdlib.h>

void chunkToRegisterIR(Chunk* chunk, RegisterChunk* out) {
    initRegisterChunk(out);

    // Simple stack to register mapping
    int stackRegs[REGISTER_COUNT];
    int sp = 0;

    // Register allocator state
    int nextReg = 0;
    int freeRegs[REGISTER_COUNT];
    int freeCount = 0;

#define ALLOC_REG() (freeCount > 0 ? freeRegs[--freeCount] : nextReg++)
#define RELEASE_REG(r) do { if (freeCount < REGISTER_COUNT) freeRegs[freeCount++] = (r); } while (0)

    // Mapping from bytecode offsets to register instruction indices
    int* offsetMap = (int*)malloc(sizeof(int) * (chunk->count + 1));
    // Patches for forward/backward jumps
    typedef struct { int instr; int target; } Patch;
    Patch patches[256];
    int patchCount = 0;

    for (int offset = 0; offset < chunk->count; ) {
        offsetMap[offset] = out->count;
        uint8_t op = chunk->code[offset];
        switch (op) {
            case OP_CONSTANT: {
                uint8_t constIndex = chunk->code[offset + 1];
                int ridx = addRegisterConstant(out, chunk->constants.values[constIndex]);
                int reg = ALLOC_REG();
                RegisterInstr instr = {ROP_CONSTANT, (uint8_t)reg, (uint8_t)ridx, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = reg;
                offset += 2;
                break;
            }
            case OP_CONSTANT_LONG: {
                uint32_t idx = (uint32_t)(chunk->code[offset + 1] << 16) |
                                (uint32_t)(chunk->code[offset + 2] << 8) |
                                (uint32_t)(chunk->code[offset + 3]);
                int ridx = addRegisterConstant(out, chunk->constants.values[idx]);
                int reg = ALLOC_REG();
                RegisterInstr instr = {ROP_CONSTANT_LONG, (uint8_t)reg, (uint8_t)ridx, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = reg;
                offset += 4;
                break;
            }
            case OP_I64_CONST: {
                uint8_t constIndex = chunk->code[offset + 1];
                if (nextReg >= REGISTER_COUNT) return; // out of registers
                Value v = chunk->constants.values[constIndex];
                int ridx = addRegisterConstant(out, v);
                int reg = ALLOC_REG();
                RegisterInstr instr = {ROP_LOAD_CONST, (uint8_t)reg, (uint8_t)ridx, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = reg;
                offset += 2;
                break;
            }
            case OP_ADD_I64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ADD_I64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_SUBTRACT_I64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_SUBTRACT_I64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MULTIPLY_I64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MULTIPLY_I64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_DIVIDE_I64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_DIVIDE_I64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_NEGATE_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_NEGATE_I64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_ADD_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ADD_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_SUBTRACT_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SUB_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_MULTIPLY_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_MUL_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_DIVIDE_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_DIV_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_ADD_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ADD_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_SUBTRACT_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SUB_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_MULTIPLY_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_MUL_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_DIVIDE_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_DIV_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_ADD_U64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ADD_U64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_SUBTRACT_U64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SUB_U64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_MULTIPLY_U64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_MUL_U64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_DIVIDE_U64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_DIV_U64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_ADD_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ADD_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_SUBTRACT_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_SUB_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MULTIPLY_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MUL_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_DIVIDE_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_DIV_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_NEGATE_F64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                int constIndex = addRegisterConstant(out, F64_VAL(-1.0));
                int tmp = ALLOC_REG();
                RegisterInstr load = {ROP_LOAD_CONST, (uint8_t)tmp, (uint8_t)constIndex, 0};
                writeRegisterInstr(out, load);
                RegisterInstr mul = {ROP_MUL_F64, (uint8_t)reg, (uint8_t)reg, (uint8_t)tmp};
                writeRegisterInstr(out, mul);
                RELEASE_REG(tmp);
                offset += 1;
                break;
            }
            case OP_MODULO_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_MOD_I64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_ADD_GENERIC: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ADD_GENERIC, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_ADD_NUMERIC: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ADD_NUMERIC, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_SUBTRACT_GENERIC: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_SUBTRACT_GENERIC, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MULTIPLY_GENERIC: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MULTIPLY_GENERIC, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_DIVIDE_GENERIC: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_DIVIDE_GENERIC, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MODULO_GENERIC: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MODULO_GENERIC, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_NEGATE_GENERIC: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_NEGATE_GENERIC, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_NEGATE_I32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_NEG_I32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_NEGATE_U32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_NEG_U32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_NEGATE_U64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_NEG_U64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BIT_AND_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_AND_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_AND_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_AND_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_OR_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_OR_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_OR_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_OR_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_XOR_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_XOR_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_XOR_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_XOR_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_NOT_I32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_NOT_I32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BIT_NOT_U32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_NOT_U32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_SHIFT_LEFT_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SHIFT_LEFT_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_SHIFT_RIGHT_I32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SHIFT_RIGHT_I32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_SHIFT_LEFT_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SHIFT_LEFT_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_SHIFT_RIGHT_U32: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SHIFT_RIGHT_U32, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_AND_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_AND_I64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_OR_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_OR_I64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_XOR_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_XOR_I64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_BIT_NOT_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BIT_NOT_I64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_SHIFT_LEFT_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SHL_I64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_SHIFT_RIGHT_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SHR_I64, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src2);
                offset += 1;
                break;
            }
            case OP_I32_TO_BOOL: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I32_TO_BOOL, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U32_TO_BOOL: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U32_TO_BOOL, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I64_TO_BOOL: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I64_TO_BOOL, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U64_TO_BOOL: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U64_TO_BOOL, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BOOL_TO_I32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BOOL_TO_I32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BOOL_TO_U32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BOOL_TO_U32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BOOL_TO_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BOOL_TO_I64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BOOL_TO_U64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BOOL_TO_U64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BOOL_TO_F64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BOOL_TO_F64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_F64_TO_BOOL: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_F64_TO_BOOL, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I32_TO_F64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I32_TO_F64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U32_TO_F64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U32_TO_F64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I32_TO_U32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I32_TO_U32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U32_TO_I32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U32_TO_I32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I32_TO_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I32_TO_I64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U32_TO_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U32_TO_I64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I64_TO_I32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I64_TO_I32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I64_TO_U32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I64_TO_U32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I32_TO_U64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I32_TO_U64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U32_TO_U64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U32_TO_U64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U64_TO_I32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U64_TO_I32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U64_TO_U32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U64_TO_U32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U64_TO_F64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U64_TO_F64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_F64_TO_U64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_F64_TO_U64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_F64_TO_I32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_F64_TO_I32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_F64_TO_U32: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_F64_TO_U32, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I64_TO_U64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I64_TO_U64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U64_TO_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U64_TO_I64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I64_TO_F64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I64_TO_F64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_F64_TO_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_F64_TO_I64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I32_TO_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I32_TO_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U32_TO_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U32_TO_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_F64_TO_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_F64_TO_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_I64_TO_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_I64_TO_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_U64_TO_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_U64_TO_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_BOOL_TO_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_BOOL_TO_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_ARRAY_TO_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ARRAY_TO_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_MAKE_ARRAY: {
                uint8_t count = chunk->code[offset + 1];
                if (sp < count) { offset += 2; break; }
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MAKE_ARRAY, (uint8_t)dst, count, 0};
                writeRegisterInstr(out, instr);
                for (int i = count - 1; i >= 0; i--) {
                    int val = stackRegs[--sp];
                    RegisterInstr set = {ROP_ARRAY_SET, (uint8_t)dst, (uint8_t)i, (uint8_t)val};
                    writeRegisterInstr(out, set);
                    RELEASE_REG(val);
                }
                stackRegs[sp++] = dst;
                offset += 2;
                break;
            }
            case OP_ARRAY_GET: {
                if (sp < 2) { offset++; break; }
                int index = stackRegs[--sp];
                int array = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ARRAY_GET, (uint8_t)dst, (uint8_t)array, (uint8_t)index};
                writeRegisterInstr(out, instr);
                RELEASE_REG(array);
                RELEASE_REG(index);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_ARRAY_SET: {
                if (sp < 3) { offset++; break; }
                int value = stackRegs[--sp];
                int index = stackRegs[--sp];
                int array = stackRegs[--sp];
                RegisterInstr instr = {ROP_ARRAY_SET, (uint8_t)array, (uint8_t)index, (uint8_t)value};
                writeRegisterInstr(out, instr);
                RELEASE_REG(array);
                RELEASE_REG(index);
                stackRegs[sp++] = value;
                offset += 1;
                break;
            }
            case OP_ARRAY_PUSH: {
                if (sp < 2) { offset++; break; }
                int value = stackRegs[--sp];
                int array = stackRegs[--sp];
                RegisterInstr instr = {ROP_ARRAY_PUSH, (uint8_t)array, 0, (uint8_t)value};
                writeRegisterInstr(out, instr);
                RELEASE_REG(value);
                stackRegs[sp++] = array;
                offset += 1;
                break;
            }
            case OP_ARRAY_POP: {
                if (sp < 1) { offset++; break; }
                int array = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ARRAY_POP, (uint8_t)array, (uint8_t)array, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_LEN: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_LEN, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_ARRAY_RESERVE: {
                if (sp < 2) { offset++; break; }
                int cap = stackRegs[--sp];
                int array = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ARRAY_RESERVE, (uint8_t)array, (uint8_t)array, (uint8_t)cap};
                writeRegisterInstr(out, instr);
                RELEASE_REG(cap);
                offset += 1;
                break;
            }
            case OP_CONCAT: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_CONCAT, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_I32: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_I32, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_I64: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_I64, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_U32: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_U32, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_U64: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_U64, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_F64: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_F64, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_BOOL: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_BOOL, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_STRING: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_STRING, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_ARRAY: {
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_ARRAY, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GC_PAUSE: {
                RegisterInstr instr = {ROP_GC_PAUSE, 0, 0, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_GC_RESUME: {
                RegisterInstr instr = {ROP_GC_RESUME, 0, 0, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_AND: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_AND, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_OR: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_OR, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_NOT: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_NOT, (uint8_t)src, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_INC_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                int constIndex = addRegisterConstant(out, I64_VAL(1));
                int tmp = ALLOC_REG();
                RegisterInstr load = {ROP_LOAD_CONST, (uint8_t)tmp, (uint8_t)constIndex, 0};
                writeRegisterInstr(out, load);
                RegisterInstr add = {ROP_ADD_I64, (uint8_t)reg, (uint8_t)reg, (uint8_t)tmp};
                writeRegisterInstr(out, add);
                RELEASE_REG(tmp);
                offset += 1;
                break;
            }
            case OP_EQUAL: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_EQUAL, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_NOT_EQUAL: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_NOT_EQUAL, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_EQ_I64, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_NOT_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_NE_I64, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_LESS_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LT_I64, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_LESS_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LE_I64, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GT_I64, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GE_I64, (uint8_t)dst, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_JUMP_IF_LT_I64: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 | chunk->code[offset + 2]);
                if (sp < 2) { offset += 3; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                int tmp = ALLOC_REG();
                RegisterInstr cmp = {ROP_LT_I64, (uint8_t)tmp, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, cmp);
                RegisterInstr jz = {ROP_JZ, 0, (uint8_t)tmp, 0};
                writeRegisterInstr(out, jz);
                patches[patchCount++] = (Patch){out->count - 1, offset + 3 + off};
                RELEASE_REG(src1);
                RELEASE_REG(src2);
                RELEASE_REG(tmp);
                offset += 3;
                break;
            }
            case OP_JUMP: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 | chunk->code[offset + 2]);
                RegisterInstr instr = {ROP_JUMP, 0, 0, 0};
                writeRegisterInstr(out, instr);
                patches[patchCount++] = (Patch){out->count - 1, offset + 3 + off};
                offset += 3;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 | chunk->code[offset + 2]);
                int cond = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_JUMP_IF_FALSE, 0, (uint8_t)cond, 0};
                writeRegisterInstr(out, instr);
                patches[patchCount++] = (Patch){out->count - 1, offset + 3 + off};
                offset += 3;
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 | chunk->code[offset + 2]);
                int cond = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_JUMP_IF_TRUE, 0, (uint8_t)cond, 0};
                writeRegisterInstr(out, instr);
                patches[patchCount++] = (Patch){out->count - 1, offset + 3 + off};
                offset += 3;
                break;
            }
            case OP_LOOP: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 | chunk->code[offset + 2]);
                RegisterInstr instr = {ROP_LOOP, 0, 0, 0};
                writeRegisterInstr(out, instr);
                patches[patchCount++] = (Patch){out->count - 1, offset - off};
                offset += 3;
                break;
            }
            case OP_PRINT: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_I64:
            case OP_PRINT_F64:
            case OP_PRINT_U64:
            case OP_PRINT_BOOL:
            case OP_PRINT_STRING: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_I64_NO_NL:
            case OP_PRINT_F64_NO_NL:
            case OP_PRINT_U64_NO_NL:
            case OP_PRINT_BOOL_NO_NL:
            case OP_PRINT_STRING_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint8_t idx = chunk->code[offset + 1];
                if (sp < 1) { offset += 2; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_STORE_GLOBAL, idx, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 2;
                break;
            }
            case OP_GET_GLOBAL: {
                uint8_t idx = chunk->code[offset + 1];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LOAD_GLOBAL, (uint8_t)dst, idx, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = dst;
                offset += 2;
                break;
            }
            case OP_SET_GLOBAL: {
                uint8_t idx = chunk->code[offset + 1];
                if (sp < 1) { offset += 2; break; }
                int src = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_STORE_GLOBAL, idx, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                offset += 2;
                break;
            }
            case OP_CALL: {
                uint8_t idx = chunk->code[offset + 1];
                RegisterInstr instr = {ROP_CALL, (uint8_t)idx, 0, chunk->code[offset + 2]};
                writeRegisterInstr(out, instr);
                offset += 3;
                break;
            }
            case OP_CALL_NATIVE: {
                uint8_t idx = chunk->code[offset + 1];
                uint8_t argc = chunk->code[offset + 2];
                RegisterInstr instr = {ROP_CALL_NATIVE, (uint8_t)idx, 0, argc};
                writeRegisterInstr(out, instr);
                offset += 3;
                break;
            }
            case OP_BREAK: {
                RegisterInstr instr = {ROP_BREAK, 0, 0, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_CONTINUE: {
                RegisterInstr instr = {ROP_CONTINUE, 0, 0, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_POP: {
                if (sp < 1) { offset++; break; }
                int dst = stackRegs[--sp];
                RegisterInstr instr = {ROP_POP, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(dst);
                offset += 1;
                break;
            }
            case OP_RETURN: {
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_RETURN, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            default:
                // Unsupported opcode -> NOP
                writeRegisterInstr(out, (RegisterInstr){ROP_NOP, 0, 0, 0});
                offset += 1;
                break;
        }
    }

    offsetMap[chunk->count] = out->count;
    for (int i = 0; i < patchCount; i++) {
        int target = patches[i].target;
        if (target >= 0 && target <= chunk->count) {
            out->code[patches[i].instr].dst = (uint8_t)offsetMap[target];
        } else {
            out->code[patches[i].instr].dst = 0;
        }
    }

    free(offsetMap);
}

