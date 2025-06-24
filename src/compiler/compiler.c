#include "../include/compiler.h"
#include "../include/register_chunk.h"
#include "../include/ast.h"
#include <stdio.h>

/**
 * @brief Stub implementation of compileToRegister
 */
bool compileToRegister(ASTNode* ast, RegisterChunk* rchunk,
                      const char* filePath, const char* sourceCode, bool requireMain) {
    // TODO: Implement register compilation
    printf("Warning: compileToRegister is not yet implemented\n");
    return true;
}

/**
 * @brief Stub implementation of compileToRegisterDirect
 */
bool compileToRegisterDirect(ASTNode* ast, RegisterChunk* rchunk,
                            const char* filePath, const char* sourceCode,
                            bool requireMain) {
    // TODO: Implement direct register compilation
    printf("Warning: compileToRegisterDirect is not yet implemented\n");
    return true;
}
