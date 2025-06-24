/**
 * @file register_chunk.c
 * @brief Orus Register VM Bytecode Chunk Implementation
 * 
 * This file implements bytecode chunk management for the Orus register VM.
 * Chunks contain instructions, constants, debug information, and metadata.
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
#include <assert.h>

#include "../../include/register_chunk.h"
#include "../../include/register_opcodes.h"
#include "../../include/memory.h"
#include "../../include/value.h"

// =============================================================================
// PRIVATE CONSTANTS
// =============================================================================

/** Initial capacity for dynamic arrays */
#define INITIAL_CAPACITY 8

/** Growth factor for dynamic arrays */
#define GROWTH_FACTOR 2

/** CRC32 polynomial for checksum calculation */
#define CRC32_POLYNOMIAL 0xEDB88320

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static bool grow_code_array(RegisterChunk* chunk);
static bool grow_constant_array(RegisterChunk* chunk);
static bool grow_global_array(RegisterChunk* chunk);
static bool grow_function_array(RegisterChunk* chunk);
static bool grow_source_file_array(DebugInfo* debug);
static uint32_t calculate_crc32(const uint8_t* data, size_t length);
static void free_function_info(FunctionInfo* func);
static void free_module_info(ModuleInfo* module);
static void free_debug_info(DebugInfo* debug);

// =============================================================================
// CHUNK LIFECYCLE FUNCTIONS
// =============================================================================

bool register_chunk_init(RegisterChunk* chunk, const char* module_name) {
    if (!chunk) {
        return false;
    }
    
    // Initialize all fields to zero/NULL
    memset(chunk, 0, sizeof(RegisterChunk));
    
    // Initialize code array
    chunk->code = malloc(INITIAL_CAPACITY * sizeof(uint32_t));
    if (!chunk->code) {
        return false;
    }
    chunk->code_count = 0;
    chunk->code_capacity = INITIAL_CAPACITY;
    
    // Initialize constant pool
    chunk->constants = malloc(INITIAL_CAPACITY * sizeof(Value));
    if (!chunk->constants) {
        free(chunk->code);
        return false;
    }
    chunk->constant_count = 0;
    chunk->constant_capacity = INITIAL_CAPACITY;
    
    // Initialize global variables
    chunk->globals = malloc(INITIAL_CAPACITY * sizeof(Value));
    if (!chunk->globals) {
        free(chunk->code);
        free(chunk->constants);
        return false;
    }
    chunk->global_count = 0;
    chunk->global_capacity = INITIAL_CAPACITY;
    
    // Initialize functions array
    chunk->functions = malloc(INITIAL_CAPACITY * sizeof(FunctionInfo));
    if (!chunk->functions) {
        free(chunk->code);
        free(chunk->constants);
        free(chunk->globals);
        return false;
    }
    chunk->function_count = 0;
    chunk->function_capacity = INITIAL_CAPACITY;
    
    // Initialize module info
    chunk->module = malloc(sizeof(ModuleInfo));
    if (!chunk->module) {
        free(chunk->code);
        free(chunk->constants);
        free(chunk->globals);
        free(chunk->functions);
        return false;
    }
    memset(chunk->module, 0, sizeof(ModuleInfo));
    
    // Set module name
    if (module_name) {
        chunk->module->name = malloc(strlen(module_name) + 1);
        if (chunk->module->name) {
            strcpy(chunk->module->name, module_name);
        }
    }
    
    // Initialize other fields
    chunk->debug = NULL; // Debug info is optional
    chunk->register_types = NULL;
    chunk->max_registers = 0;
    chunk->owns_memory = true;
    chunk->ref_count = 1;
    chunk->is_optimized = false;
    chunk->optimization_level = 0;
    chunk->checksum = 0;
    
    return true;
}

void register_chunk_free(RegisterChunk* chunk) {
    if (!chunk) {
        return;
    }
    
    // Decrement reference count
    if (chunk->ref_count > 1) {
        chunk->ref_count--;
        return;
    }
    
    // Free arrays if we own the memory
    if (chunk->owns_memory) {
        free(chunk->code);
        free(chunk->constants);
        free(chunk->globals);
        
        // Free function info
        if (chunk->functions) {
            for (uint16_t i = 0; i < chunk->function_count; i++) {
                free_function_info(&chunk->functions[i]);
            }
            free(chunk->functions);
        }
        
        // Free module info
        if (chunk->module) {
            free_module_info(chunk->module);
            free(chunk->module);
        }
        
        // Free debug info
        if (chunk->debug) {
            free_debug_info(chunk->debug);
            free(chunk->debug);
        }
        
        // Free register types
        free(chunk->register_types);
    }
    
    // Clear the chunk
    memset(chunk, 0, sizeof(RegisterChunk));
}

