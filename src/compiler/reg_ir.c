#include "../../include/reg_ir.h"
#include "../../include/chunk.h"
#include "../../include/value.h"
#include "../../include/vm.h"
#include <string.h>
#include <stdlib.h>

extern VM vm;

void chunkToRegisterIR(Chunk* chunk, RegisterChunk* out) {
    initRegisterChunk(out);

    // Simple stack to register mapping
    int stackRegs[REGISTER_COUNT];
    int sp = 0;

    int currentFunc = -1;
    int funcMax[UINT8_COUNT];
    uint8_t firstParamGlobal[UINT8_COUNT]; // Track which global is the first param
    uint8_t paramGlobals[UINT8_COUNT][16]; // Track globals for each parameter (max 16 params)
    int paramCount[UINT8_COUNT]; // Number of parameters per function
    for (int i = 0; i < UINT8_COUNT; i++) {
        funcMax[i] = 0;
        firstParamGlobal[i] = UINT8_MAX; // Invalid global index
        paramCount[i] = 0;
        for (int j = 0; j < 16; j++) {
            paramGlobals[i][j] = UINT8_MAX;
        }
    }

    // Enhanced register allocator state
    int nextReg = 0;
    int freeRegs[REGISTER_COUNT];
    int freeCount = 0;
    bool regHasConst[REGISTER_COUNT];
    Value regConst[REGISTER_COUNT];
    
    // Register lifetime tracking
    int regLastUse[REGISTER_COUNT];  // Last instruction that used this register
    int regRefCount[REGISTER_COUNT]; // Reference count for each register
    bool regSpilled[REGISTER_COUNT]; // Whether register has been spilled
    int spillSlots[REGISTER_COUNT];  // Spill slot assignments
    int nextSpillSlot = 0;
    
    for (int i = 0; i < REGISTER_COUNT; i++) {
        regHasConst[i] = false;
        regConst[i] = NIL_VAL;
        regLastUse[i] = -1;
        regRefCount[i] = 0;
        regSpilled[i] = false;
        spillSlots[i] = -1;
    }

#define ALLOC_REG() ({ \
    int r; \
    /* First try to find a free register */ \
    if (freeCount > 0) { \
        r = freeRegs[--freeCount]; \
        /* Don't reuse register 0 if we've allocated higher registers (likely in a method) */ \
        if (r == 0 && nextReg > 1) { \
            /* Put register 0 back and use a new register instead */ \
            freeRegs[freeCount++] = r; \
            r = nextReg++; \
        } \
    } else if (nextReg < REGISTER_COUNT - 16) { /* Reserve last 16 for spill temps */ \
        r = nextReg++; \
    } else { \
        /* Need to spill - find the register with oldest last use */ \
        r = 1; /* Don't spill register 0 (often self parameter) */ \
        int oldestUse = regLastUse[1]; \
        for (int i = 2; i < nextReg; i++) { \
            if (regLastUse[i] < oldestUse && regRefCount[i] == 0) { \
                r = i; \
                oldestUse = regLastUse[i]; \
            } \
        } \
        /* Generate spill instruction */ \
        if (!regSpilled[r]) { \
            spillSlots[r] = nextSpillSlot++; \
            RegisterInstr spillInstr = {ROP_SPILL_REG, (uint8_t)spillSlots[r], (uint8_t)r, 0}; \
            writeRegisterInstr(out, spillInstr); \
            regSpilled[r] = true; \
        } \
    } \
    if (currentFunc >= 0 && r + 1 > funcMax[currentFunc]) funcMax[currentFunc] = r + 1; \
    regHasConst[r] = false; \
    regRefCount[r] = 1; \
    regLastUse[r] = out->count; \
    r; })
