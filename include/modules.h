#ifndef MODULES_H
#define MODULES_H

#include "chunk.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include <stdbool.h>

typedef struct {
    char* name;
    Value value;
    uint8_t index; // Global variable index
} Export;

typedef struct {
    char* module_name;
    Chunk* bytecode;
    Export exports[UINT8_COUNT];
    uint8_t export_count;
    bool executed;
} Module;

Export* get_export(Module* module, const char* name);

char* load_module_source(const char* resolved_path);
ASTNode* parse_module_source(const char* source_code, const char* module_name);
Chunk* compile_module_ast(ASTNode* ast, const char* module_name);
bool register_module(Module* module);
Module* get_module(const char* name);
InterpretResult compile_module_only(const char* path);

// Ensure a module is loaded and executed. Returns INTERPRET_OK on success.
InterpretResult interpret_module(const char* path);

#endif
