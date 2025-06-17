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
    int nextReg = 0;

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
            case OP_I64_CONST: {
                uint8_t constIndex = chunk->code[offset + 1];
                if (nextReg >= REGISTER_COUNT) return; // out of registers
                Value v = chunk->constants.values[constIndex];
                int ridx = addRegisterConstant(out, v);
                RegisterInstr instr = {ROP_LOAD_CONST, (uint8_t)nextReg, (uint8_t)ridx, 0};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = nextReg++;
                offset += 2;
                break;
            }
            case OP_ADD_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ADD_RR, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_SUBTRACT_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_SUB_RR, (uint8_t)src1, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_INC_I64: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                int constIndex = addRegisterConstant(out, I64_VAL(1));
                RegisterInstr load = {ROP_LOAD_CONST, (uint8_t)nextReg, (uint8_t)constIndex, 0};
                writeRegisterInstr(out, load);
                RegisterInstr add = {ROP_ADD_RR, (uint8_t)reg, (uint8_t)reg, (uint8_t)nextReg};
                writeRegisterInstr(out, add);
                nextReg++;
                offset += 1;
                break;
            }
            case OP_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                RegisterInstr instr = {ROP_EQ_I64, (uint8_t)nextReg, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = nextReg++;
                offset += 1;
                break;
            }
            case OP_NOT_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                RegisterInstr instr = {ROP_NE_I64, (uint8_t)nextReg, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = nextReg++;
                offset += 1;
                break;
            }
            case OP_LESS_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                RegisterInstr instr = {ROP_LT_I64, (uint8_t)nextReg, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = nextReg++;
                offset += 1;
                break;
            }
            case OP_LESS_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                RegisterInstr instr = {ROP_LE_I64, (uint8_t)nextReg, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = nextReg++;
                offset += 1;
                break;
            }
            case OP_GREATER_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                RegisterInstr instr = {ROP_GT_I64, (uint8_t)nextReg, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = nextReg++;
                offset += 1;
                break;
            }
            case OP_GREATER_EQUAL_I64: {
                if (sp < 2) { offset++; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                RegisterInstr instr = {ROP_GE_I64, (uint8_t)nextReg, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, instr);
                stackRegs[sp++] = nextReg++;
                offset += 1;
                break;
            }
            case OP_JUMP_IF_LT_I64: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 | chunk->code[offset + 2]);
                if (sp < 2) { offset += 3; break; }
                int src2 = stackRegs[--sp];
                int src1 = stackRegs[--sp];
                RegisterInstr cmp = {ROP_LT_I64, (uint8_t)nextReg, (uint8_t)src1, (uint8_t)src2};
                writeRegisterInstr(out, cmp);
                RegisterInstr jz = {ROP_JZ, 0, (uint8_t)nextReg, 0};
                writeRegisterInstr(out, jz);
                patches[patchCount++] = (Patch){out->count - 1, offset + 3 + off};
                nextReg++;
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
                RegisterInstr instr = {ROP_JZ, 0, (uint8_t)cond, 0};
                writeRegisterInstr(out, instr);
                patches[patchCount++] = (Patch){out->count - 1, offset + 3 + off};
                offset += 3;
                break;
            }
            case OP_LOOP: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 | chunk->code[offset + 2]);
                RegisterInstr instr = {ROP_JUMP, 0, 0, 0};
                writeRegisterInstr(out, instr);
                patches[patchCount++] = (Patch){out->count - 1, offset - off};
                offset += 3;
                break;
            }
            case OP_CALL: {
                uint8_t idx = chunk->code[offset + 1];
                RegisterInstr instr = {ROP_CALL, (uint8_t)idx, 0, chunk->code[offset + 2]};
                writeRegisterInstr(out, instr);
                offset += 3;
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

