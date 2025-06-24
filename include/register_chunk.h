/**
 * @file register_chunk.h
 * @brief Orus Register VM Bytecode Management
 * 
 * This file defines the bytecode chunk structure and associated functions
 * for managing compiled Orus programs. Chunks contain instructions, constants,
 * debug information, and metadata required for execution.
 * 
 * Key Features:
 * - Efficient instruction storage and access
 * - Constant pool management
 * - Debug information preservation
 * - Function and module metadata
 * - Serialization support
 * 
 * @author Orus Development Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef ORUS_REGISTER_CHUNK_H
#define ORUS_REGISTER_CHUNK_H

#include "common.h"
#include "value.h"
#include "register_opcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

typedef struct RegisterChunk RegisterChunk;
typedef struct FunctionInfo FunctionInfo;
typedef struct ModuleInfo ModuleInfo;
typedef struct DebugInfo DebugInfo;

// =============================================================================
// DEBUG INFORMATION
// =============================================================================

/**
 * @brief Source location information
 */
typedef struct SourceLocation {
    uint32_t line;          /**< Line number (1-based) */
    uint16_t column;        /**< Column number (1-based) */
    uint16_t file_index;    /**< Index into source files array */
} SourceLocation;

/**
 * @brief Debug information for runtime debugging
 */
struct DebugInfo {
    SourceLocation* locations;    /**< Source locations for each instruction */
    uint32_t location_count;      /**< Number of source locations */
    
    char** source_files;          /**< Array of source file paths */
    uint16_t source_file_count;   /**< Number of source files */
    uint16_t source_file_capacity; /**< Capacity of source files array */
    
    char** variable_names;        /**< Local variable names */
    uint32_t* variable_scopes;    /**< Variable scope ranges */
    uint16_t variable_count;      /**< Number of variables */
    
    uint32_t* line_starts;        /**< Instruction indices where lines start */
    uint32_t line_start_count;    /**< Number of line start entries */
};

// =============================================================================
// FUNCTION METADATA
// =============================================================================

/**
 * @brief Function metadata
 */
struct FunctionInfo {
    char* name;                   /**< Function name */
    uint32_t start_address;       /**< Starting instruction address */
    uint32_t end_address;         /**< Ending instruction address */
    uint8_t parameter_count;      /**< Number of parameters */
    uint8_t local_count;          /**< Number of local variables */
    uint8_t register_count;       /**< Number of registers used */
    ValueType* parameter_types;   /**< Parameter type information */
    ValueType return_type;        /**< Return type */
    bool is_generic;              /**< Whether function is generic */
    bool is_exported;             /**< Whether function is exported */
    uint16_t generic_param_count; /**< Number of generic parameters */
};

// =============================================================================
// MODULE METADATA
// =============================================================================

/**
 * @brief Module export entry
 */
typedef struct {
    char* name;                   /**< Export name */
    uint32_t address;             /**< Address of exported symbol */
    ValueType type;               /**< Type of exported symbol */
    bool is_function;             /**< Whether export is a function */
} ExportEntry;

/**
 * @brief Module import entry
 */
typedef struct {
    char* module_name;            /**< Name of imported module */
    char* symbol_name;            /**< Name of imported symbol */
    uint32_t local_address;       /**< Local address where symbol is bound */
    ValueType expected_type;      /**< Expected type of imported symbol */
} ImportEntry;

/**
 * @brief Module metadata
 */
struct ModuleInfo {
    char* name;                   /**< Module name */
    char* file_path;              /**< Source file path */
    uint32_t version;             /**< Module version */
    uint64_t compile_time;        /**< Compilation timestamp */
    
    ExportEntry* exports;         /**< Exported symbols */
    uint16_t export_count;        /**< Number of exports */
    uint16_t export_capacity;     /**< Export array capacity */
    
    ImportEntry* imports;         /**< Imported symbols */
    uint16_t import_count;        /**< Number of imports */
    uint16_t import_capacity;     /**< Import array capacity */
    
