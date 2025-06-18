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
    // Newly supported opcodes
    ROP_ADD_I32,
    ROP_SUB_I32,
    ROP_MUL_I32,
    ROP_DIV_I32,
    ROP_ADD_U32,
    ROP_SUB_U32,
    ROP_MUL_U32,
    ROP_DIV_U32,
    ROP_ADD_U64,
    ROP_SUB_U64,
    ROP_MUL_U64,
    ROP_DIV_U64,
    ROP_NEG_I32,
    ROP_NEG_U32,
    ROP_NEG_U64,
    ROP_AND,
    ROP_OR,
    ROP_NOT,
    ROP_BIT_AND_I32,
    ROP_BIT_AND_U32,
    ROP_BIT_OR_I32,
    ROP_BIT_OR_U32,
    ROP_BIT_XOR_I32,
    ROP_BIT_XOR_U32,
    ROP_BIT_NOT_I32,
    ROP_BIT_NOT_U32,
    ROP_SHIFT_LEFT_I32,
    ROP_SHIFT_RIGHT_I32,
    ROP_SHIFT_LEFT_U32,
    ROP_SHIFT_RIGHT_U32,
    ROP_I32_TO_BOOL,
    ROP_U32_TO_BOOL,
    ROP_BOOL_TO_I32,
    ROP_BOOL_TO_U32,
    ROP_BOOL_TO_F64,
    ROP_F64_TO_BOOL,
    ROP_I32_TO_F64,
    ROP_U32_TO_F64,
    ROP_I32_TO_U32,
    ROP_U32_TO_I32,
    ROP_I32_TO_I64,
    ROP_U32_TO_I64,
    ROP_I64_TO_I32,
    ROP_I64_TO_U32,
    ROP_I32_TO_U64,
    ROP_U32_TO_U64,
    ROP_U64_TO_I32,
    ROP_U64_TO_U32,
    ROP_U64_TO_F64,
    ROP_F64_TO_U64,
    ROP_F64_TO_I32,
    ROP_F64_TO_U32,
    ROP_I64_TO_F64,
    ROP_F64_TO_I64,
    ROP_I32_TO_STRING,
    ROP_U32_TO_STRING,
    ROP_F64_TO_STRING,
    ROP_BOOL_TO_STRING,
    ROP_ARRAY_TO_STRING,
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
