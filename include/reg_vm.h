#ifndef ORUS_REG_VM_H
#define ORUS_REG_VM_H

#include "reg_chunk.h"
#include "value.h"

typedef struct {
    RegisterChunk* chunk;
    RegisterInstr* ip;
    Value registers[REGISTER_COUNT];
} RegisterVM;

void initRegisterVM(RegisterVM* vm, RegisterChunk* chunk);
void freeRegisterVM(RegisterVM* vm);

#endif // ORUS_REG_VM_H
