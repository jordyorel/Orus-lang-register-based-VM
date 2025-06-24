/**
 * @file register_opcodes.c
 * @brief Orus Register VM Instruction Metadata Implementation
 * 
 * This file implements instruction metadata, validation, and disassembly
 * functions for the Orus register VM instruction set.
 * 
 * @author Orus Development Team
 * @version 1.0.0
 * @date 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../include/register_opcodes.h"
#include "../../include/register_vm.h"

// =============================================================================
// INSTRUCTION METADATA TABLE
// =============================================================================

/**
 * @brief Complete instruction metadata table
 * 
 * This table contains metadata for every instruction in the register VM.
 * It's used for validation, disassembly, and analysis.
 */
static const InstructionMetadata instruction_table[] = {
    // Control Flow Instructions
    { ROP_NOP,         "NOP",         "No operation",                    INST_CAT_CONTROL,    0, false, false, false },
    { ROP_HALT,        "HALT",        "Halt execution",                  INST_CAT_CONTROL,    0, true,  false, false },
    { ROP_JMP,         "JMP",         "Unconditional jump",              INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_JMP_REG,     "JMP_REG",     "Jump to register address",        INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_JZ,          "JZ",          "Jump if zero",                    INST_CAT_CONTROL,    2, true,  false, false },
    { ROP_JNZ,         "JNZ",         "Jump if not zero",                INST_CAT_CONTROL,    2, true,  false, false },
    { ROP_JEQ,         "JEQ",         "Jump if equal",                   INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_JNE,         "JNE",         "Jump if not equal",               INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_JLT,         "JLT",         "Jump if less than",               INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_JLE,         "JLE",         "Jump if less than or equal",      INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_JGT,         "JGT",         "Jump if greater than",            INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_JGE,         "JGE",         "Jump if greater than or equal",   INST_CAT_CONTROL,    1, true,  false, false },
    { ROP_CALL,        "CALL",        "Call function",                   INST_CAT_CONTROL,    1, true,  true,  false },
    { ROP_CALL_REG,    "CALL_REG",    "Call function at register",       INST_CAT_CONTROL,    1, true,  true,  false },
    { ROP_RET,         "RET",         "Return from function",            INST_CAT_CONTROL,    0, true,  false, false },
    { ROP_RET_VAL,     "RET_VAL",     "Return with value",               INST_CAT_CONTROL,    1, true,  false, false },
    
    // Data Movement Instructions
    { ROP_MOVE,        "MOVE",        "Move register to register",       INST_CAT_MEMORY,     2, false, false, false },
    { ROP_LOAD_IMM,    "LOAD_IMM",    "Load immediate value",            INST_CAT_MEMORY,     2, false, false, false },
    { ROP_LOAD_CONST,  "LOAD_CONST",  "Load from constant pool",         INST_CAT_MEMORY,     2, false, true,  false },
    { ROP_LOAD_GLOBAL, "LOAD_GLOBAL", "Load global variable",            INST_CAT_MEMORY,     2, false, true,  false },
    { ROP_STORE_GLOBAL,"STORE_GLOBAL","Store to global variable",        INST_CAT_MEMORY,     2, true,  true,  false },
    { ROP_LOAD_LOCAL,  "LOAD_LOCAL",  "Load local variable",             INST_CAT_MEMORY,     2, false, true,  false },
    { ROP_STORE_LOCAL, "STORE_LOCAL", "Store to local variable",         INST_CAT_MEMORY,     2, true,  true,  false },
    { ROP_LOAD_MEM,    "LOAD_MEM",    "Load from memory address",        INST_CAT_MEMORY,     2, false, true,  false },
    { ROP_STORE_MEM,   "STORE_MEM",   "Store to memory address",         INST_CAT_MEMORY,     2, true,  true,  false },
    { ROP_LOAD_OFFSET, "LOAD_OFFSET", "Load from base + offset",         INST_CAT_MEMORY,     3, false, true,  false },
    { ROP_STORE_OFFSET,"STORE_OFFSET","Store to base + offset",          INST_CAT_MEMORY,     3, true,  true,  false },
    { ROP_PUSH,        "PUSH",        "Push register to stack",          INST_CAT_MEMORY,     1, true,  true,  false },
    { ROP_POP,         "POP",         "Pop from stack to register",      INST_CAT_MEMORY,     1, true,  true,  false },
    
    // Arithmetic Instructions
    { ROP_ADD_I32,     "ADD_I32",     "Add 32-bit integers",             INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_SUB_I32,     "SUB_I32",     "Subtract 32-bit integers",        INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_MUL_I32,     "MUL_I32",     "Multiply 32-bit integers",        INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_DIV_I32,     "DIV_I32",     "Divide 32-bit integers",          INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_MOD_I32,     "MOD_I32",     "Modulo 32-bit integers",          INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_NEG_I32,     "NEG_I32",     "Negate 32-bit integer",           INST_CAT_ARITHMETIC, 2, false, false, true },
    { ROP_ADD_I64,     "ADD_I64",     "Add 64-bit integers",             INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_SUB_I64,     "SUB_I64",     "Subtract 64-bit integers",        INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_MUL_I64,     "MUL_I64",     "Multiply 64-bit integers",        INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_DIV_I64,     "DIV_I64",     "Divide 64-bit integers",          INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_MOD_I64,     "MOD_I64",     "Modulo 64-bit integers",          INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_NEG_I64,     "NEG_I64",     "Negate 64-bit integer",           INST_CAT_ARITHMETIC, 2, false, false, true },
    { ROP_ADD_U32,     "ADD_U32",     "Add 32-bit unsigned",             INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_ADD_U64,     "ADD_U64",     "Add 64-bit unsigned",             INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_MUL_U32,     "MUL_U32",     "Multiply 32-bit unsigned",        INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_MUL_U64,     "MUL_U64",     "Multiply 64-bit unsigned",        INST_CAT_ARITHMETIC, 3, false, true,  true },
    
    // Floating Point Instructions
    { ROP_ADD_F64,     "ADD_F64",     "Add 64-bit floats",               INST_CAT_ARITHMETIC, 3, false, false, true },
    { ROP_SUB_F64,     "SUB_F64",     "Subtract 64-bit floats",          INST_CAT_ARITHMETIC, 3, false, false, true },
    { ROP_MUL_F64,     "MUL_F64",     "Multiply 64-bit floats",          INST_CAT_ARITHMETIC, 3, false, false, true },
    { ROP_DIV_F64,     "DIV_F64",     "Divide 64-bit floats",            INST_CAT_ARITHMETIC, 3, false, true,  true },
    { ROP_NEG_F64,     "NEG_F64",     "Negate 64-bit float",             INST_CAT_ARITHMETIC, 2, false, false, true },
    { ROP_ABS_F64,     "ABS_F64",     "Absolute value 64-bit float",     INST_CAT_ARITHMETIC, 2, false, false, true },
    { ROP_SQRT_F64,    "SQRT_F64",    "Square root 64-bit float",        INST_CAT_ARITHMETIC, 2, false, true,  true },
    { ROP_FLOOR_F64,   "FLOOR_F64",   "Floor 64-bit float",              INST_CAT_ARITHMETIC, 2, false, false, true },
    { ROP_CEIL_F64,    "CEIL_F64",    "Ceiling 64-bit float",            INST_CAT_ARITHMETIC, 2, false, false, true },
    { ROP_ROUND_F64,   "ROUND_F64",   "Round 64-bit float",              INST_CAT_ARITHMETIC, 2, false, false, true },
    
    // Logical Instructions
    { ROP_AND,         "AND",         "Bitwise AND",                     INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_OR,          "OR",          "Bitwise OR",                      INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_XOR,         "XOR",         "Bitwise XOR",                     INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_NOT,         "NOT",         "Bitwise NOT",                     INST_CAT_LOGICAL,    2, false, false, true },
    { ROP_SHL,         "SHL",         "Shift left",                      INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_SHR,         "SHR",         "Shift right (logical)",           INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_SAR,         "SAR",         "Shift right (arithmetic)",        INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_BOOL_AND,    "BOOL_AND",    "Logical AND",                     INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_BOOL_OR,     "BOOL_OR",     "Logical OR",                      INST_CAT_LOGICAL,    3, false, false, true },
    { ROP_BOOL_NOT,    "BOOL_NOT",    "Logical NOT",                     INST_CAT_LOGICAL,    2, false, false, true },
    
    // Comparison Instructions
    { ROP_CMP_I32,     "CMP_I32",     "Compare 32-bit integers",         INST_CAT_COMPARISON, 2, false, false, true },
    { ROP_CMP_I64,     "CMP_I64",     "Compare 64-bit integers",         INST_CAT_COMPARISON, 2, false, false, true },
    { ROP_CMP_U32,     "CMP_U32",     "Compare 32-bit unsigned",         INST_CAT_COMPARISON, 2, false, false, true },
    { ROP_CMP_U64,     "CMP_U64",     "Compare 64-bit unsigned",         INST_CAT_COMPARISON, 2, false, false, true },
    { ROP_CMP_F64,     "CMP_F64",     "Compare 64-bit floats",           INST_CAT_COMPARISON, 2, false, false, true },
    { ROP_EQ_I32,      "EQ_I32",      "Equal 32-bit integers",           INST_CAT_COMPARISON, 3, false, false, false },
    { ROP_NE_I32,      "NE_I32",      "Not equal 32-bit integers",       INST_CAT_COMPARISON, 3, false, false, false },
    { ROP_LT_I32,      "LT_I32",      "Less than 32-bit integers",       INST_CAT_COMPARISON, 3, false, false, false },
    { ROP_LE_I32,      "LE_I32",      "Less than or equal 32-bit",       INST_CAT_COMPARISON, 3, false, false, false },
    { ROP_GT_I32,      "GT_I32",      "Greater than 32-bit integers",    INST_CAT_COMPARISON, 3, false, false, false },
    { ROP_GE_I32,      "GE_I32",      "Greater than or equal 32-bit",    INST_CAT_COMPARISON, 3, false, false, false },
    { ROP_EQ_STR,      "EQ_STR",      "String equality",                 INST_CAT_COMPARISON, 3, false, false, false },
    { ROP_EQ_OBJ,      "EQ_OBJ",      "Object equality",                 INST_CAT_COMPARISON, 3, false, false, false },
    
    // Type Instructions
    { ROP_CAST_I32_I64,"CAST_I32_I64","Cast i32 to i64",                INST_CAT_TYPE,       2, false, false, false },
    { ROP_CAST_I32_U32,"CAST_I32_U32","Cast i32 to u32",                INST_CAT_TYPE,       2, false, false, false },
    { ROP_CAST_I32_F64,"CAST_I32_F64","Cast i32 to f64",                INST_CAT_TYPE,       2, false, false, false },
    { ROP_CAST_I64_I32,"CAST_I64_I32","Cast i64 to i32",                INST_CAT_TYPE,       2, false, false, false },
    { ROP_CAST_F64_I32,"CAST_F64_I32","Cast f64 to i32",                INST_CAT_TYPE,       2, false, false, false },
    { ROP_CAST_TO_STR, "CAST_TO_STR", "Cast any type to string",         INST_CAT_TYPE,       2, false, true,  false },
    { ROP_CAST_TO_BOOL,"CAST_TO_BOOL","Cast any type to boolean",        INST_CAT_TYPE,       2, false, false, false },
    { ROP_TYPE_OF,     "TYPE_OF",     "Get type of value",               INST_CAT_TYPE,       2, false, true,  false },
    { ROP_IS_TYPE,     "IS_TYPE",     "Check if value is specific type", INST_CAT_TYPE,       3, false, false, false },
    { ROP_TYPE_CHECK,  "TYPE_CHECK",  "Runtime type check",              INST_CAT_TYPE,       2, false, true,  false },
    
    // Object Instructions
    { ROP_NEW_OBJECT,  "NEW_OBJECT",  "Create new object",               INST_CAT_OBJECT,     2, true,  true,  false },
    { ROP_NEW_ARRAY,   "NEW_ARRAY",   "Create new array",                INST_CAT_OBJECT,     2, true,  true,  false },
    { ROP_NEW_STRING,  "NEW_STRING",  "Create new string",               INST_CAT_OBJECT,     2, true,  true,  false },
    { ROP_NEW_STRUCT,  "NEW_STRUCT",  "Create new struct",               INST_CAT_OBJECT,     2, true,  true,  false },
    { ROP_NEW_ENUM,    "NEW_ENUM",    "Create new enum",                 INST_CAT_OBJECT,     2, true,  true,  false },
    { ROP_GET_FIELD,   "GET_FIELD",   "Get object field",                INST_CAT_OBJECT,     3, false, true,  false },
    { ROP_SET_FIELD,   "SET_FIELD",   "Set object field",                INST_CAT_OBJECT,     3, true,  true,  false },
    { ROP_GET_INDEX,   "GET_INDEX",   "Get array element",               INST_CAT_OBJECT,     3, false, true,  false },
    { ROP_SET_INDEX,   "SET_INDEX",   "Set array element",               INST_CAT_OBJECT,     3, true,  true,  false },
    { ROP_GET_LENGTH,  "GET_LENGTH",  "Get array/string length",         INST_CAT_OBJECT,     2, false, false, false },
    { ROP_CALL_METHOD, "CALL_METHOD", "Call object method",              INST_CAT_OBJECT,     2, true,  true,  false },
    { ROP_CALL_STATIC, "CALL_STATIC", "Call static method",              INST_CAT_OBJECT,     2, true,  true,  false },
    
    // Built-in Function Instructions
    { ROP_PRINT,       "PRINT",       "Print value",                     INST_CAT_BUILTIN,    1, true,  false, false },
    { ROP_INPUT,       "INPUT",       "Read input",                      INST_CAT_BUILTIN,    1, true,  true,  false },
    { ROP_LEN,         "LEN",         "Get length",                      INST_CAT_BUILTIN,    2, false, true,  false },
    { ROP_RANGE,       "RANGE",       "Create range",                    INST_CAT_BUILTIN,    3, true,  true,  false },
    { ROP_MIN,         "MIN",         "Find minimum",                    INST_CAT_BUILTIN,    3, false, true,  false },
    { ROP_MAX,         "MAX",         "Find maximum",                    INST_CAT_BUILTIN,    3, false, true,  false },
    { ROP_SUM,         "SUM",         "Sum array elements",              INST_CAT_BUILTIN,    2, false, true,  false },
    { ROP_SORTED,      "SORTED",      "Sort array (new copy)",           INST_CAT_BUILTIN,    2, true,  true,  false },
    { ROP_REVERSED,    "REVERSED",    "Reverse array (new copy)",        INST_CAT_BUILTIN,    2, true,  true,  false },
    { ROP_TIMESTAMP,   "TIMESTAMP",   "Get timestamp",                   INST_CAT_BUILTIN,    1, false, false, false },
};

