#ifndef ORUS_REG_VM_H
#define ORUS_REG_VM_H

#include "reg_chunk.h"
#include "value.h"

typedef struct {
    RegisterChunk* chunk;
    RegisterInstr* ip;
    /* Primitive typed register banks */
    int64_t i64_regs[REGISTER_COUNT];
    double  f64_regs[REGISTER_COUNT];
    /* Dynamic Value registers for non-primitive types */
    Value   registers[REGISTER_COUNT];
} RegisterVM;

void initRegisterVM(RegisterVM* vm, RegisterChunk* chunk);
void freeRegisterVM(RegisterVM* vm);
Value runRegisterVM(RegisterVM* vm);

#endif // ORUS_REG_VM_H