bool register_chunk_clone(const RegisterChunk* source, RegisterChunk* dest) {
    if (!source || !dest) {
        return false;
    }
    
    // Initialize destination
    if (!register_chunk_init(dest, source->module ? source->module->name : NULL)) {
        return false;
    }
    
    // Copy code
    if (source->code_count > 0) {
        if (source->code_count > dest->code_capacity) {
            uint32_t* new_code = realloc(dest->code, source->code_count * sizeof(uint32_t));
            if (!new_code) {
                register_chunk_free(dest);
                return false;
            }
            dest->code = new_code;
            dest->code_capacity = source->code_count;
        }
        memcpy(dest->code, source->code, source->code_count * sizeof(uint32_t));
        dest->code_count = source->code_count;
    }
    
    // Copy constants
    if (source->constant_count > 0) {
        if (source->constant_count > dest->constant_capacity) {
            Value* new_constants = realloc(dest->constants, source->constant_count * sizeof(Value));
            if (!new_constants) {
                register_chunk_free(dest);
                return false;
            }
            dest->constants = new_constants;
            dest->constant_capacity = source->constant_count;
        }
        memcpy(dest->constants, source->constants, source->constant_count * sizeof(Value));
        dest->constant_count = source->constant_count;
    }
    
    // Copy other fields
    dest->max_registers = source->max_registers;
    dest->is_optimized = source->is_optimized;
    dest->optimization_level = source->optimization_level;
    dest->checksum = source->checksum;
    
    return true;
}

void register_chunk_ref(RegisterChunk* chunk) {
    if (chunk) {
        chunk->ref_count++;
    }
}

void register_chunk_unref(RegisterChunk* chunk) {
    if (chunk && chunk->ref_count > 0) {
        chunk->ref_count--;
        if (chunk->ref_count == 0) {
            register_chunk_free(chunk);
        }
    }
}

// =============================================================================
// INSTRUCTION MANAGEMENT
// =============================================================================

uint32_t register_chunk_add_instruction(RegisterChunk* chunk, uint32_t instruction,
                                       uint32_t line, uint16_t column) {
    if (!chunk) {
        return UINT32_MAX;
    }
    
    // Grow array if needed
    if (chunk->code_count >= chunk->code_capacity) {
        if (!grow_code_array(chunk)) {
            return UINT32_MAX;
        }
    }
    
    // Add instruction
    uint32_t address = chunk->code_count;
    chunk->code[chunk->code_count++] = instruction;
    
    // Add debug info if enabled
    if (chunk->debug) {
        if (chunk->debug->location_count < chunk->code_count) {
            // Grow location array
            size_t new_capacity = chunk->code_count * 2;
            SourceLocation* new_locations = realloc(chunk->debug->locations,
                                                   new_capacity * sizeof(SourceLocation));
            if (new_locations) {
                chunk->debug->locations = new_locations;
            }
        }
        
        if (chunk->debug->locations && chunk->debug->location_count < chunk->code_count) {
            chunk->debug->locations[address].line = line;
            chunk->debug->locations[address].column = column;
            chunk->debug->locations[address].file_index = 0; // Default to first file
            chunk->debug->location_count = chunk->code_count;
        }
    }
    
    return address;
}

uint32_t register_chunk_get_instruction(const RegisterChunk* chunk, uint32_t address) {
    if (!chunk || address >= chunk->code_count) {
        return 0;
    }
    return chunk->code[address];
}

bool register_chunk_set_instruction(RegisterChunk* chunk, uint32_t address,
                                   uint32_t instruction) {
    if (!chunk || address >= chunk->code_count) {
        return false;
    }
    chunk->code[address] = instruction;
    return true;
}

uint32_t register_chunk_instruction_count(const RegisterChunk* chunk) {
    return chunk ? chunk->code_count : 0;
}