/** Number of entries in the instruction table */
#define INSTRUCTION_TABLE_SIZE (sizeof(instruction_table) / sizeof(InstructionMetadata))

// =============================================================================
// METADATA LOOKUP FUNCTIONS
// =============================================================================

const InstructionMetadata* get_instruction_metadata(RegisterOpcode opcode) {
    // Linear search through the table
    for (size_t i = 0; i < INSTRUCTION_TABLE_SIZE; i++) {
        if (instruction_table[i].opcode == opcode) {
            return &instruction_table[i];
        }
    }
    return NULL;
}

const char* get_instruction_name(RegisterOpcode opcode) {
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    return meta ? meta->name : "UNKNOWN";
}

InstructionCategory get_instruction_category(RegisterOpcode opcode) {
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    return meta ? meta->category : INST_CAT_DEBUG; // Default to debug category
}

bool instruction_has_side_effects(RegisterOpcode opcode) {
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    return meta ? meta->has_side_effects : true; // Conservative default
}

bool instruction_can_throw(RegisterOpcode opcode) {
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    return meta ? meta->can_throw : true; // Conservative default
}

// =============================================================================
// INSTRUCTION VALIDATION
// =============================================================================

bool validate_instruction(uint32_t instruction) {
    RegisterOpcode opcode = (RegisterOpcode)GET_OPCODE(instruction);
    uint8_t dst = GET_DST(instruction);
    uint8_t src1 = GET_SRC1(instruction);
    uint8_t src2 = GET_SRC2(instruction);
    
    // Check if opcode is valid
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    if (!meta) {
        return false;
    }
    
    // Check register bounds (basic validation)
    if (dst >= TOTAL_REGISTER_COUNT || 
        src1 >= TOTAL_REGISTER_COUNT || 
        src2 >= TOTAL_REGISTER_COUNT) {
        return false;
    }
    
    // Additional validation based on instruction type
    switch (meta->category) {
        case INST_CAT_CONTROL:
            // Control flow instructions may have special requirements
            if (opcode == ROP_JMP || opcode == ROP_CALL) {
                // Jump target should be reasonable (this is a basic check)
                uint16_t target = GET_IMM(instruction);
                if (target == 0xFFFF) { // Invalid immediate
                    return false;
                }
            }
            break;
            
        case INST_CAT_MEMORY:
            // Memory instructions need valid register operands
            if (meta->operand_count > 0 && dst >= REGISTER_COUNT) {
                // Most memory operations shouldn't target special registers
                if (opcode != ROP_PUSH && opcode != ROP_POP) {
                    return false;
                }
            }
            break;
            
        case INST_CAT_ARITHMETIC:
        case INST_CAT_LOGICAL:
        case INST_CAT_COMPARISON:
            // Arithmetic/logical operations should use general-purpose registers
            if (dst >= REGISTER_COUNT || src1 >= REGISTER_COUNT || src2 >= REGISTER_COUNT) {
                return false;
            }
            break;
            
        default:
            // Other categories pass basic validation
            break;
    }
    
    return true;
}

