#define _POSIX_C_SOURCE 200809L
#include "../../include/modules.h"
#include "../../include/file_utils.h"
#include "../../include/parser.h"
#include "../../include/compiler.h"
#include "../../include/vm.h"
#include <string.h>
#include <stdlib.h>

static Module module_cache[UINT8_COUNT];
static uint8_t module_cache_count = 0;

extern bool traceImports;
const char* moduleError = NULL;

char* load_module_source(const char* resolved_path) {
    return readFileSilent(resolved_path);
}

char* load_module_with_fallback(const char* path, char** disk_path, long* mtime, bool* from_embedded) {
    if (disk_path) *disk_path = strdup(path);
    if (mtime) *mtime = 0;
    if (from_embedded) *from_embedded = false;
    return readFileSilent(path);
}

ASTNode* parse_module_source(const char* source_code, const char* module_name) {
    ASTNode* ast; 
    if (!parse(source_code, module_name, &ast)) return NULL; 
    return ast; 
}

RegisterChunk* compile_module_ast_to_register(ASTNode* ast, const char* module_name) {
    RegisterChunk* chunk = malloc(sizeof(RegisterChunk));
    if (!chunk) return NULL;
    initRegisterChunk(chunk);
    if (!compileToRegister(ast, chunk, module_name, NULL, false)) {
        freeRegisterChunk(chunk);
        free(chunk);
        return NULL;
    }
    return chunk;
}

bool register_module(Module* module) {
    if (module_cache_count == UINT8_MAX) return false;
    module_cache[module_cache_count++] = *module;
    return true;
}

Module* get_module(const char* name) {
    for (int i = 0; i < module_cache_count; i++) {
        if (strcmp(module_cache[i].module_name, name) == 0) return &module_cache[i];
    }
    return NULL;
}

InterpretResult compile_module_only(const char* path) {
    char* source = readFileSilent(path);
    if (!source) return INTERPRET_RUNTIME_ERROR;
    ASTNode* ast = parse_module_source(source, path);
    free(source);
    if (!ast) return INTERPRET_COMPILE_ERROR;
    RegisterChunk* chunk = compile_module_ast_to_register(ast, path);
    if (!chunk) return INTERPRET_COMPILE_ERROR;
    Module mod = {0};
    mod.module_name = strdup(path);
    mod.name = strdup(path);
    mod.regBytecode = chunk;
    mod.export_count = 0;
    mod.executed = false;
    register_module(&mod);
    return INTERPRET_OK;
}

InterpretResult interpret_module(const char* path) {
    return compile_module_only(path);
}