// =============================================================================
// CONSTANT POOL MANAGEMENT
// =============================================================================

uint32_t register_chunk_add_constant(RegisterChunk* chunk, Value value) {
    if (!chunk) {
        return UINT32_MAX;
    }
    
    // Check if constant already exists
    uint32_t existing = register_chunk_find_constant(chunk, value);
    if (existing != UINT32_MAX) {
        return existing;
    }
    
    // Grow array if needed
    if (chunk->constant_count >= chunk->constant_capacity) {
        if (!grow_constant_array(chunk)) {
            return UINT32_MAX;
        }
    }
    
    // Add constant
    uint32_t index = chunk->constant_count;
    chunk->constants[chunk->constant_count++] = value;
    return index;
}

Value register_chunk_get_constant(const RegisterChunk* chunk, uint32_t index) {
    if (!chunk || index >= chunk->constant_count) {
        return NIL_VAL;
    }
    return chunk->constants[index];
}

uint32_t register_chunk_find_constant(const RegisterChunk* chunk, Value value) {
    if (!chunk) {
        return UINT32_MAX;
    }
    
    for (uint32_t i = 0; i < chunk->constant_count; i++) {
        if (valuesEqual(chunk->constants[i], value)) {
            return i;
        }
    }
    
    return UINT32_MAX;
}

// =============================================================================
// FUNCTION MANAGEMENT
// =============================================================================

uint16_t register_chunk_add_function(RegisterChunk* chunk, const char* name,
                                    uint32_t start_address, uint32_t end_address,
                                    uint8_t parameter_count, ValueType return_type) {
    if (!chunk || !name) {
        return UINT16_MAX;
    }
    
    // Grow array if needed
    if (chunk->function_count >= chunk->function_capacity) {
        if (!grow_function_array(chunk)) {
            return UINT16_MAX;
        }
    }
    
    // Add function
    uint16_t index = chunk->function_count;
    FunctionInfo* func = &chunk->functions[chunk->function_count++];
    
    memset(func, 0, sizeof(FunctionInfo));
    func->name = malloc(strlen(name) + 1);
    if (func->name) {
        strcpy(func->name, name);
    }
    func->start_address = start_address;
    func->end_address = end_address;
    func->parameter_count = parameter_count;
    func->return_type = return_type;
    func->is_generic = false;
    func->is_exported = false;
    
    return index;
}

const FunctionInfo* register_chunk_get_function(const RegisterChunk* chunk, uint16_t index) {
    if (!chunk || index >= chunk->function_count) {
        return NULL;
    }
    return &chunk->functions[index];
}

uint16_t register_chunk_find_function(const RegisterChunk* chunk, const char* name) {
    if (!chunk || !name) {
        return UINT16_MAX;
    }
    
    for (uint16_t i = 0; i < chunk->function_count; i++) {
        if (chunk->functions[i].name && strcmp(chunk->functions[i].name, name) == 0) {
            return i;
        }
    }
    
    return UINT16_MAX;
}

uint16_t register_chunk_find_function_at(const RegisterChunk* chunk, uint32_t address) {
    if (!chunk) {
        return UINT16_MAX;
    }
    
    for (uint16_t i = 0; i < chunk->function_count; i++) {
        if (address >= chunk->functions[i].start_address &&
            address <= chunk->functions[i].end_address) {
            return i;
        }
    }
    
    return UINT16_MAX;
}

// =============================================================================
// GLOBAL VARIABLE MANAGEMENT
// =============================================================================

uint16_t register_chunk_add_global(RegisterChunk* chunk, Value initial_value) {
    if (!chunk) {
        return UINT16_MAX;
    }
    
    // Grow array if needed
    if (chunk->global_count >= chunk->global_capacity) {
        if (!grow_global_array(chunk)) {
            return UINT16_MAX;
        }
    }
    
    // Add global
    uint16_t index = chunk->global_count;
    chunk->globals[chunk->global_count++] = initial_value;
    return index;
}

Value register_chunk_get_global(const RegisterChunk* chunk, uint16_t index) {
    if (!chunk || index >= chunk->global_count) {
        return NIL_VAL;
    }
    return chunk->globals[index];
}

bool register_chunk_set_global(RegisterChunk* chunk, uint16_t index, Value value) {
    if (!chunk || index >= chunk->global_count) {
        return false;
    }
    chunk->globals[index] = value;
    return true;
}