// =============================================================================
// INSTRUCTION DISASSEMBLY
// =============================================================================

int disassemble_instruction(uint32_t instruction, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return 0;
    }
    
    RegisterOpcode opcode = (RegisterOpcode)GET_OPCODE(instruction);
    uint8_t dst = GET_DST(instruction);
    uint8_t src1 = GET_SRC1(instruction);
    uint8_t src2 = GET_SRC2(instruction);
    uint16_t imm = GET_IMM(instruction);
    
    const char* name = get_instruction_name(opcode);
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    
    if (!meta) {
        return snprintf(buffer, buffer_size, "UNKNOWN_%02X R%d, R%d, R%d", 
                       opcode, dst, src1, src2);
    }
    
    // Format instruction based on operand count and type
    switch (meta->operand_count) {
        case 0:
            return snprintf(buffer, buffer_size, "%s", name);
            
        case 1:
            // Check if it's an immediate instruction
            if (opcode == ROP_JMP || opcode == ROP_CALL || 
                opcode == ROP_LOAD_IMM || opcode == ROP_LOAD_CONST ||
                opcode == ROP_LOAD_GLOBAL || opcode == ROP_STORE_GLOBAL) {
                return snprintf(buffer, buffer_size, "%s R%d, #%d", name, dst, imm);
            } else {
                return snprintf(buffer, buffer_size, "%s R%d", name, dst);
            }
            
        case 2:
            // Check for immediate operands
            if (opcode == ROP_LOAD_IMM || opcode == ROP_LOAD_CONST ||
                opcode == ROP_LOAD_GLOBAL || opcode == ROP_STORE_GLOBAL ||
                opcode == ROP_LOAD_LOCAL || opcode == ROP_STORE_LOCAL) {
                return snprintf(buffer, buffer_size, "%s R%d, #%d", name, dst, imm);
            } else if (opcode == ROP_JZ || opcode == ROP_JNZ) {
                return snprintf(buffer, buffer_size, "%s R%d, #%d", name, src1, imm);
            } else {
                return snprintf(buffer, buffer_size, "%s R%d, R%d", name, dst, src1);
            }
            
        case 3:
            return snprintf(buffer, buffer_size, "%s R%d, R%d, R%d", 
                           name, dst, src1, src2);
            
        default:
            return snprintf(buffer, buffer_size, "%s R%d, R%d, R%d [+%d operands]", 
                           name, dst, src1, src2, meta->operand_count - 3);
    }
}

