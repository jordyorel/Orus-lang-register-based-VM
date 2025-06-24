/**
 * @file register_opcodes.h
 * @brief Orus Register VM Instruction Set Architecture
 * 
 * This file defines the complete instruction set for the Orus register VM.
 * Instructions are organized into logical categories with consistent naming
 * conventions and comprehensive documentation.
 * 
 * Instruction Format:
 * - Standard: OPCODE DST SRC1 SRC2 (4 bytes)
 * - Immediate: OPCODE DST IMMEDIATE (4 bytes)
 * - Branch: OPCODE CONDITION TARGET (4 bytes)
 * 
 * @author Orus Development Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef ORUS_REGISTER_OPCODES_H
#define ORUS_REGISTER_OPCODES_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// OPCODE ENUMERATION
// =============================================================================

/**
 * @brief Register VM instruction opcodes
 * 
 * Opcodes are organized into logical groups for maintainability.
 * Each group uses a specific range to allow for future expansion.
 */
typedef enum {
    // ==========================================================================
    // CONTROL FLOW (0x00 - 0x0F)
    // ==========================================================================
    
    ROP_NOP         = 0x00,  /**< No operation */
    ROP_HALT        = 0x01,  /**< Halt execution */
    
    // Unconditional jumps
    ROP_JMP         = 0x02,  /**< Unconditional jump to address */
    ROP_JMP_REG     = 0x03,  /**< Jump to address in register */
    
    // Conditional jumps
    ROP_JZ          = 0x04,  /**< Jump if zero */
    ROP_JNZ         = 0x05,  /**< Jump if not zero */
    ROP_JEQ         = 0x06,  /**< Jump if equal (flags) */
    ROP_JNE         = 0x07,  /**< Jump if not equal (flags) */
    ROP_JLT         = 0x08,  /**< Jump if less than (flags) */
    ROP_JLE         = 0x09,  /**< Jump if less than or equal (flags) */
    ROP_JGT         = 0x0A,  /**< Jump if greater than (flags) */
    ROP_JGE         = 0x0B,  /**< Jump if greater than or equal (flags) */
    
    // Function calls
    ROP_CALL        = 0x0C,  /**< Call function */
    ROP_CALL_REG    = 0x0D,  /**< Call function address in register */
    ROP_RET         = 0x0E,  /**< Return from function */
    ROP_RET_VAL     = 0x0F,  /**< Return with value in register */
    
    // ==========================================================================
    // DATA MOVEMENT (0x10 - 0x1F)
    // ==========================================================================
    
    ROP_MOVE        = 0x10,  /**< Move register to register */
    ROP_LOAD_IMM    = 0x11,  /**< Load immediate value */
    ROP_LOAD_CONST  = 0x12,  /**< Load from constant pool */
    ROP_LOAD_GLOBAL = 0x13,  /**< Load global variable */
    ROP_STORE_GLOBAL= 0x14,  /**< Store to global variable */
    ROP_LOAD_LOCAL  = 0x15,  /**< Load local variable */
    ROP_STORE_LOCAL = 0x16,  /**< Store to local variable */
    
    // Memory operations
    ROP_LOAD_MEM    = 0x17,  /**< Load from memory address */
    ROP_STORE_MEM   = 0x18,  /**< Store to memory address */
    ROP_LOAD_OFFSET = 0x19,  /**< Load from base + offset */
    ROP_STORE_OFFSET= 0x1A,  /**< Store to base + offset */
    
    // Stack operations (for compatibility)
    ROP_PUSH        = 0x1B,  /**< Push register to stack */
    ROP_POP         = 0x1C,  /**< Pop from stack to register */
    
    // ==========================================================================
    // ARITHMETIC OPERATIONS (0x20 - 0x2F)
    // ==========================================================================
    
    // Integer arithmetic
    ROP_ADD_I32     = 0x20,  /**< Add 32-bit integers */
    ROP_SUB_I32     = 0x21,  /**< Subtract 32-bit integers */
    ROP_MUL_I32     = 0x22,  /**< Multiply 32-bit integers */
    ROP_DIV_I32     = 0x23,  /**< Divide 32-bit integers */
    ROP_MOD_I32     = 0x24,  /**< Modulo 32-bit integers */
    ROP_NEG_I32     = 0x25,  /**< Negate 32-bit integer */
    
    // 64-bit integer arithmetic
    ROP_ADD_I64     = 0x26,  /**< Add 64-bit integers */
    ROP_SUB_I64     = 0x27,  /**< Subtract 64-bit integers */
    ROP_MUL_I64     = 0x28,  /**< Multiply 64-bit integers */
    ROP_DIV_I64     = 0x29,  /**< Divide 64-bit integers */
    ROP_MOD_I64     = 0x2A,  /**< Modulo 64-bit integers */
    ROP_NEG_I64     = 0x2B,  /**< Negate 64-bit integer */
    
    // Unsigned arithmetic
    ROP_ADD_U32     = 0x2C,  /**< Add 32-bit unsigned */
    ROP_ADD_U64     = 0x2D,  /**< Add 64-bit unsigned */
    ROP_MUL_U32     = 0x2E,  /**< Multiply 32-bit unsigned */
    ROP_MUL_U64     = 0x2F,  /**< Multiply 64-bit unsigned */
    
    // ==========================================================================
    // FLOATING POINT OPERATIONS (0x30 - 0x3F)
    // ==========================================================================
    
    ROP_ADD_F64     = 0x30,  /**< Add 64-bit floats */
    ROP_SUB_F64     = 0x31,  /**< Subtract 64-bit floats */
    ROP_MUL_F64     = 0x32,  /**< Multiply 64-bit floats */
    ROP_DIV_F64     = 0x33,  /**< Divide 64-bit floats */
    ROP_NEG_F64     = 0x34,  /**< Negate 64-bit float */
    ROP_ABS_F64     = 0x35,  /**< Absolute value 64-bit float */
    ROP_SQRT_F64    = 0x36,  /**< Square root 64-bit float */
    ROP_FLOOR_F64   = 0x37,  /**< Floor 64-bit float */
    ROP_CEIL_F64    = 0x38,  /**< Ceiling 64-bit float */
    ROP_ROUND_F64   = 0x39,  /**< Round 64-bit float */
    
    // ==========================================================================
    // LOGICAL OPERATIONS (0x40 - 0x4F)
    // ==========================================================================
    
    ROP_AND         = 0x40,  /**< Bitwise AND */
    ROP_OR          = 0x41,  /**< Bitwise OR */
    ROP_XOR         = 0x42,  /**< Bitwise XOR */
    ROP_NOT         = 0x43,  /**< Bitwise NOT */
    ROP_SHL         = 0x44,  /**< Shift left */
    ROP_SHR         = 0x45,  /**< Shift right (logical) */
    ROP_SAR         = 0x46,  /**< Shift right (arithmetic) */
    
    // Boolean operations
    ROP_BOOL_AND    = 0x47,  /**< Logical AND */
    ROP_BOOL_OR     = 0x48,  /**< Logical OR */
    ROP_BOOL_NOT    = 0x49,  /**< Logical NOT */
    
    // ==========================================================================
    // COMPARISON OPERATIONS (0x50 - 0x5F)
    // ==========================================================================
    
    // Integer comparisons
    ROP_CMP_I32     = 0x50,  /**< Compare 32-bit integers (set flags) */
    ROP_CMP_I64     = 0x51,  /**< Compare 64-bit integers (set flags) */
    ROP_CMP_U32     = 0x52,  /**< Compare 32-bit unsigned (set flags) */
    ROP_CMP_U64     = 0x53,  /**< Compare 64-bit unsigned (set flags) */
    ROP_CMP_F64     = 0x54,  /**< Compare 64-bit floats (set flags) */
    
    // Direct comparisons (result in register)
    ROP_EQ_I32      = 0x55,  /**< Equal 32-bit integers */
    ROP_NE_I32      = 0x56,  /**< Not equal 32-bit integers */
    ROP_LT_I32      = 0x57,  /**< Less than 32-bit integers */
    ROP_LE_I32      = 0x58,  /**< Less than or equal 32-bit integers */
    ROP_GT_I32      = 0x59,  /**< Greater than 32-bit integers */
    ROP_GE_I32      = 0x5A,  /**< Greater than or equal 32-bit integers */
    
    // String and object comparisons
    ROP_EQ_STR      = 0x5B,  /**< String equality */
    ROP_EQ_OBJ      = 0x5C,  /**< Object equality */
    
    // ==========================================================================
    // TYPE OPERATIONS (0x60 - 0x6F)
    // ==========================================================================
    
    ROP_CAST_I32_I64 = 0x60, /**< Cast i32 to i64 */
    ROP_CAST_I32_U32 = 0x61, /**< Cast i32 to u32 */
    ROP_CAST_I32_F64 = 0x62, /**< Cast i32 to f64 */
    ROP_CAST_I64_I32 = 0x63, /**< Cast i64 to i32 */
    ROP_CAST_F64_I32 = 0x64, /**< Cast f64 to i32 */
    ROP_CAST_TO_STR  = 0x65, /**< Cast any type to string */
    ROP_CAST_TO_BOOL = 0x66, /**< Cast any type to boolean */
    
    ROP_TYPE_OF     = 0x67,  /**< Get type of value */
    ROP_IS_TYPE     = 0x68,  /**< Check if value is specific type */
    ROP_TYPE_CHECK  = 0x69,  /**< Runtime type check */
    
    // ==========================================================================
    // OBJECT OPERATIONS (0x70 - 0x7F)
    // ==========================================================================
    
    // Object creation
    ROP_NEW_OBJECT  = 0x70,  /**< Create new object */
    ROP_NEW_ARRAY   = 0x71,  /**< Create new array */
    ROP_NEW_STRING  = 0x72,  /**< Create new string */
    ROP_NEW_STRUCT  = 0x73,  /**< Create new struct */
    ROP_NEW_ENUM    = 0x74,  /**< Create new enum */
    
    // Object access
    ROP_GET_FIELD   = 0x75,  /**< Get object field */
    ROP_SET_FIELD   = 0x76,  /**< Set object field */
    ROP_GET_INDEX   = 0x77,  /**< Get array element */
    ROP_SET_INDEX   = 0x78,  /**< Set array element */
    ROP_GET_LENGTH  = 0x79,  /**< Get array/string length */
    
    // Method calls
    ROP_CALL_METHOD = 0x7A,  /**< Call object method */
    ROP_CALL_STATIC = 0x7B,  /**< Call static method */
    
    // ==========================================================================
    // STRING OPERATIONS (0x80 - 0x8F)
    // ==========================================================================
    
    ROP_STR_CONCAT  = 0x80,  /**< Concatenate strings */
    ROP_STR_LENGTH  = 0x81,  /**< Get string length */
    ROP_STR_SUBSTR  = 0x82,  /**< Get substring */
    ROP_STR_CHAR_AT = 0x83,  /**< Get character at index */
    ROP_STR_INDEX_OF= 0x84,  /**< Find substring index */
    ROP_STR_COMPARE = 0x85,  /**< Compare strings */
    ROP_STR_TO_UPPER= 0x86,  /**< Convert to uppercase */
    ROP_STR_TO_LOWER= 0x87,  /**< Convert to lowercase */
    
    // ==========================================================================
    // ARRAY OPERATIONS (0x90 - 0x9F)
    // ==========================================================================
    
    ROP_ARRAY_PUSH  = 0x90,  /**< Push element to array */
    ROP_ARRAY_POP   = 0x91,  /**< Pop element from array */
    ROP_ARRAY_INSERT= 0x92,  /**< Insert element at index */
    ROP_ARRAY_REMOVE= 0x93,  /**< Remove element at index */
    ROP_ARRAY_SLICE = 0x94,  /**< Create array slice */
    ROP_ARRAY_CONCAT= 0x95,  /**< Concatenate arrays */
    ROP_ARRAY_REVERSE=0x96,  /**< Reverse array in place */
    ROP_ARRAY_SORT  = 0x97,  /**< Sort array */
    
    // ==========================================================================
    // GENERIC OPERATIONS (0xA0 - 0xAF)
    // ==========================================================================
    
    ROP_GENERIC_CALL= 0xA0,  /**< Call generic function */
    ROP_GENERIC_INST= 0xA1,  /**< Instantiate generic type */
    ROP_GENERIC_CHECK=0xA2,  /**< Check generic constraints */
    ROP_GENERIC_CAST= 0xA3,  /**< Generic type cast */
    
    // ==========================================================================
    // PATTERN MATCHING (0xB0 - 0xBF)
    // ==========================================================================
    
    ROP_MATCH_BEGIN = 0xB0,  /**< Begin match statement */
    ROP_MATCH_CASE  = 0xB1,  /**< Match case */
    ROP_MATCH_GUARD = 0xB2,  /**< Match guard condition */
    ROP_MATCH_END   = 0xB3,  /**< End match statement */
    ROP_ENUM_MATCH  = 0xB4,  /**< Match enum variant */
    ROP_STRUCT_MATCH= 0xB5,  /**< Match struct pattern */
    
    // ==========================================================================
    // EXCEPTION HANDLING (0xC0 - 0xCF)
    // ==========================================================================
    
    ROP_TRY_BEGIN   = 0xC0,  /**< Begin try block */
    ROP_TRY_END     = 0xC1,  /**< End try block */
    ROP_CATCH_BEGIN = 0xC2,  /**< Begin catch block */
    ROP_CATCH_END   = 0xC3,  /**< End catch block */
    ROP_THROW       = 0xC4,  /**< Throw exception */
    ROP_RETHROW     = 0xC5,  /**< Rethrow current exception */
    
    // ==========================================================================
    // MODULE OPERATIONS (0xD0 - 0xDF)
    // ==========================================================================
    
    ROP_IMPORT      = 0xD0,  /**< Import module */
    ROP_EXPORT      = 0xD1,  /**< Export symbol */
    ROP_MODULE_CALL = 0xD2,  /**< Call function in module */
    ROP_MODULE_GET  = 0xD3,  /**< Get module symbol */
    ROP_MODULE_SET  = 0xD4,  /**< Set module symbol */
    
    // ==========================================================================
    // BUILT-IN FUNCTIONS (0xE0 - 0xEF)
    // ==========================================================================
    
    ROP_PRINT       = 0xE0,  /**< Print value */
    ROP_INPUT       = 0xE1,  /**< Read input */
    ROP_LEN         = 0xE2,  /**< Get length */
    ROP_RANGE       = 0xE3,  /**< Create range */
    ROP_MIN         = 0xE4,  /**< Find minimum */
    ROP_MAX         = 0xE5,  /**< Find maximum */
    ROP_SUM         = 0xE6,  /**< Sum array elements */
    ROP_SORTED      = 0xE7,  /**< Sort array (new copy) */
    ROP_REVERSED    = 0xE8,  /**< Reverse array (new copy) */
    ROP_TIMESTAMP   = 0xE9,  /**< Get timestamp */
    
    // ==========================================================================
    // DEBUG AND PROFILING (0xF0 - 0xFF)
    // ==========================================================================
    
    ROP_DEBUG_BREAK = 0xF0,  /**< Debug breakpoint */
    ROP_DEBUG_PRINT = 0xF1,  /**< Debug print */
    ROP_DEBUG_TRACE = 0xF2,  /**< Debug trace */
    ROP_PROFILE_START=0xF3,  /**< Start profiling */
    ROP_PROFILE_END = 0xF4,  /**< End profiling */
    ROP_PROFILE_MARK= 0xF5,  /**< Profile marker */
    
    // Reserved for future expansion
    ROP_RESERVED    = 0xFF,  /**< Reserved opcode */
    
} RegisterOpcode;