// =============================================================================
// DEBUG INFORMATION
// =============================================================================

bool register_chunk_enable_debug(RegisterChunk* chunk) {
    if (!chunk) {
        return false;
    }
    
    if (!chunk->debug) {
        chunk->debug = malloc(sizeof(DebugInfo));
        if (!chunk->debug) {
            return false;
        }
        memset(chunk->debug, 0, sizeof(DebugInfo));
        
        // Initialize debug arrays
        chunk->debug->locations = malloc(INITIAL_CAPACITY * sizeof(SourceLocation));
        chunk->debug->source_files = malloc(INITIAL_CAPACITY * sizeof(char*));
        
        if (!chunk->debug->locations || !chunk->debug->source_files) {
            free_debug_info(chunk->debug);
            free(chunk->debug);
            chunk->debug = NULL;
            return false;
        }
        
        chunk->debug->source_file_capacity = INITIAL_CAPACITY;
    }
    
    return true;
}

uint16_t register_chunk_add_source_file(RegisterChunk* chunk, const char* file_path) {
    if (!chunk || !file_path || !chunk->debug) {
        return UINT16_MAX;
    }
    
    // Check if file already exists
    for (uint16_t i = 0; i < chunk->debug->source_file_count; i++) {
        if (chunk->debug->source_files[i] && 
            strcmp(chunk->debug->source_files[i], file_path) == 0) {
            return i;
        }
    }
    
    // Grow array if needed
    if (chunk->debug->source_file_count >= chunk->debug->source_file_capacity) {
        if (!grow_source_file_array(chunk->debug)) {
            return UINT16_MAX;
        }
    }
    
    // Add file
    uint16_t index = chunk->debug->source_file_count;
    chunk->debug->source_files[index] = malloc(strlen(file_path) + 1);
    if (chunk->debug->source_files[index]) {
        strcpy(chunk->debug->source_files[index], file_path);
        chunk->debug->source_file_count++;
        return index;
    }
    
    return UINT16_MAX;
}

const SourceLocation* register_chunk_get_location(const RegisterChunk* chunk, uint32_t address) {
    if (!chunk || !chunk->debug || address >= chunk->debug->location_count) {
        return NULL;
    }
    return &chunk->debug->locations[address];
}