    char** dependencies;          /**< Module dependencies */
    uint16_t dependency_count;    /**< Number of dependencies */
};

// =============================================================================
// BYTECODE CHUNK
// =============================================================================

/**
 * @brief Bytecode chunk containing compiled instructions and metadata
 */
struct RegisterChunk {
    // Core instruction data
    uint32_t* code;               /**< Array of instructions */
    uint32_t code_count;          /**< Number of instructions */
    uint32_t code_capacity;       /**< Allocated instruction capacity */
    
    // Constant pool
    Value* constants;             /**< Constant pool */
    uint32_t constant_count;      /**< Number of constants */
    uint32_t constant_capacity;   /**< Constant pool capacity */
    
    // Global variables
    Value* globals;               /**< Global variable storage */
    uint16_t global_count;        /**< Number of global variables */
    uint16_t global_capacity;     /**< Global storage capacity */
    
    // Function information
    FunctionInfo* functions;      /**< Function metadata */
    uint16_t function_count;      /**< Number of functions */
    uint16_t function_capacity;   /**< Function array capacity */
    
    // Module information
    ModuleInfo* module;           /**< Module metadata */
    
    // Debug information
    DebugInfo* debug;             /**< Debug information (NULL if not available) */
    
    // Runtime type information
    ValueType* register_types;    /**< Expected register types */
    uint8_t max_registers;        /**< Maximum registers used */
    
    // Memory management
    bool owns_memory;             /**< Whether chunk owns its memory */
    uint32_t ref_count;           /**< Reference count for sharing */
    
    // Optimization hints
    bool is_optimized;            /**< Whether chunk has been optimized */
    uint32_t optimization_level;  /**< Optimization level used */
    
    // Checksum for integrity
    uint32_t checksum;            /**< CRC32 checksum of chunk data */
};

// =============================================================================
// CHUNK LIFECYCLE FUNCTIONS
// =============================================================================

/**
 * @brief Initialize a new bytecode chunk
 * 
 * @param chunk Pointer to chunk to initialize
 * @param module_name Name of the module (copied)
 * @return true on success, false on failure
 */
bool register_chunk_init(RegisterChunk* chunk, const char* module_name);

/**
 * @brief Free all resources associated with a chunk
 * 
 * @param chunk Pointer to chunk to free
 */
void register_chunk_free(RegisterChunk* chunk);

/**
 * @brief Clone a bytecode chunk
 * 
 * @param source Source chunk to clone
 * @param dest Destination chunk (must be uninitialized)
 * @return true on success, false on failure
 */
bool register_chunk_clone(const RegisterChunk* source, RegisterChunk* dest);

/**
 * @brief Add reference to chunk (for sharing)
 * 
 * @param chunk Pointer to chunk
 */
void register_chunk_ref(RegisterChunk* chunk);

/**
 * @brief Remove reference from chunk (for sharing)
 * 
 * @param chunk Pointer to chunk
 */
void register_chunk_unref(RegisterChunk* chunk);

// =============================================================================
// INSTRUCTION MANAGEMENT
// =============================================================================

/**
 * @brief Add an instruction to the chunk
 * 
 * @param chunk Pointer to chunk
 * @param instruction Instruction to add
 * @param line Line number for debug info
 * @param column Column number for debug info
 * @return Instruction address (index)
 */
uint32_t register_chunk_add_instruction(RegisterChunk* chunk, uint32_t instruction,
                                       uint32_t line, uint16_t column);

/**
 * @brief Get instruction at address
 * 
 * @param chunk Pointer to chunk
 * @param address Instruction address
 * @return Instruction word, or 0 if invalid address
 */
uint32_t register_chunk_get_instruction(const RegisterChunk* chunk, uint32_t address);

/**
 * @brief Set instruction at address
 * 
 * @param chunk Pointer to chunk
 * @param address Instruction address
 * @param instruction New instruction
 * @return true on success, false on invalid address
 */
bool register_chunk_set_instruction(RegisterChunk* chunk, uint32_t address,
                                   uint32_t instruction);

