#include <stdio.h>

#include "debug.h"
#include "value.h"

// Forward declarations for all static functions
static int constantInstruction(const char* name, Chunk* chunk, int offset);
static int simpleInstruction(const char* name, int offset);
static int byteInstruction(const char* name, Chunk* chunk, int offset);

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < len(chunk);) {
        offset = disassembleInstruction(chunk, offset);
    }
}

// Handles the disassembly of instructions that have a single operand (a
// constant). Parameters:
//   name - The name of the opcode to print (e.g., "OP_CONSTANT").
//   chunk - Pointer to the Chunk containing the bytecode instructions.
//   offset - The current index of the instruction.
// Returns:
//   The offset of the next instruction (current offset + 2).
static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = get_code(chunk, offset);
    printf("%-16s %4d '", name, constant);
    printValue(get_constant(chunk, offset));
    printf("'\n");
    return offset + 2;
}

// Disassembles a single instruction at a specific offset in the chunk.
// Parameters:
//   chunk - Pointer to the Chunk containing the bytecode instructions.
//   offset - The current index of the instruction to disassemble.
// Returns:
//   The offset of the next instruction after the one being disassembled.
int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    // Use get_line function instead of directly accessing lines
    int current_line = get_line(chunk, offset);
    int previous_line = (offset > 0) ? get_line(chunk, offset - 1) : -1;

    if (offset > 0 && current_line == previous_line) {
        printf("   | ");
    } else {
        printf("%4d ", current_line);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
            
        // Integer operations
        case OP_ADD_I32:
            return simpleInstruction("OP_ADD_I32", offset);
        case OP_SUBTRACT_I32:
            return simpleInstruction("OP_SUBTRACT_I32", offset);
        case OP_MULTIPLY_I32:
            return simpleInstruction("OP_MULTIPLY_I32", offset);
        case OP_DIVIDE_I32:
            return simpleInstruction("OP_DIVIDE_I32", offset);
        case OP_NEGATE_I32:
            return simpleInstruction("OP_NEGATE_I32", offset);
            
        // Unsigned integer operations
        case OP_ADD_U32:
            return simpleInstruction("OP_ADD_U32", offset);
        case OP_SUBTRACT_U32:
            return simpleInstruction("OP_SUBTRACT_U32", offset);
        case OP_MULTIPLY_U32:
            return simpleInstruction("OP_MULTIPLY_U32", offset);
        case OP_DIVIDE_U32:
            return simpleInstruction("OP_DIVIDE_U32", offset);
        case OP_NEGATE_U32:
            return simpleInstruction("OP_NEGATE_U32", offset);
            
        // Floating point operations
        case OP_ADD_F64:
            return simpleInstruction("OP_ADD_F64", offset);
        case OP_SUBTRACT_F64:
            return simpleInstruction("OP_SUBTRACT_F64", offset);
        case OP_MULTIPLY_F64:
            return simpleInstruction("OP_MULTIPLY_F64", offset);
        case OP_DIVIDE_F64:
            return simpleInstruction("OP_DIVIDE_F64", offset);
        case OP_NEGATE_F64:
            return simpleInstruction("OP_NEGATE_F64", offset);
        case OP_MODULO_I32:
            return simpleInstruction("OP_MODULO_I32", offset);
        case OP_MODULO_U32:
            return simpleInstruction("OP_MODULO_U32", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
            
        case OP_I32_TO_F64:
            return simpleInstruction("OP_I32_TO_F64", offset);
        case OP_U32_TO_F64:
            return simpleInstruction("OP_U32_TO_F64", offset);
            
        case OP_DEFINE_GLOBAL:
            return byteInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:
            return byteInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return byteInstruction("OP_SET_GLOBAL", chunk, offset);
            
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

// Handles the disassembly of simple 1-byte instructions (no operands).
// Parameters:
//   name - The name of the opcode to print (e.g., "OP_ADD").
//   offset - The current index of the instruction.
// Returns:
//   The offset of the next instruction (current offset + 1).
static int simpleInstruction(const char* name, int offset) {
    printf("%s \n", name);
    return offset + 1;
}

// Implementation of byteInstruction
static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;  // Skip the opcode and the byte operand
}
