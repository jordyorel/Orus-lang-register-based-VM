/**
 * @file register_vm.c
 * @brief Orus Register Virtual Machine Implementation
 * 
 * This file implements the core register-based virtual machine for the Orus
 * programming language. The VM provides efficient execution of bytecode
 * instructions using a register-based architecture.
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

#include "../../include/register_vm.h"
#include "../../include/register_opcodes.h"
#include "../../include/register_chunk.h"
#include "../../include/memory.h"
#include "../../include/value.h"

// =============================================================================
// PRIVATE CONSTANTS
// =============================================================================

/** Initial call stack capacity */
#define INITIAL_CALL_STACK_SIZE 64

/** Initial exception stack capacity */
#define INITIAL_EXCEPTION_STACK_SIZE 16

/** GC threshold growth factor */
#define GC_HEAP_GROW_FACTOR 2

/** Default next GC threshold */
#define DEFAULT_GC_THRESHOLD (1024 * 1024) // 1MB

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static ExecutionResult execute_instruction(RegisterVM* vm, uint32_t instruction);
static bool check_register_bounds(uint8_t reg);
static void update_flags_arithmetic(RegisterVM* vm, Value result);
static void update_flags_comparison(RegisterVM* vm, int comparison_result);
static Value perform_arithmetic_operation(RegisterOpcode op, Value a, Value b, bool* error);
static void trace_instruction(const RegisterVM* vm, uint32_t instruction);

// =============================================================================
// VM LIFECYCLE FUNCTIONS
// =============================================================================

bool registervm_init(RegisterVM* vm, RegisterChunk* chunk) {
    if (!vm) {
        return false;
    }
    
    // Initialize all fields to zero/NULL
    memset(vm, 0, sizeof(RegisterVM));
    
    // Initialize registers with NIL values
    for (int i = 0; i < TOTAL_REGISTER_COUNT; i++) {
        vm->registers[i] = NIL_VAL;
    }
    
    // Set initial execution state
    vm->ip = 0;
    vm->flags = 0;
    vm->running = false;
    vm->chunk = chunk;
    vm->objects = NULL;
    
    // Initialize call stack
    vm->current_frame = NULL;
    vm->call_depth = 0;
    
    // Initialize exception handling
    vm->current_handler = NULL;
    vm->exception_depth = 0;
    vm->current_exception = NIL_VAL;
    
    // Initialize memory management
    vm->bytes_allocated = 0;
    vm->next_gc = DEFAULT_GC_THRESHOLD;
    vm->gc_running = false;
    
    // Initialize performance monitoring (disabled by default)
    vm->perf = NULL;
    
    // Initialize debug settings
    vm->debug_mode = false;
    vm->trace_execution = false;
    vm->trace_memory = false;
    
    // Initialize module system
    vm->loaded_modules = NULL;
    vm->module_count = 0;
    vm->module_capacity = 0;
    
    // Initialize error state
    vm->last_error = NIL_VAL;
    vm->has_error = false;
    
    return true;
}

void registervm_free(RegisterVM* vm) {
    if (!vm) {
        return;
    }
    
    // Free performance monitoring if enabled
    if (vm->perf) {
        free(vm->perf);
        vm->perf = NULL;
    }
    
    // Free loaded modules array
    if (vm->loaded_modules) {
        free(vm->loaded_modules);
        vm->loaded_modules = NULL;
    }
    
    // Free all objects in the heap
    freeObjects();
    
    // Clear all state
    memset(vm, 0, sizeof(RegisterVM));
}

void registervm_reset(RegisterVM* vm, RegisterChunk* chunk) {
    if (!vm) {
        return;
    }
    
    // Reset execution state
    vm->ip = 0;
    vm->flags = 0;
    vm->running = false;
    vm->chunk = chunk;
    
    // Reset call stack
    vm->current_frame = NULL;
    vm->call_depth = 0;
    
    // Reset exception handling
    vm->current_handler = NULL;
    vm->exception_depth = 0;
    vm->current_exception = NIL_VAL;
    
    // Reset error state
    vm->last_error = NIL_VAL;
    vm->has_error = false;
    
    // Clear registers (keep NIL values)
    for (int i = 0; i < REGISTER_COUNT; i++) {
        vm->registers[i] = NIL_VAL;
    }
    
    // Reset performance counters if enabled
    if (vm->perf) {
        memset(vm->perf, 0, sizeof(PerformanceCounters));
    }
}

// =============================================================================
// EXECUTION FUNCTIONS
// =============================================================================

