/**
 * @file register_vm.h
 * @brief Orus Register Virtual Machine - Core Engine
 * 
 * This file defines the core data structures and interfaces for the Orus
 * register-based virtual machine. The VM uses a 32-register architecture
 * with efficient instruction dispatch and integrated memory management.
 * 
 * Key Features:
 * - 32 general-purpose registers (R0-R31)
 * - Special-purpose registers (SP, FP, FLAGS)
 * - Direct bytecode execution without stack translation
 * - Integrated garbage collection support
 * - Comprehensive debugging and profiling support
 * 
 * @author Orus Development Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef ORUS_REGISTER_VM_H
#define ORUS_REGISTER_VM_H

#include "common.h"
#include "value.h"
#include "memory.h"
#include "error.h"
#include "register_chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// ARCHITECTURE CONSTANTS
// =============================================================================

/** Number of general-purpose registers */
#define REGISTER_COUNT 32

/** Special register indices (beyond general-purpose registers) */
#define REG_SP  32  /**< Stack Pointer */
#define REG_FP  33  /**< Frame Pointer */
#define REG_FLAGS 34 /**< Status Flags Register */

/** Total number of registers including special registers */
#define TOTAL_REGISTER_COUNT 35

/** Maximum call stack depth */
#define MAX_CALL_STACK_DEPTH 256

/** Maximum exception handler nesting */
#define MAX_EXCEPTION_HANDLERS 64

// =============================================================================
// INSTRUCTION FORMAT
// =============================================================================

/**
 * @brief 32-bit instruction format
 * 
 * Instructions follow a consistent 4-byte format:
 * +--------+--------+--------+--------+
 * | OPCODE | DST    | SRC1   | SRC2   |
 * +--------+--------+--------+--------+
 *   8 bits   8 bits   8 bits   8 bits
 */
typedef union {
    uint32_t raw;
    struct {
        uint8_t opcode;  /**< Operation code */
        uint8_t dst;     /**< Destination register */
        uint8_t src1;    /**< First source register */
        uint8_t src2;    /**< Second source register */
    } fields;
} Instruction;

/**
 * @brief Instruction with immediate value
 * 
 * For instructions that require immediate values:
 * +--------+--------+----------------+
 * | OPCODE | DST    | IMMEDIATE      |
 * +--------+--------+----------------+
 *   8 bits   8 bits    16 bits
 */
typedef union {
    uint32_t raw;
    struct {
        uint8_t opcode;   /**< Operation code */
        uint8_t dst;      /**< Destination register */
        uint16_t imm;     /**< Immediate value */
    } imm_fields;
} ImmediateInstruction;

// =============================================================================
// STATUS FLAGS
// =============================================================================

/**
 * @brief CPU status flags
 * 
 * These flags are set by comparison and arithmetic operations
 * and tested by conditional branches.
 */
typedef enum {
    FLAG_ZERO     = 0x01,  /**< Zero flag - set when result is zero */
    FLAG_NEGATIVE = 0x02,  /**< Negative flag - set when result is negative */
    FLAG_CARRY    = 0x04,  /**< Carry flag - set on arithmetic overflow */
    FLAG_OVERFLOW = 0x08,  /**< Overflow flag - set on signed overflow */
    FLAG_ERROR    = 0x10,  /**< Error flag - set when runtime error occurs */
} StatusFlags;

// =============================================================================
// CALL FRAME
// =============================================================================

/**
 * @brief Function call frame
 * 
 * Represents a single function call on the call stack.
 * Tracks return address, register state, and local variable scope.
 */
typedef struct CallFrame {
    uint32_t return_address;     /**< Return instruction pointer */
    uint8_t register_base;       /**< Base register for local variables */
    uint8_t register_count;      /**< Number of registers used by this frame */
    Value* locals;               /**< Local variable storage */
    uint16_t local_count;        /**< Number of local variables */
    struct CallFrame* previous;  /**< Previous frame in call stack */
} CallFrame;

// =============================================================================
// EXCEPTION HANDLING
// =============================================================================

/**
 * @brief Exception handler frame
 * 
 * Represents a try-catch block for exception handling.
 */
typedef struct ExceptionHandler {
    uint32_t try_start;          /**< Start of try block */
    uint32_t try_end;            /**< End of try block */
    uint32_t catch_address;      /**< Address of catch handler */
    uint8_t catch_register;      /**< Register to store exception in */
    struct ExceptionHandler* previous; /**< Previous handler in stack */
} ExceptionHandler;

// =============================================================================
// BYTECODE CHUNK
// =============================================================================