#define RELEASE_REG(r) do { \
    regHasConst[(r)] = false; \
    if (regRefCount[(r)] > 0) regRefCount[(r)]--; \
    if (regRefCount[(r)] == 0) { \
        if (regSpilled[(r)]) { \
            /* Generate unspill instruction */ \
            RegisterInstr unspillInstr = {ROP_UNSPILL_REG, (uint8_t)(r), (uint8_t)spillSlots[(r)], 0}; \
            writeRegisterInstr(out, unspillInstr); \
            regSpilled[(r)] = false; \
        } \
        if (freeCount < REGISTER_COUNT) freeRegs[freeCount++] = (r); \
    } \
} while (0)
#define ALLOC_CONTIG(n) ({ \
    int base = nextReg; \
    nextReg += (n); \
    if (currentFunc >= 0 && nextReg > funcMax[currentFunc]) funcMax[currentFunc] = nextReg; \
    for (int _i=0; _i<(n); _i++) regHasConst[base+_i] = false; \
    base; })

    // Mapping from bytecode offsets to register instruction indices
    int* offsetMap = (int*)malloc(sizeof(int) * (chunk->count + 1));
    // Patches for forward/backward jumps
    typedef struct { int instr; int target; } Patch;
    Patch patches[256];
    int patchCount = 0;

    for (int offset = 0; offset < chunk->count; ) {
        for (int fi = 0; fi < vm.functionCount; fi++) {
            if (vm.functions[fi].chunk == chunk && vm.functions[fi].start == offset) {
                currentFunc = fi;
                // If function has parameters, reserve registers 0..arity-1 for parameters
                if (vm.functions[fi].arity > 0) {
                    nextReg = vm.functions[fi].arity; // Start allocation after parameter registers
                    paramCount[fi] = vm.functions[fi].arity;
                } else {
                    nextReg = 0;
                }
                freeCount = 0;
                sp = 0;
                firstParamGlobal[fi] = UINT8_MAX;
                break;
            }
        }
        offsetMap[offset] = out->count;
        uint8_t op = chunk->code[offset];
        switch (op) {
            case OP_CONSTANT: {
                uint8_t constIndex = chunk->code[offset + 1];
                int ridx = addRegisterConstant(out, chunk->constants.values[constIndex]);
                int reg = ALLOC_REG();
                RegisterInstr instr = {ROP_CONSTANT, (uint8_t)reg, (uint8_t)ridx, 0};
                writeRegisterInstr(out, instr);
                regHasConst[reg] = true;
                regConst[reg] = chunk->constants.values[constIndex];
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
                regHasConst[reg] = true;
                regConst[reg] = chunk->constants.values[idx];
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
                regHasConst[reg] = true;
                regConst[reg] = v;
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
                RegisterInstr instr = {ROP_NEGATE_F64, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_MODULO_I32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MODULO_I32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MODULO_I64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MODULO_I64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MODULO_U32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MODULO_U32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MODULO_NUMERIC: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MODULO_NUMERIC, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
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
            
            /* Array operations - translate stack VM to register VM */
            case OP_MAKE_ARRAY: {
                uint8_t count = chunk->code[offset + 1];
                if (sp < count) { offset += 2; break; }
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ARRAY_NEW, (uint8_t)dst, 0, 0};
                writeRegisterInstr(out, instr);
                for (int i = count - 1; i >= 0; i--) {
                    int val = stackRegs[--sp];
                    int idxReg = ALLOC_REG();
                    int idxConst = addRegisterConstant(out, I32_VAL(i));
                    RegisterInstr loadIdx = {ROP_LOAD_CONST, (uint8_t)idxReg, (uint8_t)idxConst, 0};
                    writeRegisterInstr(out, loadIdx);
                    RegisterInstr push = {ROP_ARRAY_PUSH, (uint8_t)dst, (uint8_t)val, 0};
                    writeRegisterInstr(out, push);
                    RELEASE_REG(val);
                    RELEASE_REG(idxReg);
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
            
            /* 
             * NOTE: Struct operations (ROP_STRUCT_LITERAL, ROP_FIELD_GET, ROP_FIELD_SET) 
             * are not directly translated here because the stack VM uses array opcodes
             * for struct operations. The struct register opcodes are implemented in
             * reg_vm.c and will be available for future compiler optimizations that
             * can distinguish between struct and array operations at compile time.
             */
            case OP_LEN: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_LEN, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_LEN_ARRAY: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_ARRAY_LEN, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_LEN_STRING: {
                if (sp < 1) { offset++; break; }
                int reg = stackRegs[sp - 1];
                RegisterInstr instr = {ROP_LEN_STRING, (uint8_t)reg, (uint8_t)reg, 0};
                writeRegisterInstr(out, instr);
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
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_I32, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_I64: {
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_I64, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_U32: {
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_U32, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_U64: {
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_U64, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_F64: {
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_F64, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_BOOL: {
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_BOOL, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_STRING: {
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_STRING, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_TYPE_OF_ARRAY: {
                if (sp < 1) { offset += 1; break; }
                int src = stackRegs[--sp];  // Consume the argument
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_TYPE_OF_ARRAY, (uint8_t)dst, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
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
            case OP_LESS_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LESS_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_LESS_EQUAL_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LESS_EQUAL_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GREATER_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_EQUAL_F64: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GREATER_EQUAL_F64, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_LESS_I32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LESS_I32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_LESS_U32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LESS_U32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_LESS_EQUAL_I32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LESS_EQUAL_I32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_LESS_EQUAL_U32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_LESS_EQUAL_U32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_I32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GREATER_I32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_U32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GREATER_U32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_EQUAL_I32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GREATER_EQUAL_I32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_GREATER_EQUAL_U32: {
                if (sp < 2) { offset++; break; }
                int b = stackRegs[--sp];
                int a = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_GREATER_EQUAL_U32, (uint8_t)dst, (uint8_t)a, (uint8_t)b};
                writeRegisterInstr(out, instr);
                RELEASE_REG(a);
                RELEASE_REG(b);
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
                patches[patchCount++] = (Patch){out->count - 1, offset + 3 - off};
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
            case OP_PRINT_I32: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_I32, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_I32_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_I32_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_I64: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_I64, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_I64_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_I64_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_U32: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_U32, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_U32_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_U32_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_U64: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_U64, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_U64_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_U64_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_F64: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_F64, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_F64_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_F64_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_BOOL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_BOOL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_BOOL_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_BOOL_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_STRING: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_STRING, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_PRINT_STRING_NO_NL: {
                if (sp < 1) { offset++; break; }
                int src = stackRegs[--sp];
                RegisterInstr instr = {ROP_PRINT_STRING_NO_NL, 0, (uint8_t)src, 0};
                writeRegisterInstr(out, instr);
                RELEASE_REG(src);
                offset += 1;
                break;
            }
            case OP_FORMAT_PRINT:
            case OP_FORMAT_PRINT_NO_NL: {
                if (sp < 2) { offset++; break; }
                int countReg = stackRegs[--sp];
                int argCount = 0;
                if (regHasConst[countReg]) {
                    if (IS_I32(regConst[countReg])) {
                        argCount = AS_I32(regConst[countReg]);
                    } else if (IS_I64(regConst[countReg])) {
                        argCount = (int)AS_I64(regConst[countReg]);
                    } else if (IS_U32(regConst[countReg])) {
                        argCount = (int)AS_U32(regConst[countReg]);
                    } else if (IS_U64(regConst[countReg])) {
                        argCount = (int)AS_U64(regConst[countReg]);
                    }
                }
                int formatIndex = sp - argCount - 1;
                if (formatIndex < 0) { offset++; break; }
                int formatReg = stackRegs[formatIndex];

                // Allocate contiguous registers starting at a high offset to avoid conflicts
                int base = REGISTER_COUNT - 16 - argCount - 1; // Use high-numbered registers
                if (base < 0) base = ALLOC_CONTIG(argCount + 1);
                
                RegisterInstr mvf = {ROP_MOV, (uint8_t)base, (uint8_t)formatReg, 0};
                writeRegisterInstr(out, mvf);
                for (int i = 0; i < argCount; i++) {
                    int src = stackRegs[formatIndex + 1 + i];
                    RegisterInstr mv = {ROP_MOV, (uint8_t)(base + 1 + i), (uint8_t)src, 0};
                    writeRegisterInstr(out, mv);
                }

                RegisterOp opCode = (op == OP_FORMAT_PRINT) ? ROP_FORMAT_PRINT : ROP_FORMAT_PRINT_NO_NL;
                RegisterInstr instr = {opCode, (uint8_t)base, (uint8_t)argCount, 0};
                writeRegisterInstr(out, instr);
                
                RELEASE_REG(formatReg);
                for (int i = 0; i < argCount; i++) {
                    RELEASE_REG(stackRegs[formatIndex + 1 + i]);
                }
                RELEASE_REG(countReg);
                sp = formatIndex;
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
                int dst;
                
                // Check if this global corresponds to a function parameter
                if (currentFunc >= 0 && paramCount[currentFunc] > 0) {
                    // Check if we already know this global as a parameter
                    int paramIndex = -1;
                    for (int i = 0; i < paramCount[currentFunc]; i++) {
                        if (paramGlobals[currentFunc][i] == idx) {
                            paramIndex = i;
                            break;
                        }
                    }
                    
                    if (paramIndex >= 0) {
                        // This is a known parameter - use its reserved register
                        dst = paramIndex;
                        stackRegs[sp++] = dst;
                        offset += 2;
                        break;
                    } else {
                        // This might be a new parameter - find the next available parameter slot
                        int nextParamSlot = -1;
                        for (int i = 0; i < paramCount[currentFunc]; i++) {
                            if (paramGlobals[currentFunc][i] == UINT8_MAX) {
                                nextParamSlot = i;
                                break;
                            }
                        }
                        
                        if (nextParamSlot >= 0) {
                            // This is a new parameter - assign it to the next parameter register
                            paramGlobals[currentFunc][nextParamSlot] = idx;
                            dst = nextParamSlot;
                            stackRegs[sp++] = dst;
                            offset += 2;
                            break;
                        }
                    }
                }
                
                dst = ALLOC_REG();
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
                uint8_t argc = chunk->code[offset + 2];
                // Allocate contiguous registers for function arguments and return value
                int base = ALLOC_CONTIG(argc > 0 ? argc : 1);
                
                for (int i = 0; i < argc; i++) {
                    int src = stackRegs[sp - argc + i];
                    RegisterInstr mv = {ROP_MOV, (uint8_t)(base + i), (uint8_t)src, 0};
                    writeRegisterInstr(out, mv);
                }
                RegisterInstr instr = {ROP_CALL, (uint8_t)base, idx, argc};
                writeRegisterInstr(out, instr);
                sp = sp - argc + 1;
                stackRegs[sp - 1] = base;
                offset += 3;
                break;
            }
            case OP_CALL_NATIVE: {
                uint8_t idx = chunk->code[offset + 1];
                uint8_t argc = chunk->code[offset + 2];
                int base = ALLOC_CONTIG(argc > 0 ? argc : 1);
                for (int i = 0; i < argc; i++) {
                    int src = stackRegs[sp - argc + i];
                    RegisterInstr mv = {ROP_MOV, (uint8_t)(base + i), (uint8_t)src, 0};
                    writeRegisterInstr(out, mv);
                }
                RegisterInstr instr = {ROP_CALL_NATIVE, (uint8_t)base, idx, argc};
                writeRegisterInstr(out, instr);
                sp = sp - argc + 1;
                stackRegs[sp - 1] = base;
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
            case OP_SETUP_EXCEPT: {
                uint16_t off = (uint16_t)(chunk->code[offset + 1] << 8 |
                                        chunk->code[offset + 2]);
                uint8_t var = chunk->code[offset + 3];
                RegisterInstr instr = {ROP_SETUP_EXCEPT, 0, var, 0};
                writeRegisterInstr(out, instr);
                patches[patchCount++] = (Patch){out->count - 1,
                                                offset + 4 + off};
                offset += 4;
                break;
            }
            case OP_POP_EXCEPT: {
                RegisterInstr instr = {ROP_POP_EXCEPT, 0, 0, 0};
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
            case OP_SLICE: {
                // OP_SLICE: stack has [array, start, end] -> [result]
                // Use the original ROP_SLICE but store end value in temp register
                int endReg = stackRegs[--sp];     // pop end 
                int startReg = stackRegs[--sp];   // pop start
                int arrayReg = stackRegs[--sp];   // pop array
                int resultReg = ALLOC_REG();      // allocate result
                stackRegs[sp++] = resultReg;      // push result
                
                // Store end value in a known register (250) for ROP_SLICE to access
                RegisterInstr storeEnd = {ROP_MOV, 250, (uint8_t)endReg, 0};
                writeRegisterInstr(out, storeEnd);
                
                // Execute slice: result = slice(array, start, temp_reg_250)
                RegisterInstr sliceInstr = {ROP_SLICE, (uint8_t)resultReg, (uint8_t)arrayReg, (uint8_t)startReg};
                writeRegisterInstr(out, sliceInstr);
                
                RELEASE_REG(endReg);
                RELEASE_REG(startReg);
                RELEASE_REG(arrayReg);
                offset += 2; // Two instructions: MOV + SLICE
                break;
            }
            
            // Enum operations - Phase 2.1.2
            case OP_ENUM_LITERAL: {
                if (sp < 2) { offset += 1; break; }
                int typeNameReg = stackRegs[--sp]; // Type name string
                int variantReg = stackRegs[--sp];  // Variant index
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ENUM_LITERAL, (uint8_t)dst, (uint8_t)variantReg, (uint8_t)typeNameReg};
                writeRegisterInstr(out, instr);
                RELEASE_REG(typeNameReg);
                RELEASE_REG(variantReg);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_ENUM_VARIANT: {
                // For now, treat like enum literal - proper implementation needed
                if (sp < 2) { offset += 1; break; }
                int typeNameReg = stackRegs[--sp];
                int variantReg = stackRegs[--sp];
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ENUM_VARIANT, (uint8_t)dst, (uint8_t)variantReg, (uint8_t)typeNameReg};
                writeRegisterInstr(out, instr);
                RELEASE_REG(typeNameReg);
                RELEASE_REG(variantReg);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_ENUM_CHECK: {
                if (sp < 2) { offset += 1; break; }
                int variantReg = stackRegs[--sp];   // Variant index to check
                int enumReg = stackRegs[--sp];      // Enum value
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_ENUM_CHECK, (uint8_t)dst, (uint8_t)enumReg, (uint8_t)variantReg};
                writeRegisterInstr(out, instr);
                RELEASE_REG(variantReg);
                RELEASE_REG(enumReg);
                stackRegs[sp++] = dst;
                offset += 1;
                break;
            }
            case OP_MATCH_BEGIN: {
                // Pattern matching begin - placeholder
                RegisterInstr instr = {ROP_MATCH_BEGIN, 0, 0, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            case OP_MATCH_END: {
                // Pattern matching end - placeholder
                RegisterInstr instr = {ROP_MATCH_END, 0, 0, 0};
                writeRegisterInstr(out, instr);
                offset += 1;
                break;
            }
            
            // Generic operations - Phase 3.1.2
            case OP_CALL_GENERIC: {
                // Generic function call translation
                if (sp < 3) { offset += 1; break; }
                int argcReg = stackRegs[--sp];      // Argument count
                int funcIndexReg = stackRegs[--sp];  // Function index
                int baseReg = stackRegs[--sp];       // Base register for arguments
                RegisterInstr instr = {ROP_CALL_GENERIC, (uint8_t)baseReg, (uint8_t)funcIndexReg, (uint8_t)argcReg};
                writeRegisterInstr(out, instr);
                RELEASE_REG(argcReg);
                RELEASE_REG(funcIndexReg);
                RELEASE_REG(baseReg);
                // Generic call result goes to baseReg
                stackRegs[sp++] = baseReg;
                offset += 1;
                break;
            }
            
            // Module operations - Phase 4.1.1
            case OP_IMPORT: {
                if (sp < 2) { offset += 1; break; }
                int aliasReg = stackRegs[--sp];      // Import alias (optional)
                int moduleNameReg = stackRegs[--sp]; // Module name string
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_IMPORT, (uint8_t)dst, (uint8_t)moduleNameReg, (uint8_t)aliasReg};
                writeRegisterInstr(out, instr);
                RELEASE_REG(moduleNameReg);
                RELEASE_REG(aliasReg);
                stackRegs[sp++] = dst; // Module object result
                offset += 1;
                break;
            }
            
            case OP_MODULE_CALL: {
                if (sp < 2) { offset += 1; break; }
                int functionNameReg = stackRegs[--sp]; // Function name
                int moduleReg = stackRegs[--sp];       // Module object
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MODULE_CALL, (uint8_t)dst, (uint8_t)moduleReg, (uint8_t)functionNameReg};
                writeRegisterInstr(out, instr);
                RELEASE_REG(moduleReg);
                RELEASE_REG(functionNameReg);
                stackRegs[sp++] = dst; // Function call result base
                offset += 1;
                break;
            }
            
            case OP_MODULE_ACCESS: {
                if (sp < 2) { offset += 1; break; }
                int memberNameReg = stackRegs[--sp]; // Member name
                int moduleReg = stackRegs[--sp];     // Module object
                int dst = ALLOC_REG();
                RegisterInstr instr = {ROP_MODULE_ACCESS, (uint8_t)dst, (uint8_t)moduleReg, (uint8_t)memberNameReg};
                writeRegisterInstr(out, instr);
                RELEASE_REG(moduleReg);
                RELEASE_REG(memberNameReg);
                stackRegs[sp++] = dst; // Member access result
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

    out->functionCount = vm.functionCount;
    for (int i = 0; i < vm.functionCount && i < UINT8_COUNT; i++) {
        if (vm.functions[i].chunk == chunk) {
            int off = vm.functions[i].start;
            if (off >= 0 && off <= chunk->count) {
                out->functionOffsets[i] = offsetMap[off];
            } else {
                out->functionOffsets[i] = -1;
            }
            out->functionRegCount[i] = (uint8_t)(funcMax[i] & 0xFF);
        } else {
            out->functionOffsets[i] = -1;
            out->functionRegCount[i] = 0;
        }
    }

    free(offsetMap);
}