ExecutionResult registervm_execute(RegisterVM* vm) {
    if (!vm || !vm->chunk) {
        return EXEC_ERROR;
    }
    
    vm->running = true;
    ExecutionResult result = EXEC_OK;
    
    while (vm->running && vm->ip < vm->chunk->code_count) {
        // Check for errors
        if (vm->has_error) {
            result = EXEC_ERROR;
            break;
        }
        
        // Get next instruction
        uint32_t instruction = vm->chunk->code[vm->ip];
        
        // Trace execution if enabled
        if (vm->trace_execution) {
            trace_instruction(vm, instruction);
        }
        
        // Update performance counters
        if (vm->perf) {
            vm->perf->instructions_executed++;
        }
        
        // Execute instruction
        result = execute_instruction(vm, instruction);
        
        if (result != EXEC_OK) {
            break;
        }
        
        // Check for garbage collection
        if (vm->bytes_allocated > vm->next_gc && !vm->gc_running) {
            registervm_gc_collect(vm);
        }
    }
    
    vm->running = false;
    return result;
}

ExecutionResult registervm_step(RegisterVM* vm) {
    if (!vm || !vm->chunk || vm->ip >= vm->chunk->code_count) {
        return EXEC_ERROR;
    }
    
    // Get next instruction
    uint32_t instruction = vm->chunk->code[vm->ip];
    
    // Trace execution if enabled
    if (vm->trace_execution) {
        trace_instruction(vm, instruction);
    }
    
    // Update performance counters
    if (vm->perf) {
        vm->perf->instructions_executed++;
    }
    
    // Execute single instruction
    return execute_instruction(vm, instruction);
}

ExecutionResult registervm_step_over(RegisterVM* vm) {
    if (!vm || !vm->chunk) {
        return EXEC_ERROR;
    }
    
    uint32_t initial_call_depth = vm->call_depth;
    ExecutionResult result;
    
    do {
        result = registervm_step(vm);
        if (result != EXEC_OK) {
            break;
        }
    } while (vm->call_depth > initial_call_depth && vm->running);
    
    return result;
}

// =============================================================================
// CORE INSTRUCTION EXECUTION
// =============================================================================