// =============================================================================
// INSTRUCTION ANALYSIS HELPERS
// =============================================================================

/**
 * @brief Check if instruction modifies a register
 */
bool instruction_modifies_register(uint32_t instruction, uint8_t reg) {
    RegisterOpcode opcode = (RegisterOpcode)GET_OPCODE(instruction);
    uint8_t dst = GET_DST(instruction);
    
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    if (!meta) {
        return false;
    }
    
    // Most instructions write to the destination register
    switch (opcode) {
        case ROP_NOP:
        case ROP_HALT:
        case ROP_JMP:
        case ROP_JMP_REG:
        case ROP_JZ:
        case ROP_JNZ:
        case ROP_JEQ:
        case ROP_JNE:
        case ROP_JLT:
        case ROP_JLE:
        case ROP_JGT:
        case ROP_JGE:
        case ROP_RET:
        case ROP_STORE_GLOBAL:
        case ROP_STORE_LOCAL:
        case ROP_STORE_MEM:
        case ROP_STORE_OFFSET:
        case ROP_PUSH:
        case ROP_CMP_I32:
        case ROP_CMP_I64:
        case ROP_CMP_U32:
        case ROP_CMP_U64:
        case ROP_CMP_F64:
        case ROP_SET_FIELD:
        case ROP_SET_INDEX:
        case ROP_PRINT:
            // These instructions don't modify registers (except possibly special ones)
            return false;
            
        case ROP_POP:
            // POP modifies the specified register
            return (dst == reg);
            
        default:
            // Most other instructions modify the destination register
            return (dst == reg);
    }
}