// =============================================================================
// INSTRUCTION UTILITY MACROS
// =============================================================================

/**
 * @brief Create a standard 3-register instruction
 */
#define MAKE_INSTRUCTION(op, dst, src1, src2) \
    ((uint32_t)(op) | ((uint32_t)(dst) << 8) | ((uint32_t)(src1) << 16) | ((uint32_t)(src2) << 24))

/**
 * @brief Create an immediate instruction
 */
#define MAKE_IMM_INSTRUCTION(op, dst, imm) \
    ((uint32_t)(op) | ((uint32_t)(dst) << 8) | ((uint32_t)(imm) << 16))

/**
 * @brief Extract opcode from instruction
 */
#define GET_OPCODE(inst) ((uint8_t)((inst) & 0xFF))

/**
 * @brief Extract destination register from instruction
 */
#define GET_DST(inst) ((uint8_t)(((inst) >> 8) & 0xFF))

/**
 * @brief Extract first source register from instruction
 */
#define GET_SRC1(inst) ((uint8_t)(((inst) >> 16) & 0xFF))

/**
 * @brief Extract second source register from instruction
 */
#define GET_SRC2(inst) ((uint8_t)(((inst) >> 24) & 0xFF))

/**
 * @brief Extract immediate value from instruction
 */
#define GET_IMM(inst) ((uint16_t)(((inst) >> 16) & 0xFFFF))

