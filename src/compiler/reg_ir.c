#include "../../include/reg_ir.h"
#include "../../include/chunk.h"
#include "../../include/value.h"
#include <string.h>

void chunkToRegisterIR(Chunk* chunk, RegisterChunk* out) {
    initRegisterChunk(out);

    // Simple stack to register mapping
    int stackRegs[REGISTER_COUNT];
    int sp = 0;
    int nextReg = 0;

    for (int offset = 0; offset < chunk->count; ) {
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
            default:
                // Unsupported opcode -> NOP
                writeRegisterInstr(out, (RegisterInstr){ROP_NOP, 0, 0, 0});
                offset += 1;
                break;
        }
    }
}

