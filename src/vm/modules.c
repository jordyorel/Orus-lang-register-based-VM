#include "../../include/modules.h"
#include "../../include/file_utils.h"
#include "../../include/parser.h"
#include "../../include/compiler.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static Module module_cache[UINT8_COUNT];
static uint8_t module_cache_count = 0;

char* load_module_source(const char* resolved_path) {
    return readFile(resolved_path);
}

ASTNode* parse_module_source(const char* source_code, const char* module_name) {
    ASTNode* ast;
    if (!parse(source_code, module_name, &ast)) {
        return NULL;
    }
    return ast;
}

Chunk* compile_module_ast(ASTNode* ast, const char* module_name) {
    Chunk* chunk = malloc(sizeof(Chunk));
    if (!chunk) return NULL;
    initChunk(chunk);
    Compiler compiler;
    initCompiler(&compiler, chunk, module_name, NULL);
    if (!compile(ast, &compiler, false)) {
        freeChunk(chunk);
        free(chunk);
        return NULL;
    }
    return chunk;
}

bool register_module(Module* module) {
    if (module_cache_count >= UINT8_COUNT) return false;
    module_cache[module_cache_count++] = *module;
    return true;
}

Module* get_module(const char* name) {
    for (int i = 0; i < module_cache_count; i++) {
        if (strcmp(module_cache[i].module_name, name) == 0) {
            return &module_cache[i];
        }
    }
    return NULL;
}

Value* get_export(Module* module, const char* name) {
    for (int i = 0; i < module->export_count; i++) {
        if (strcmp(module->exports[i].name, name) == 0) {
            return &module->exports[i].value;
        }
    }
    return NULL;
}
