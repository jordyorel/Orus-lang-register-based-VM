/**
 * @file debug.c
 * @brief Bytecode disassembly helpers for the register VM
 *
 * This file provides debugging and disassembly functionality for the
 * register-based virtual machine used by the Orus language.
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