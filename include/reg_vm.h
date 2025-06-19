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

/*
 * Call frame used by the register-based VM. Each frame owns a full
 * register file so the GC must treat the registers as roots when
 * scanning live objects. The frame also stores the return address and
 * previous chunk information required to resume execution.
 */
typedef struct {
    RegisterInstr* returnAddress; // Instruction after the call
    RegisterChunk* previousChunk; // Chunk to resume
    uint8_t        retReg;        // Register index for return value
    RegisterVM     vm;            // Saved register file
} RegisterFrame;

void initRegisterVM(RegisterVM* vm, RegisterChunk* chunk);
void freeRegisterVM(RegisterVM* vm);
Value runRegisterVM(RegisterVM* vm);

#endif // ORUS_REG_VM_H
