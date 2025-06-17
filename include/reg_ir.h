#ifndef ORUS_REG_IR_H
#define ORUS_REG_IR_H

#include "chunk.h"
#include "reg_chunk.h"

// Translate a stack-based bytecode Chunk to a RegisterChunk.
// Only a subset of opcodes are currently supported.
void chunkToRegisterIR(Chunk* chunk, RegisterChunk* out);

#endif // ORUS_REG_IR_H
