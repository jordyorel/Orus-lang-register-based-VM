#include "../../include/modules.h"
#include "../../include/file_utils.h"
#include "../../include/parser.h"
#include "../../include/compiler.h"
#include "../../include/vm.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static Module module_cache[UINT8_COUNT];
static uint8_t module_cache_count = 0;

extern VM vm;

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

Export* get_export(Module* module, const char* name) {
    for (int i = 0; i < module->export_count; i++) {
        if (strcmp(module->exports[i].name, name) == 0) {
            return &module->exports[i];
        }
    }
    return NULL;
}

InterpretResult compile_module_only(const char* path) {
    if (get_module(path)) return INTERPRET_OK;

    char* source = load_module_source(path);
    if (!source) {
        const char* stdPath = getenv("ORUS_STD_PATH");
        if (!stdPath) stdPath = "std";
        char full[256];
        snprintf(full, sizeof(full), "%s/%s", stdPath, path);
        source = load_module_source(full);
        if (!source) {
            return INTERPRET_RUNTIME_ERROR;
        }
    }

    ASTNode* ast = parse_module_source(source, path);
    if (!ast) {
        free(source);
        return INTERPRET_COMPILE_ERROR;
    }

    int startGlobals = vm.variableCount;

    Chunk* chunk = compile_module_ast(ast, path);
    if (!chunk) {
        free(source);
        return INTERPRET_COMPILE_ERROR;
    }

    Module mod;
    mod.module_name = strdup(path);
    mod.bytecode = chunk;
    mod.export_count = 0;
    mod.executed = false;

    for (int i = startGlobals; i < vm.variableCount && mod.export_count < UINT8_COUNT; i++) {
        Export ex;
        ex.name = vm.variableNames[i].name ? vm.variableNames[i].name->chars : NULL;
        if (ex.name) {
            ex.name = strdup(ex.name);
            ex.value = vm.globals[i];
            ex.index = i;
            mod.exports[mod.export_count++] = ex;
        }
    }

    register_module(&mod);
    free(source);
    return INTERPRET_OK;
}
