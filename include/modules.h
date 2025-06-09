#ifndef MODULES_H
#define MODULES_H

#include "chunk.h"
#include "ast.h"

typedef struct {
    char* module_name;
    Chunk* bytecode;
} Module;

char* load_module_source(const char* resolved_path);
ASTNode* parse_module_source(const char* source_code, const char* module_name);
Chunk* compile_module_ast(ASTNode* ast, const char* module_name);
bool register_module(Module* module);
Module* get_module(const char* name);

#endif
