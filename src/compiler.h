#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "chunk.h"
#include "symtable.h"
#include "type.h"

typedef struct {
    SymbolTable* symbols;
    Chunk* chunk;
    bool hadError;
    bool panicMode;
} Compiler;

void initCompiler(Compiler* compiler, Chunk* chunk);
bool compile(ASTNode* ast, Compiler* compiler);
uint8_t resolveVariable(Compiler* compiler, Token name);       // Added
uint8_t addLocal(Compiler* compiler, Token name, Type* type);  // Added
uint8_t defineVariable(Compiler* compiler, Token name, Type* type);  // Added

#endif