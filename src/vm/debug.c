/**
 * @file debug.c
 * @brief Bytecode disassembly helpers for both stack and register VMs
 * 
 * This file provides debugging and disassembly functionality for the Orus
 * language virtual machines, supporting both legacy stack-based and new
 * register-based architectures.
 * 
 * @author Orus Development Team  
 * @version 1.0.0
 * @date 2024
 */

#include <stdio.h>
#include <string.h>

#include "../../include/debug.h"
#include "../../include/value.h"
#include "../../include/register_opcodes.h"
#include "../../include/register_vm.h"

// =============================================================================
// STACK VM DISASSEMBLY (Legacy Support)
// =============================================================================

// Forward declarations for static functions
static int constantInstruction(const char* name, Chunk* chunk, int offset);
static int simpleInstruction(const char* name, int offset);
static int byteInstruction(const char* name, Chunk* chunk, int offset);
static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset);

/**
 * Print a human readable disassembly of a stack VM chunk.
 *
 * @param chunk Chunk to disassemble.
 * @param name  Label printed at the top of the output.
 */
void disassembleChunk(Chunk* chunk, const char* name) {
    if (!chunk || !name) {
        printf("== NULL CHUNK ==\n");
        return;
    }
    
    printf("== %s ==\n", name);

    for (int offset = 0; offset < len(chunk);) {
        offset = disassembleInstruction(chunk, offset);
    }
}

/**
 * Disassemble a single stack VM instruction.
 *
 * @param chunk  Chunk containing the instruction.
 * @param offset Offset into the chunk.
 * @return       Offset of the next instruction.
 */
int disassembleInstruction(Chunk* chunk, int offset) {
    if (!chunk || offset >= len(chunk)) {
        printf("ERROR: Invalid chunk or offset\n");
        return offset + 1;
    }
    
    printf("%04d ", offset);
    if (offset > 0 && get_line(chunk, offset) == get_line(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", get_line(chunk, offset));
    }

    uint8_t instruction = get_code(chunk, offset);
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constantInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_I64_CONST:
            return constantInstruction("OP_I64_CONST", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
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
        case OP_ADD_I64:
            return simpleInstruction("OP_ADD_I64", offset);
        case OP_SUBTRACT_I64:
            return simpleInstruction("OP_SUBTRACT_I64", offset);
        case OP_MULTIPLY_I64:
            return simpleInstruction("OP_MULTIPLY_I64", offset);
        case OP_DIVIDE_I64:
            return simpleInstruction("OP_DIVIDE_I64", offset);
        case OP_NEGATE_I64:
            return simpleInstruction("OP_NEGATE_I64", offset);
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
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

/**
 * Disassemble an instruction that has a constant operand.
 *
 * @param name   Opcode name.
 * @param chunk  Chunk containing the instruction.
 * @param offset Offset into the chunk.
 * @return       Offset of the next instruction.
 */
static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = get_code(chunk, offset + 1);
    printf("%-16s %4d '", name, constant);
    printValue(get_constant(chunk, constant));
    printf("'\n");
    return offset + 2;
}

/**
 * Disassemble a simple instruction (no operands).
 *
 * @param name   Opcode name.
 * @param offset Current offset.
 * @return       Offset of the next instruction.
 */
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

/**
 * Disassemble an instruction with a single byte operand.
 *
 * @param name   Opcode name.
 * @param chunk  Chunk containing the instruction.
 * @param offset Offset into the chunk.
 * @return       Offset of the next instruction.
 */
static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = get_code(chunk, offset + 1);
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

/**
 * Disassemble a jump instruction.
 *
 * @param name   Opcode name.
 * @param sign   Direction of jump (+1 for forward, -1 for backward).
 * @param chunk  Chunk containing the instruction.
 * @param offset Offset into the chunk.
 * @return       Offset of the next instruction.
 */
static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)((get_code(chunk, offset + 1) << 8) | get_code(chunk, offset + 2));
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

// =============================================================================
// REGISTER VM DISASSEMBLY
// =============================================================================

/**
 * Print a human readable disassembly of a register VM chunk.
 *
 * @param chunk Register chunk to disassemble.
 * @param name  Label printed at the top of the output.
 */
void disassembleRegisterChunk(RegisterChunk* chunk, const char* name) {
    if (!chunk || !name) {
        printf("== NULL REGISTER CHUNK ==\n");
        return;
    }
    
    printf("=== %s (Register VM) ===\n", name);
    printf("Instructions: %d\n", chunk->code_count);
    printf("Constants: %d\n", chunk->constant_count);
    printf("Globals: %d\n", chunk->global_count);
    printf("\n");

    for (uint32_t offset = 0; offset < chunk->code_count; offset++) {
        disassembleRegisterInstruction(chunk, offset);
    }
    
    // Print constants section
    if (chunk->constant_count > 0) {
        printf("\n=== Constants ===\n");
        for (uint32_t i = 0; i < chunk->constant_count; i++) {
            printf("  [%04d] ", i);
            printValue(chunk->constants[i]);
            printf("\n");
        }
    }
}

/**
 * Disassemble a single register VM instruction.
 *
 * @param chunk  Register chunk containing the instruction.
 * @param offset Instruction index (not byte offset).
 * @return       Next instruction index.
 */
int disassembleRegisterInstruction(RegisterChunk* chunk, int offset) {
    if (!chunk || offset < 0 || offset >= (int)chunk->code_count) {
        printf("ERROR: Invalid register chunk or offset\n");
        return offset + 1;
    }
    
    uint32_t instruction = chunk->code[offset];
    RegisterOpcode opcode = (RegisterOpcode)GET_OPCODE(instruction);
    uint16_t imm = GET_IMM(instruction);
    
    printf("%04d  ", offset);
    
    // Use the instruction metadata system for consistent disassembly
    char buffer[256];
    int len = disassemble_instruction(instruction, buffer, sizeof(buffer));
    if (len > 0) {
        printf("%-32s", buffer);
    } else {
        printf("%-32s", "UNKNOWN");
    }
    
    // Add raw instruction encoding for debugging
    printf(" ; [%08X]", instruction);
    
    // Add helpful annotations for certain instruction types
    switch (opcode) {
        case ROP_LOAD_CONST:
            if (imm < chunk->constant_count) {
                printf(" -> ");
                printValue(chunk->constants[imm]);
            }
            break;
            
        case ROP_LOAD_GLOBAL:
        case ROP_STORE_GLOBAL:
            if (imm < chunk->global_count) {
                printf(" (global[%d])", imm);
            }
            break;
            
        case ROP_JMP:
        case ROP_JZ:
        case ROP_JNZ:
        case ROP_CALL:
            printf(" (target: %04d)", imm);
            break;
            
        default:
            // No additional annotation needed
            break;
    }
    
    printf("\n");
    return offset + 1;
}