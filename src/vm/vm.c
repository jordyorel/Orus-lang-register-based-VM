#include "../include/vm.h"
#include "../include/memory.h"
#include "../include/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global VM instance
VM vm;

// Module system globals
bool traceImports = false;

// =============================================================================
// VM LIFECYCLE FUNCTIONS
// =============================================================================

void initVM(void) {
    memset(&vm, 0, sizeof(VM));
    vm.initialized = true;
    vm.useRegisterVM = false;
    vm.filePath = NULL;
    vm.loadedModules = NULL;
    vm.moduleCount = 0;
    vm.astRoot = NULL;
    vm.lastError = NIL_VAL;
    vm.trace = false;
    vm.devMode = false;
    vm.stdPath = NULL;
    
    // Initialize stack VM components
    vm.stack = NULL;
    vm.stackTop = NULL;
}

void freeVM(void) {
    if (vm.initialized) {
        freeRegisterChunk(&vm.regChunk);
        freeRegisterVM(&vm.regVM);
        memset(&vm, 0, sizeof(VM));
    }
}

// =============================================================================
// REGISTER VM WRAPPER FUNCTIONS
// =============================================================================

void initRegisterChunk(RegisterChunk* chunk) {
    if (!chunk) return;
    register_chunk_init(chunk, "main");
}

void freeRegisterChunk(RegisterChunk* chunk) {
    if (!chunk) return;
    register_chunk_free(chunk);
}

void initRegisterVM(RegisterVM* vm, RegisterChunk* chunk) {
    if (!vm || !chunk) return;
    registervm_init(vm, chunk);
}

void freeRegisterVM(RegisterVM* vm) {
    if (!vm) return;
    registervm_free(vm);
}

void runRegisterVM(RegisterVM* vm) {
    if (!vm) return;
    registervm_execute(vm);
}

void disassembleRegisterChunk(RegisterChunk* chunk, const char* name) {
    if (!chunk || !name) return;
    register_chunk_disassemble(chunk, true);
}
