#ifndef ORUS_REG_CHUNK_H
#define ORUS_REG_CHUNK_H

#include "common.h"
#include "value.h"

#define REGISTER_COUNT 256

typedef enum {
    ROP_NOP,
    ROP_MOV,
    ROP_LOAD_CONST,
    ROP_ADD_RR,
    ROP_SUB_RR,
    ROP_EQ_I64,
    ROP_NE_I64,
    ROP_LT_I64,
    ROP_LE_I64,
    ROP_GT_I64,
    ROP_GE_I64,
    ROP_JUMP,
    ROP_JZ,
    ROP_CALL,
} RegisterOp;

typedef struct {
    uint8_t opcode; // RegisterOp
    uint8_t dst;
    uint8_t src1;
    uint8_t src2;
} RegisterInstr;

typedef struct {
    int count;
    int capacity;
    RegisterInstr* code;
    ValueArray constants;
} RegisterChunk;

void initRegisterChunk(RegisterChunk* chunk);
void freeRegisterChunk(RegisterChunk* chunk);
void writeRegisterInstr(RegisterChunk* chunk, RegisterInstr instr);
int addRegisterConstant(RegisterChunk* chunk, Value value);

#endif // ORUS_REG_CHUNK_H