static ExecutionResult execute_instruction(RegisterVM* vm, uint32_t instruction) {
    RegisterOpcode opcode = (RegisterOpcode)GET_OPCODE(instruction);
    uint8_t dst = GET_DST(instruction);
    uint8_t src1 = GET_SRC1(instruction);
    uint8_t src2 = GET_SRC2(instruction);
    uint16_t imm = GET_IMM(instruction);
    
    // Advance instruction pointer (may be overridden by control flow)
    vm->ip++;
    
    switch (opcode) {
        // =================================================================
        // CONTROL FLOW
        // =================================================================
        
        case ROP_NOP:
            // No operation
            break;
            
        case ROP_HALT:
            vm->running = false;
            break;
            
        case ROP_JMP:
            vm->ip = imm;
            if (vm->ip >= vm->chunk->code_count) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME, 
                    "Jump target out of bounds", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            break;
            
        case ROP_JMP_REG:
            if (!check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for jump", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            if (!IS_I32(vm->registers[src1])) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Jump target must be integer", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            vm->ip = AS_I32(vm->registers[src1]);
            if (vm->ip >= vm->chunk->code_count) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Jump target out of bounds", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            break;
            
        case ROP_JZ:
            if (!check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for conditional jump", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            if (IS_BOOL(vm->registers[src1]) && !AS_BOOL(vm->registers[src1])) {
                vm->ip = imm;
            } else if (IS_I32(vm->registers[src1]) && AS_I32(vm->registers[src1]) == 0) {
                vm->ip = imm;
            } else if (IS_NIL(vm->registers[src1])) {
                vm->ip = imm;
            }
            break;
            
        case ROP_JNZ:
            if (!check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for conditional jump", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            if (IS_BOOL(vm->registers[src1]) && AS_BOOL(vm->registers[src1])) {
                vm->ip = imm;
            } else if (IS_I32(vm->registers[src1]) && AS_I32(vm->registers[src1]) != 0) {
                vm->ip = imm;
            } else if (!IS_NIL(vm->registers[src1]) && !IS_BOOL(vm->registers[src1])) {
                vm->ip = imm;
            }
            break;
            
        // =================================================================
        // DATA MOVEMENT
        // =================================================================
        
        case ROP_MOVE:
            if (!check_register_bounds(dst) || !check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for move", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            vm->registers[dst] = vm->registers[src1];
            break;
            
        case ROP_LOAD_IMM:
            if (!check_register_bounds(dst)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid destination register", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            // Load 16-bit immediate as i32
            vm->registers[dst] = I32_VAL((int32_t)imm);
            break;
            
        case ROP_LOAD_CONST:
            if (!check_register_bounds(dst)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid destination register", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            if (imm >= vm->chunk->constant_count) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Constant index out of bounds", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            vm->registers[dst] = vm->chunk->constants[imm];
            break;
            
        case ROP_LOAD_GLOBAL:
            if (!check_register_bounds(dst)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid destination register", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            if (imm >= vm->chunk->global_count) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Global variable index out of bounds", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            vm->registers[dst] = vm->chunk->globals[imm];
            break;
            
        case ROP_STORE_GLOBAL:
            if (!check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid source register", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            if (imm >= vm->chunk->global_count) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Global variable index out of bounds", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            vm->chunk->globals[imm] = vm->registers[src1];
            break;
            
        // =================================================================
        // ARITHMETIC OPERATIONS
        // =================================================================
        
        case ROP_ADD_I32:
        case ROP_SUB_I32:
        case ROP_MUL_I32:
        case ROP_DIV_I32:
        case ROP_MOD_I32:
        case ROP_ADD_I64:
        case ROP_SUB_I64:
        case ROP_MUL_I64:
        case ROP_DIV_I64:
        case ROP_MOD_I64:
        case ROP_ADD_F64:
        case ROP_SUB_F64:
        case ROP_MUL_F64:
        case ROP_DIV_F64: {
            if (!check_register_bounds(dst) || !check_register_bounds(src1) || !check_register_bounds(src2)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for arithmetic", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            
            bool error = false;
            Value result = perform_arithmetic_operation(opcode, vm->registers[src1], vm->registers[src2], &error);
            
            if (error) {
                return EXEC_ERROR;
            }
            
            vm->registers[dst] = result;
            update_flags_arithmetic(vm, result);
            break;
        }
        
        case ROP_NEG_I32:
            if (!check_register_bounds(dst) || !check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for negation", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            if (!IS_I32(vm->registers[src1])) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Cannot negate non-integer value", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            vm->registers[dst] = I32_VAL(-AS_I32(vm->registers[src1]));
            update_flags_arithmetic(vm, vm->registers[dst]);
            break;
            
        // =================================================================
        // COMPARISON OPERATIONS
        // =================================================================
        
        case ROP_CMP_I32:
        case ROP_CMP_I64:
        case ROP_CMP_F64:
            if (!check_register_bounds(src1) || !check_register_bounds(src2)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for comparison", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            
            // Perform comparison and set flags
            if (valuesEqual(vm->registers[src1], vm->registers[src2])) {
                vm->flags |= FLAG_ZERO;
                update_flags_comparison(vm, 0);
            } else {
                vm->flags &= ~FLAG_ZERO;
                // Set other flags based on comparison result
                // This is a simplified implementation
                update_flags_comparison(vm, IS_I32(vm->registers[src1]) ? 
                    AS_I32(vm->registers[src1]) - AS_I32(vm->registers[src2]) : -1);
            }
            break;
            
        case ROP_EQ_I32:
        case ROP_EQ_STR:
        case ROP_EQ_OBJ: {
            if (!check_register_bounds(dst) || !check_register_bounds(src1) || !check_register_bounds(src2)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for equality", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            
            bool equal = valuesEqual(vm->registers[src1], vm->registers[src2]);
            vm->registers[dst] = BOOL_VAL(equal);
            break;
        }
        
        // =================================================================
        // TYPE OPERATIONS
        // =================================================================
        
        case ROP_TYPE_OF:
            if (!check_register_bounds(dst) || !check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for type operation", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            
            // Create string representation of type
            const char* type_name = "unknown";
            switch (vm->registers[src1].type) {
                case VAL_I32: type_name = "i32"; break;
                case VAL_I64: type_name = "i64"; break;
                case VAL_U32: type_name = "u32"; break;
                case VAL_U64: type_name = "u64"; break;
                case VAL_F64: type_name = "f64"; break;
                case VAL_BOOL: type_name = "bool"; break;
                case VAL_NIL: type_name = "nil"; break;
                case VAL_STRING: type_name = "string"; break;
                case VAL_ARRAY: type_name = "array"; break;
                case VAL_ERROR: type_name = "error"; break;
                case VAL_ENUM: type_name = "enum"; break;
                default: type_name = "unknown"; break;
            }
            
            vm->registers[dst] = STRING_VAL(allocateString(type_name, strlen(type_name)));
            break;
            
        // =================================================================
        // BUILT-IN FUNCTIONS
        // =================================================================
        
        case ROP_PRINT:
            if (!check_register_bounds(src1)) {
                registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                    "Invalid register for print", (SrcLocation){0, 0, 0})));
                return EXEC_ERROR;
            }
            printValue(vm->registers[src1]);
            printf("\n");
            break;
            
        // =================================================================
        // DEFAULT CASE
        // =================================================================
        
        default:
            registervm_set_error(vm, ERROR_VAL(allocateError(ERROR_RUNTIME,
                "Unknown opcode", (SrcLocation){0, 0, 0})));
            return EXEC_INVALID_OPCODE;
    }
    
    return EXEC_OK;
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

static bool check_register_bounds(uint8_t reg) {
    return reg < TOTAL_REGISTER_COUNT;
}

static void update_flags_arithmetic(RegisterVM* vm, Value result) {
    vm->flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
    
    if (IS_I32(result) && AS_I32(result) == 0) {
        vm->flags |= FLAG_ZERO;
    } else if (IS_I32(result) && AS_I32(result) < 0) {
        vm->flags |= FLAG_NEGATIVE;
    }
}

static void update_flags_comparison(RegisterVM* vm, int comparison_result) {
    vm->flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
    
    if (comparison_result == 0) {
        vm->flags |= FLAG_ZERO;
    } else if (comparison_result < 0) {
        vm->flags |= FLAG_NEGATIVE;
    }
}

static Value perform_arithmetic_operation(RegisterOpcode op, Value a, Value b, bool* error) {
    *error = false;
    
    switch (op) {
        case ROP_ADD_I32:
            if (!IS_I32(a) || !IS_I32(b)) {
                *error = true;
                return NIL_VAL;
            }
            return I32_VAL(AS_I32(a) + AS_I32(b));
            
        case ROP_SUB_I32:
            if (!IS_I32(a) || !IS_I32(b)) {
                *error = true;
                return NIL_VAL;
            }
            return I32_VAL(AS_I32(a) - AS_I32(b));
            
        case ROP_MUL_I32:
            if (!IS_I32(a) || !IS_I32(b)) {
                *error = true;
                return NIL_VAL;
            }
            return I32_VAL(AS_I32(a) * AS_I32(b));
            
        case ROP_DIV_I32:
            if (!IS_I32(a) || !IS_I32(b)) {
                *error = true;
                return NIL_VAL;
            }
            if (AS_I32(b) == 0) {
                *error = true;
                return NIL_VAL;
            }
            return I32_VAL(AS_I32(a) / AS_I32(b));
            
        case ROP_ADD_F64:
            if (!IS_F64(a) || !IS_F64(b)) {
                *error = true;
                return NIL_VAL;
            }
            return F64_VAL(AS_F64(a) + AS_F64(b));
            
        case ROP_SUB_F64:
            if (!IS_F64(a) || !IS_F64(b)) {
                *error = true;
                return NIL_VAL;
            }
            return F64_VAL(AS_F64(a) - AS_F64(b));
            
        case ROP_MUL_F64:
            if (!IS_F64(a) || !IS_F64(b)) {
                *error = true;
                return NIL_VAL;
            }
            return F64_VAL(AS_F64(a) * AS_F64(b));
            
        case ROP_DIV_F64:
            if (!IS_F64(a) || !IS_F64(b)) {
                *error = true;
                return NIL_VAL;
            }
            if (AS_F64(b) == 0.0) {
                *error = true;
                return NIL_VAL;
            }
            return F64_VAL(AS_F64(a) / AS_F64(b));
            
        default:
            *error = true;
            return NIL_VAL;
    }
}

static void trace_instruction(const RegisterVM* vm, uint32_t instruction) {
    RegisterOpcode opcode = (RegisterOpcode)GET_OPCODE(instruction);
    uint8_t dst = GET_DST(instruction);
    uint8_t src1 = GET_SRC1(instruction);
    uint8_t src2 = GET_SRC2(instruction);
    
    printf("[%04X] ROP_%02X R%d R%d R%d\n", vm->ip, opcode, dst, src1, src2);
}

// =============================================================================
// REGISTER ACCESS FUNCTIONS (already implemented as inline in header)
// =============================================================================

// =============================================================================
// MEMORY MANAGEMENT INTEGRATION
// =============================================================================

void registervm_gc_collect(RegisterVM* vm) {
    if (!vm || vm->gc_running) {
        return;
    }
    
    vm->gc_running = true;
    
    size_t before = vm->bytes_allocated;
    
    // Mark all reachable objects
    registervm_gc_mark_roots(vm);
    
    // Collect garbage
    collectGarbage();
    
    // Update GC threshold
    vm->next_gc = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;
    
    if (vm->perf) {
        vm->perf->gc_collections++;
    }
    
    vm->gc_running = false;
    
    if (vm->trace_memory) {
        printf("GC: collected %zu bytes (%zu -> %zu)\n", 
               before - vm->bytes_allocated, before, vm->bytes_allocated);
    }
}

void registervm_gc_mark_roots(RegisterVM* vm) {
    if (!vm) {
        return;
    }
    
    // Mark all register values
    for (int i = 0; i < TOTAL_REGISTER_COUNT; i++) {
        markValue(vm->registers[i]);
    }
    
    // Mark current exception
    markValue(vm->current_exception);
    
    // Mark last error
    markValue(vm->last_error);
    
    // Mark globals in chunk
    if (vm->chunk && vm->chunk->globals) {
        for (uint16_t i = 0; i < vm->chunk->global_count; i++) {
            markValue(vm->chunk->globals[i]);
        }
    }
    
    // Mark constants in chunk
    if (vm->chunk && vm->chunk->constants) {
        for (uint32_t i = 0; i < vm->chunk->constant_count; i++) {
            markValue(vm->chunk->constants[i]);
        }
    }
    
    // Mark call frame locals
    CallFrame* frame = vm->current_frame;
    while (frame) {
        if (frame->locals) {
            for (uint16_t i = 0; i < frame->local_count; i++) {
                markValue(frame->locals[i]);
            }
        }
        frame = frame->previous;
    }
}

// =============================================================================
// ERROR HANDLING
// =============================================================================

Value registervm_get_last_error(const RegisterVM* vm) {
    return vm ? vm->last_error : NIL_VAL;
}

void registervm_clear_error(RegisterVM* vm) {
    if (vm) {
        vm->last_error = NIL_VAL;
        vm->has_error = false;
    }
}

void registervm_set_error(RegisterVM* vm, Value error) {
    if (vm) {
        vm->last_error = error;
        vm->has_error = true;
    }
}

// =============================================================================
// DEBUG AND PROFILING
// =============================================================================

bool registervm_enable_profiling(RegisterVM* vm) {
    if (!vm) {
        return false;
    }
    
    if (!vm->perf) {
        vm->perf = malloc(sizeof(PerformanceCounters));
        if (!vm->perf) {
            return false;
        }
    }
    
    memset(vm->perf, 0, sizeof(PerformanceCounters));
    return true;
}

void registervm_disable_profiling(RegisterVM* vm) {
    if (vm && vm->perf) {
        free(vm->perf);
        vm->perf = NULL;
    }
}

const PerformanceCounters* registervm_get_performance(const RegisterVM* vm) {
    return vm ? vm->perf : NULL;
}

void registervm_debug_print_state(const RegisterVM* vm, bool include_registers) {
    if (!vm) {
        printf("VM: NULL\n");
        return;
    }
    
    printf("=== Register VM State ===\n");
    printf("IP: %04X\n", vm->ip);
    printf("Flags: %02X\n", vm->flags);
    printf("Running: %s\n", vm->running ? "true" : "false");
    printf("Call Depth: %d\n", vm->call_depth);
    printf("Exception Depth: %d\n", vm->exception_depth);
    printf("Has Error: %s\n", vm->has_error ? "true" : "false");
    
    if (include_registers) {
        printf("\n=== Registers ===\n");
        for (int i = 0; i < REGISTER_COUNT; i++) {
            printf("R%02d: ", i);
            printValue(vm->registers[i]);
            printf("\n");
        }
        
        printf("\n=== Special Registers ===\n");
        printf("SP:    ");
        printValue(vm->registers[REG_SP]);
        printf("\n");
        printf("FP:    ");
        printValue(vm->registers[REG_FP]);
        printf("\n");
        printf("FLAGS: %02X\n", vm->flags);
    }
    
    printf("========================\n");
}

void registervm_set_debug_options(RegisterVM* vm, bool trace_execution, bool trace_memory) {
    if (vm) {
        vm->trace_execution = trace_execution;
        vm->trace_memory = trace_memory;
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

bool registervm_is_valid(const RegisterVM* vm) {
    if (!vm) {
        return false;
    }
    
    // Check basic validity
    if (vm->call_depth > MAX_CALL_STACK_DEPTH) {
        return false;
    }
    
    if (vm->exception_depth > MAX_EXCEPTION_HANDLERS) {
        return false;
    }
    
    // Check chunk validity if present
    if (vm->chunk && vm->ip >= vm->chunk->code_count) {
        return false;
    }
    
    return true;
}

const char* registervm_get_version(void) {
    return "Orus Register VM 1.0.0";
}

const char* registervm_get_build_info(void) {
    return "Built on " __DATE__ " at " __TIME__;
}