/**
 * @brief Get current instruction count
 * 
 * @param chunk Pointer to chunk
 * @return Number of instructions in chunk
 */
uint32_t register_chunk_instruction_count(const RegisterChunk* chunk);

// =============================================================================
// CONSTANT POOL MANAGEMENT
// =============================================================================

/**
 * @brief Add a constant to the pool
 * 
 * @param chunk Pointer to chunk
 * @param value Constant value to add
 * @return Constant index
 */
uint32_t register_chunk_add_constant(RegisterChunk* chunk, Value value);

/**
 * @brief Get constant from pool
 * 
 * @param chunk Pointer to chunk
 * @param index Constant index
 * @return Constant value, or NIL_VAL if invalid index
 */
Value register_chunk_get_constant(const RegisterChunk* chunk, uint32_t index);

/**
 * @brief Find existing constant in pool
 * 
 * @param chunk Pointer to chunk
 * @param value Value to find
 * @return Constant index, or UINT32_MAX if not found
 */
uint32_t register_chunk_find_constant(const RegisterChunk* chunk, Value value);

// =============================================================================
// FUNCTION MANAGEMENT
// =============================================================================

/**
 * @brief Add function metadata
 * 
 * @param chunk Pointer to chunk
 * @param name Function name
 * @param start_address Function start address
 * @param end_address Function end address
 * @param parameter_count Number of parameters
 * @param return_type Return type
 * @return Function index
 */
uint16_t register_chunk_add_function(RegisterChunk* chunk, const char* name,
                                    uint32_t start_address, uint32_t end_address,
                                    uint8_t parameter_count, ValueType return_type);

/**
 * @brief Get function metadata
 * 
 * @param chunk Pointer to chunk
 * @param index Function index
 * @return Pointer to function info, or NULL if invalid index
 */
const FunctionInfo* register_chunk_get_function(const RegisterChunk* chunk, uint16_t index);

/**
 * @brief Find function by name
 * 
 * @param chunk Pointer to chunk
 * @param name Function name
 * @return Function index, or UINT16_MAX if not found
 */
uint16_t register_chunk_find_function(const RegisterChunk* chunk, const char* name);

/**
 * @brief Find function containing address
 * 
 * @param chunk Pointer to chunk
 * @param address Instruction address
 * @return Function index, or UINT16_MAX if not found
 */
uint16_t register_chunk_find_function_at(const RegisterChunk* chunk, uint32_t address);

// =============================================================================
// GLOBAL VARIABLE MANAGEMENT
// =============================================================================

/**
 * @brief Add global variable
 * 
 * @param chunk Pointer to chunk
 * @param initial_value Initial value
 * @return Global variable index
 */
uint16_t register_chunk_add_global(RegisterChunk* chunk, Value initial_value);

/**
 * @brief Get global variable
 * 
 * @param chunk Pointer to chunk
 * @param index Global variable index
 * @return Global variable value, or NIL_VAL if invalid index
 */
Value register_chunk_get_global(const RegisterChunk* chunk, uint16_t index);

/**
 * @brief Set global variable
 * 
 * @param chunk Pointer to chunk
 * @param index Global variable index
 * @param value New value
 * @return true on success, false on invalid index
 */
bool register_chunk_set_global(RegisterChunk* chunk, uint16_t index, Value value);

// =============================================================================
// DEBUG INFORMATION
// =============================================================================

/**
 * @brief Enable debug information collection
 * 
 * @param chunk Pointer to chunk
 * @return true on success, false on failure
 */
bool register_chunk_enable_debug(RegisterChunk* chunk);

/**
 * @brief Add source file to debug info
 * 
 * @param chunk Pointer to chunk
 * @param file_path Source file path
 * @return File index
 */
uint16_t register_chunk_add_source_file(RegisterChunk* chunk, const char* file_path);

/**
 * @brief Get source location for instruction
 * 
 * @param chunk Pointer to chunk
 * @param address Instruction address
 * @return Pointer to source location, or NULL if not available
 */
