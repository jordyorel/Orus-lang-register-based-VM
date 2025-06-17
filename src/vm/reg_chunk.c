#include "../../include/reg_chunk.h"
#include "../../include/memory.h"
#include <stdlib.h>

void initRegisterChunk(RegisterChunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    initValueArray(&chunk->constants);
}

void freeRegisterChunk(RegisterChunk* chunk) {
    FREE_ARRAY(RegisterInstr, chunk->code, chunk->capacity);
    freeValueArray(&chunk->constants);
    initRegisterChunk(chunk);
}

static void grow(RegisterChunk* chunk, int minCapacity) {
    int old = chunk->capacity;
    int newCap = old < 8 ? 8 : old * 2;
    if (newCap < minCapacity) newCap = minCapacity;
    chunk->code = GROW_ARRAY(RegisterInstr, chunk->code, old, newCap);
    chunk->capacity = newCap;
}

void writeRegisterInstr(RegisterChunk* chunk, RegisterInstr instr) {
    if (chunk->count >= chunk->capacity) {
        grow(chunk, chunk->count + 1);
    }
    chunk->code[chunk->count++] = instr;
}

int addRegisterConstant(RegisterChunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}