// RegisterChunk is defined in register_chunk.h

// =============================================================================
// PERFORMANCE COUNTERS
// =============================================================================

/**
 * @brief Performance monitoring counters
 * 
 * Tracks VM performance metrics for profiling and optimization.
 */
typedef struct {
    uint64_t instructions_executed;  /**< Total instructions executed */
    uint64_t function_calls;         /**< Number of function calls */
    uint64_t memory_allocations;     /**< Number of memory allocations */
    uint64_t gc_collections;         /**< Garbage collection cycles */
    uint64_t cache_hits;             /**< Cache hits (if caching enabled) */
    uint64_t cache_misses;           /**< Cache misses */
    
    // Timing information (in nanoseconds)
    uint64_t execution_time;         /**< Total execution time */
    uint64_t gc_time;                /**< Time spent in garbage collection */
    uint64_t compilation_time;       /**< Time spent compiling */
} PerformanceCounters;

// =============================================================================
// REGISTER VM STATE
// =============================================================================

/**
 * @brief Register Virtual Machine state
 * 
 * Central VM state containing all registers, execution context,
 * and runtime state information.
 */
typedef struct RegisterVM {
    // Register file
    Value registers[TOTAL_REGISTER_COUNT]; /**< All VM registers */
    
    // Execution state
    uint32_t ip;                     /**< Instruction pointer */
    uint8_t flags;                   /**< Status flags register */
    bool running;                    /**< VM execution state */
    
    // Memory and objects
    RegisterChunk* chunk;            /**< Current executing chunk */
    Obj* objects;                    /**< Linked list of all allocated objects */
    
    // Call stack
    CallFrame* current_frame;        /**< Current function call frame */
    CallFrame call_stack[MAX_CALL_STACK_DEPTH]; /**< Call stack storage */
    uint16_t call_depth;             /**< Current call stack depth */
    
    // Exception handling
    ExceptionHandler* current_handler; /**< Current exception handler */
    ExceptionHandler exception_stack[MAX_EXCEPTION_HANDLERS]; /**< Exception stack */
    uint16_t exception_depth;        /**< Current exception nesting */
    Value current_exception;         /**< Current exception being handled */
    
    // Memory management
    size_t bytes_allocated;          /**< Total bytes allocated */
    size_t next_gc;                  /**< Threshold for next GC */
    bool gc_running;                 /**< GC execution state */
    
    // Performance monitoring
    PerformanceCounters* perf;       /**< Performance counters (NULL if disabled) */
    
    // Debug support
    bool debug_mode;                 /**< Debug mode enabled */
    bool trace_execution;            /**< Trace instruction execution */
    bool trace_memory;               /**< Trace memory operations */
    
    // Module system
    struct RegisterVM** loaded_modules; /**< Array of loaded modules */
    uint16_t module_count;           /**< Number of loaded modules */
    uint16_t module_capacity;        /**< Module array capacity */
    
    // Error state
    Value last_error;                /**< Last error that occurred */
    bool has_error;                  /**< Error state flag */
} RegisterVM;

// =============================================================================
// VM LIFECYCLE FUNCTIONS
// =============================================================================

/**
 * @brief Initialize a new register VM instance
 * 
 * @param vm Pointer to VM structure to initialize
 * @param chunk Bytecode chunk to execute (can be NULL)
 * @return true on success, false on failure
 */
bool registervm_init(RegisterVM* vm, RegisterChunk* chunk);

/**
 * @brief Free all resources associated with a register VM
 * 
 * @param vm Pointer to VM to free
 */
void registervm_free(RegisterVM* vm);

/**
 * @brief Reset VM state for new execution
 * 
 * @param vm Pointer to VM to reset
 * @param chunk New bytecode chunk (can be NULL to keep current)
 */
void registervm_reset(RegisterVM* vm, RegisterChunk* chunk);

// =============================================================================
// EXECUTION FUNCTIONS
// =============================================================================

/**
 * @brief Execute bytecode in the register VM
 * 
 * @param vm Pointer to VM instance
 * @return Execution result code
 */
typedef enum {
    EXEC_OK,           /**< Execution completed successfully */
    EXEC_ERROR,        /**< Runtime error occurred */
    EXEC_EXCEPTION,    /**< Unhandled exception */
    EXEC_STACK_OVERFLOW, /**< Call stack overflow */
    EXEC_OUT_OF_MEMORY,  /**< Out of memory */
    EXEC_INVALID_OPCODE, /**< Invalid instruction opcode */
} ExecutionResult;