const SourceLocation* register_chunk_get_location(const RegisterChunk* chunk, uint32_t address);

/**
 * @brief Get source file path
 * 
 * @param chunk Pointer to chunk
 * @param file_index File index
 * @return File path, or NULL if invalid index
 */
const char* register_chunk_get_source_file(const RegisterChunk* chunk, uint16_t file_index);

// =============================================================================
// MODULE MANAGEMENT
// =============================================================================

/**
 * @brief Add export to module
 * 
 * @param chunk Pointer to chunk
 * @param name Export name
 * @param address Export address
 * @param type Export type
 * @param is_function Whether export is a function
 * @return true on success, false on failure
 */
bool register_chunk_add_export(RegisterChunk* chunk, const char* name, uint32_t address,
                              ValueType type, bool is_function);

/**
 * @brief Find export by name
 * 
 * @param chunk Pointer to chunk
 * @param name Export name
 * @return Pointer to export entry, or NULL if not found
 */
const ExportEntry* register_chunk_find_export(const RegisterChunk* chunk, const char* name);

/**
 * @brief Add import to module
 * 
 * @param chunk Pointer to chunk
 * @param module_name Module to import from
 * @param symbol_name Symbol to import
 * @param local_address Local binding address
 * @param expected_type Expected symbol type
 * @return true on success, false on failure
 */
bool register_chunk_add_import(RegisterChunk* chunk, const char* module_name,
                              const char* symbol_name, uint32_t local_address,
                              ValueType expected_type);

// =============================================================================
// SERIALIZATION
// =============================================================================

/**
 * @brief Serialize chunk to binary format
 * 
 * @param chunk Pointer to chunk
 * @param buffer Output buffer (allocated by function)
 * @param size Output buffer size
 * @return true on success, false on failure
 */
bool register_chunk_serialize(const RegisterChunk* chunk, uint8_t** buffer, size_t* size);

/**
 * @brief Deserialize chunk from binary format
 * 
 * @param buffer Input buffer
 * @param size Buffer size
 * @param chunk Output chunk (must be uninitialized)
 * @return true on success, false on failure
 */
bool register_chunk_deserialize(const uint8_t* buffer, size_t size, RegisterChunk* chunk);

/**
 * @brief Calculate chunk checksum
 * 
 * @param chunk Pointer to chunk
 * @return CRC32 checksum
 */
uint32_t register_chunk_checksum(const RegisterChunk* chunk);

/**
 * @brief Verify chunk integrity
 * 
 * @param chunk Pointer to chunk
 * @return true if chunk is valid, false otherwise
 */
bool register_chunk_verify(const RegisterChunk* chunk);

// =============================================================================
// OPTIMIZATION
// =============================================================================

/**
 * @brief Apply basic optimizations to chunk
 * 
 * @param chunk Pointer to chunk
 * @param level Optimization level (0-3)
 * @return true on success, false on failure
 */
bool register_chunk_optimize(RegisterChunk* chunk, uint32_t level);

/**
 * @brief Check if chunk is optimized
 * 
 * @param chunk Pointer to chunk
 * @return true if chunk has been optimized
 */
bool register_chunk_is_optimized(const RegisterChunk* chunk);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Print chunk statistics
 * 
 * @param chunk Pointer to chunk
 * @param detailed Whether to include detailed information
 */
void register_chunk_print_stats(const RegisterChunk* chunk, bool detailed);

/**
 * @brief Disassemble entire chunk
 * 
 * @param chunk Pointer to chunk
 * @param include_debug Whether to include debug information
 */
void register_chunk_disassemble(const RegisterChunk* chunk, bool include_debug);

/**
 * @brief Validate chunk consistency
 * 
 * @param chunk Pointer to chunk
 * @return true if chunk is consistent, false otherwise
 */
bool register_chunk_validate(const RegisterChunk* chunk);

#ifdef __cplusplus
}
#endif

#endif // ORUS_REGISTER_CHUNK_H