// =============================================================================
// INSTRUCTION CATEGORIES
// =============================================================================

/**
 * @brief Instruction categories for analysis and optimization
 */
typedef enum {
    INST_CAT_CONTROL,      /**< Control flow instructions */
    INST_CAT_MEMORY,       /**< Memory access instructions */
    INST_CAT_ARITHMETIC,   /**< Arithmetic operations */
    INST_CAT_LOGICAL,      /**< Logical operations */
    INST_CAT_COMPARISON,   /**< Comparison operations */
    INST_CAT_TYPE,         /**< Type operations */
    INST_CAT_OBJECT,       /**< Object operations */
    INST_CAT_STRING,       /**< String operations */
    INST_CAT_ARRAY,        /**< Array operations */
    INST_CAT_GENERIC,      /**< Generic operations */
    INST_CAT_PATTERN,      /**< Pattern matching */
    INST_CAT_EXCEPTION,    /**< Exception handling */
    INST_CAT_MODULE,       /**< Module operations */
    INST_CAT_BUILTIN,      /**< Built-in functions */
    INST_CAT_DEBUG,        /**< Debug and profiling */
} InstructionCategory;

// =============================================================================
// INSTRUCTION METADATA
// =============================================================================

/**
 * @brief Instruction metadata for analysis and debugging
 */