ExecutionResult registervm_execute(RegisterVM* vm);

/**
 * @brief Execute a single instruction (for debugging/stepping)
 * 
 * @param vm Pointer to VM instance
 * @return Execution result code
 */
ExecutionResult registervm_step(RegisterVM* vm);

/**
 * @brief Execute until next function call or return
 * 
 * @param vm Pointer to VM instance
 * @return Execution result code
 */
ExecutionResult registervm_step_over(RegisterVM* vm);

// =============================================================================
// REGISTER ACCESS FUNCTIONS
// =============================================================================

/**
 * @brief Get value from a register
 * 
 * @param vm Pointer to VM instance
 * @param reg Register index
 * @return Register value
 */
static inline Value registervm_get_register(const RegisterVM* vm, uint8_t reg) {
    if (reg >= TOTAL_REGISTER_COUNT) {
        return NIL_VAL;
    }
    return vm->registers[reg];
}

/**
 * @brief Set value in a register
 * 
 * @param vm Pointer to VM instance
 * @param reg Register index
 * @param value Value to set
 * @return true on success, false on invalid register
 */
static inline bool registervm_set_register(RegisterVM* vm, uint8_t reg, Value value) {
    if (reg >= TOTAL_REGISTER_COUNT) {
        return false;
    }
    vm->registers[reg] = value;
    return true;
}

/**
 * @brief Get status flags
 * 
 * @param vm Pointer to VM instance
 * @return Current status flags
 */
static inline uint8_t registervm_get_flags(const RegisterVM* vm) {
    return vm->flags;
}

/**
 * @brief Set status flags
 * 
 * @param vm Pointer to VM instance
 * @param flags Flags to set
 */
static inline void registervm_set_flags(RegisterVM* vm, uint8_t flags) {
    vm->flags = flags;
}

// =============================================================================
// MEMORY MANAGEMENT INTEGRATION
// =============================================================================

/**
 * @brief Trigger garbage collection
 * 
 * @param vm Pointer to VM instance
 */
void registervm_gc_collect(RegisterVM* vm);

/**
 * @brief Mark VM roots for garbage collection
 * 
 * @param vm Pointer to VM instance
 */
void registervm_gc_mark_roots(RegisterVM* vm);

// =============================================================================
// DEBUG AND PROFILING
// =============================================================================

/**
 * @brief Enable performance monitoring
 * 
 * @param vm Pointer to VM instance
 * @return true on success, false on failure
 */
bool registervm_enable_profiling(RegisterVM* vm);

/**
 * @brief Disable performance monitoring
 * 
 * @param vm Pointer to VM instance
 */
void registervm_disable_profiling(RegisterVM* vm);

/**
 * @brief Get performance counters
 * 
 * @param vm Pointer to VM instance
 * @return Pointer to performance counters (NULL if disabled)
 */
const PerformanceCounters* registervm_get_performance(const RegisterVM* vm);

/**
 * @brief Print VM state for debugging
 * 
 * @param vm Pointer to VM instance
 * @param include_registers Whether to include register values
 */
void registervm_debug_print_state(const RegisterVM* vm, bool include_registers);

/**
 * @brief Set debug trace options
 * 
 * @param vm Pointer to VM instance
 * @param trace_execution Trace instruction execution
 * @param trace_memory Trace memory operations
 */
void registervm_set_debug_options(RegisterVM* vm, bool trace_execution, bool trace_memory);

// =============================================================================
// ERROR HANDLING
// =============================================================================

/**
 * @brief Get last error from VM
 * 
 * @param vm Pointer to VM instance
 * @return Last error value (NIL_VAL if no error)
 */
Value registervm_get_last_error(const RegisterVM* vm);

/**
 * @brief Clear error state
 * 
 * @param vm Pointer to VM instance
 */
void registervm_clear_error(RegisterVM* vm);

/**
 * @brief Set runtime error
 * 
 * @param vm Pointer to VM instance
 * @param error Error value
 */
void registervm_set_error(RegisterVM* vm, Value error);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Check if VM is in valid state
 * 
 * @param vm Pointer to VM instance
 * @return true if valid, false otherwise
 */
bool registervm_is_valid(const RegisterVM* vm);

/**
 * @brief Get VM version information
 * 
 * @return Version string
 */
const char* registervm_get_version(void);

/**
 * @brief Get VM build information
 * 
 * @return Build information string
 */
const char* registervm_get_build_info(void);

#ifdef __cplusplus
}
#endif

#endif // ORUS_REGISTER_VM_H