/**
 * @brief Check if instruction reads from a register
 */
bool instruction_reads_register(uint32_t instruction, uint8_t reg) {
    RegisterOpcode opcode = (RegisterOpcode)GET_OPCODE(instruction);
    uint8_t dst = GET_DST(instruction);
    uint8_t src1 = GET_SRC1(instruction);
    uint8_t src2 = GET_SRC2(instruction);
    
    const InstructionMetadata* meta = get_instruction_metadata(opcode);
    if (!meta) {
        return false;
    }
    
    // Check each operand
    switch (meta->operand_count) {
        case 3:
            if (src2 == reg) return true;
            // Fall through
        case 2:
            if (src1 == reg) return true;
            // Fall through
        case 1:
            // Some single-operand instructions read from dst
            if (opcode == ROP_PUSH || opcode == ROP_PRINT || opcode == ROP_RET_VAL) {
                if (dst == reg) return true;
            }
            break;
    }
    
    return false;
}

/**
 * @brief Get instruction execution cost (in cycles, rough estimate)
 */
uint32_t get_instruction_cost(RegisterOpcode opcode) {
    switch (get_instruction_category(opcode)) {
        case INST_CAT_CONTROL:
            return 2; // Branch prediction, pipeline flush potential
            
        case INST_CAT_MEMORY:
            return 3; // Memory access latency
            
        case INST_CAT_ARITHMETIC:
            if (opcode == ROP_DIV_I32 || opcode == ROP_DIV_I64 || 
                opcode == ROP_DIV_F64 || opcode == ROP_MOD_I32 || opcode == ROP_MOD_I64) {
                return 10; // Division is expensive
            }
            if (opcode >= ROP_ADD_F64 && opcode <= ROP_ROUND_F64) {
                return 2; // Floating point operations
            }
            return 1; // Basic integer arithmetic
            
        case INST_CAT_LOGICAL:
            return 1; // Fast bit operations
            
        case INST_CAT_COMPARISON:
            return 1; // Fast comparisons
            
        case INST_CAT_TYPE:
            return 2; // Type checks and casts
            
        case INST_CAT_OBJECT:
            return 5; // Object manipulation is more expensive
            
        case INST_CAT_STRING:
            return 4; // String operations
            
        case INST_CAT_ARRAY:
            return 3; // Array operations
            
        case INST_CAT_BUILTIN:
            return 10; // Built-in functions vary widely
            
        default:
            return 1; // Default cost
    }
}