#ifndef ORUS_VM_H
#define ORUS_VM_H

#include "common.h"
#include "chunk.h"
#include "register_chunk.h"
#include "register_vm.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct ASTNode;

/**
 * @brief Main VM structure that contains both stack and register VM components
 */
typedef struct {
    // Register VM components
    RegisterChunk regChunk;
    RegisterVM regVM;
    
    // Stack VM components (for compatibility)
    Chunk chunk;
    Value* stack;
    Value* stackTop;
    
    // Global state
    bool initialized;
    bool useRegisterVM;
    
    // File and module management
    const char* filePath;
    ObjString** loadedModules;  // Array of module name strings
    int moduleCount;
    
    // AST root for current compilation
    struct ASTNode* astRoot;
    
    // Error handling
    Value lastError;
    
    // Development and debugging flags
    bool trace;
    bool devMode;
    const char* stdPath;
} VM;

// Global VM instance
extern VM vm;

// =============================================================================
// VM LIFECYCLE FUNCTIONS
// =============================================================================

/**
 * @brief Initialize the main VM
 */
void initVM(void);

/**
 * @brief Free the main VM
 */
void freeVM(void);

// =============================================================================
// REGISTER VM WRAPPER FUNCTIONS
// =============================================================================

/**
 * @brief Initialize a register chunk
 * @param chunk Pointer to chunk to initialize
 */
void initRegisterChunk(RegisterChunk* chunk);

/**
 * @brief Free a register chunk
 * @param chunk Pointer to chunk to free
 */
void freeRegisterChunk(RegisterChunk* chunk);

/**
 * @brief Initialize a register VM
 * @param vm Pointer to RegisterVM to initialize
 * @param chunk Pointer to chunk to execute
 */
void initRegisterVM(RegisterVM* vm, RegisterChunk* chunk);

/**
 * @brief Free a register VM
 * @param vm Pointer to RegisterVM to free
 */
void freeRegisterVM(RegisterVM* vm);

/**
 * @brief Run the register VM
 * @param vm Pointer to RegisterVM to run
 */
void runRegisterVM(RegisterVM* vm);

/**
 * @brief Disassemble a register chunk
 * @param chunk Pointer to chunk to disassemble
 * @param name Name to display for the chunk
 */
void disassembleRegisterChunk(RegisterChunk* chunk, const char* name);

#ifdef __cplusplus
}
#endif

#endif // ORUS_VM_H