typedef struct {
    RegisterOpcode opcode;        /**< Instruction opcode */
    const char* name;             /**< Human-readable name */
    const char* description;      /**< Brief description */
    InstructionCategory category; /**< Instruction category */
    uint8_t operand_count;        /**< Number of operands */
    bool has_side_effects;        /**< Whether instruction has side effects */
    bool can_throw;               /**< Whether instruction can throw exceptions */
    bool modifies_flags;          /**< Whether instruction modifies status flags */
} InstructionMetadata;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Get instruction metadata
 * 
 * @param opcode Instruction opcode
 * @return Pointer to instruction metadata
 */
const InstructionMetadata* get_instruction_metadata(RegisterOpcode opcode);

/**
 * @brief Get instruction name
 * 
 * @param opcode Instruction opcode
 * @return Instruction name string
 */
const char* get_instruction_name(RegisterOpcode opcode);

/**
 * @brief Get instruction category
 * 
 * @param opcode Instruction opcode
 * @return Instruction category
 */
InstructionCategory get_instruction_category(RegisterOpcode opcode);

/**
 * @brief Check if instruction has side effects
 * 
 * @param opcode Instruction opcode
 * @return true if instruction has side effects
 */
bool instruction_has_side_effects(RegisterOpcode opcode);

/**
 * @brief Check if instruction can throw exceptions
 * 
 * @param opcode Instruction opcode
 * @return true if instruction can throw
 */
bool instruction_can_throw(RegisterOpcode opcode);

/**
 * @brief Validate instruction format
 * 
 * @param instruction Raw instruction word
 * @return true if instruction is valid
 */
bool validate_instruction(uint32_t instruction);

/**
 * @brief Disassemble instruction to string
 * 
 * @param instruction Raw instruction word
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return Number of characters written
 */
int disassemble_instruction(uint32_t instruction, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // ORUS_REGISTER_OPCODES_H