const char* register_chunk_get_source_file(const RegisterChunk* chunk, uint16_t file_index) {
    if (!chunk || !chunk->debug || file_index >= chunk->debug->source_file_count) {
        return NULL;
    }
    return chunk->debug->source_files[file_index];
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void register_chunk_print_stats(const RegisterChunk* chunk, bool detailed) {
    if (!chunk) {
        printf("Chunk: NULL\n");
        return;
    }
    
    printf("=== Register Chunk Statistics ===\n");
    printf("Instructions: %u / %u\n", chunk->code_count, chunk->code_capacity);
    printf("Constants: %u / %u\n", chunk->constant_count, chunk->constant_capacity);
    printf("Globals: %u / %u\n", chunk->global_count, chunk->global_capacity);
    printf("Functions: %u / %u\n", chunk->function_count, chunk->function_capacity);
    printf("Max Registers: %u\n", chunk->max_registers);
    printf("Module: %s\n", chunk->module && chunk->module->name ? chunk->module->name : "unknown");
    printf("Optimized: %s (level %u)\n", chunk->is_optimized ? "yes" : "no", chunk->optimization_level);
    printf("Debug Info: %s\n", chunk->debug ? "yes" : "no");
    printf("Reference Count: %u\n", chunk->ref_count);
    printf("Checksum: 0x%08X\n", chunk->checksum);
    
    if (detailed && chunk->function_count > 0) {
        printf("\n=== Functions ===\n");
        for (uint16_t i = 0; i < chunk->function_count; i++) {
            const FunctionInfo* func = &chunk->functions[i];
            printf("%u: %s [%04X-%04X] %u params\n", 
                   i, func->name ? func->name : "unnamed", 
                   func->start_address, func->end_address, func->parameter_count);
        }
    }
    
    printf("================================\n");
}

bool register_chunk_validate(const RegisterChunk* chunk) {
    if (!chunk) {
        return false;
    }
    
    // Check basic structure
    if (!chunk->code || !chunk->constants || !chunk->globals || !chunk->functions) {
        return false;
    }
    
    // Check array bounds
    if (chunk->code_count > chunk->code_capacity ||
        chunk->constant_count > chunk->constant_capacity ||
        chunk->global_count > chunk->global_capacity ||
        chunk->function_count > chunk->function_capacity) {
        return false;
    }
    
    // Validate function addresses
    for (uint16_t i = 0; i < chunk->function_count; i++) {
        const FunctionInfo* func = &chunk->functions[i];
        if (func->start_address >= chunk->code_count ||
            func->end_address >= chunk->code_count ||
            func->start_address > func->end_address) {
            return false;
        }
    }
    
    return true;
}

// =============================================================================
// PRIVATE HELPER FUNCTIONS
// =============================================================================

static bool grow_code_array(RegisterChunk* chunk) {
    uint32_t new_capacity = chunk->code_capacity * GROWTH_FACTOR;
    uint32_t* new_code = realloc(chunk->code, new_capacity * sizeof(uint32_t));
    if (!new_code) {
        return false;
    }
    chunk->code = new_code;
    chunk->code_capacity = new_capacity;
    return true;
}

static bool grow_constant_array(RegisterChunk* chunk) {
    uint32_t new_capacity = chunk->constant_capacity * GROWTH_FACTOR;
    Value* new_constants = realloc(chunk->constants, new_capacity * sizeof(Value));
    if (!new_constants) {
        return false;
    }
    chunk->constants = new_constants;
    chunk->constant_capacity = new_capacity;
    return true;
}

static bool grow_global_array(RegisterChunk* chunk) {
    uint16_t new_capacity = chunk->global_capacity * GROWTH_FACTOR;
    Value* new_globals = realloc(chunk->globals, new_capacity * sizeof(Value));
    if (!new_globals) {
        return false;
    }
    chunk->globals = new_globals;
    chunk->global_capacity = new_capacity;
    return true;
}

static bool grow_function_array(RegisterChunk* chunk) {
    uint16_t new_capacity = chunk->function_capacity * GROWTH_FACTOR;
    FunctionInfo* new_functions = realloc(chunk->functions, new_capacity * sizeof(FunctionInfo));
    if (!new_functions) {
        return false;
    }
    chunk->functions = new_functions;
    chunk->function_capacity = new_capacity;
    return true;
}

static bool grow_source_file_array(DebugInfo* debug) {
    uint16_t new_capacity = debug->source_file_capacity * GROWTH_FACTOR;
    char** new_files = realloc(debug->source_files, new_capacity * sizeof(char*));
    if (!new_files) {
        return false;
    }
    debug->source_files = new_files;
    debug->source_file_capacity = new_capacity;
    return true;
}

static void free_function_info(FunctionInfo* func) {
    if (func) {
        free(func->name);
        free(func->parameter_types);
        memset(func, 0, sizeof(FunctionInfo));
    }
}

static void free_module_info(ModuleInfo* module) {
    if (module) {
        free(module->name);
        free(module->file_path);
        
        // Free exports
        if (module->exports) {
            for (uint16_t i = 0; i < module->export_count; i++) {
                free(module->exports[i].name);
            }
            free(module->exports);
        }
        
        // Free imports
        if (module->imports) {
            for (uint16_t i = 0; i < module->import_count; i++) {
                free(module->imports[i].module_name);
                free(module->imports[i].symbol_name);
            }
            free(module->imports);
        }
        
        // Free dependencies
        if (module->dependencies) {
            for (uint16_t i = 0; i < module->dependency_count; i++) {
                free(module->dependencies[i]);
            }
            free(module->dependencies);
        }
    }
}

static void free_debug_info(DebugInfo* debug) {
    if (debug) {
        free(debug->locations);
        
        // Free source files
        if (debug->source_files) {
            for (uint16_t i = 0; i < debug->source_file_count; i++) {
                free(debug->source_files[i]);
            }
            free(debug->source_files);
        }
        
        // Free variable names
        if (debug->variable_names) {
            for (uint16_t i = 0; i < debug->variable_count; i++) {
                free(debug->variable_names[i]);
            }
            free(debug->variable_names);
        }
        
        free(debug->variable_scopes);
        free(debug->line_starts);
    }
}