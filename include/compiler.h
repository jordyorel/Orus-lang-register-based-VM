#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "chunk.h"
#include "symtable.h"
#include "type.h"
#include "value.h"

typedef struct {
    int loopStart;         // Start position of the current loop
    int loopEnd;           // End position of the current loop (for breaks)
    int loopContinue;      // Position to jump to for continue statements
    int loopDepth;         // Nesting level of loops

    // Arrays to store break and continue jumps for patching
    ObjIntArray* breakJumps;       // GC-managed array of break jump positions
    int breakJumpCount;            // Number of break jumps
    int breakJumpCapacity;         // Capacity of the breakJumps array

    ObjIntArray* continueJumps;    // GC-managed array of continue jump positions
    int continueJumpCount;         // Number of continue jumps
    int continueJumpCapacity;      // Capacity of the continueJumps array

    SymbolTable symbols;
    int scopeDepth;
    Chunk* chunk;
    bool hadError;
    bool panicMode;

    // File and source information used for diagnostics
    const char* filePath;
    const char* sourceCode;
    const char** lineStarts;
    int lineCount;

    // Line/column of the AST node currently being compiled
    int currentLine;
    int currentColumn;
    Type* currentReturnType;
    bool currentFunctionHasGenerics;
    ObjString** genericNames;
    GenericConstraint* genericConstraints;
    int genericCount;
    
    // Register VM compilation mode - Phase 1.1 enhancement
    bool isRegisterMode;           // True when compiling directly to register VM
    struct RegisterChunk* rchunk;  // Target register chunk when in register mode
    uint8_t nextRegister;          // Next available register for allocation
    uint8_t registerStack[256];    // Stack of allocated registers for expression evaluation
    int registerStackTop;          // Top of register stack
} Compiler;

void initCompiler(Compiler* compiler, Chunk* chunk,
                  const char* filePath, const char* sourceCode);
bool compile(ASTNode* ast, Compiler* compiler, bool requireMain);

// Register VM compilation functions - Phase 1.1 enhancement
void initRegisterCompiler(Compiler* compiler, struct RegisterChunk* rchunk,
                         const char* filePath, const char* sourceCode);
bool compileToRegisterDirect(ASTNode* ast, struct RegisterChunk* rchunk, 
                            const char* filePath, const char* sourceCode, bool requireMain);
bool compileToRegister(ASTNode* ast, struct RegisterChunk* rchunk,
                       const char* filePath, const char* sourceCode,
                       bool requireMain);
uint8_t resolveVariable(Compiler* compiler, Token name);       // Added
uint8_t addLocal(Compiler* compiler, Token name, Type* type, bool isMutable, bool isConst);  // Added
uint8_t defineVariable(Compiler* compiler, Token name, Type* type);  // Added

#endif