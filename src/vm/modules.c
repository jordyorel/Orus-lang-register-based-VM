#include "../../include/modules.h"
#include "../../include/file_utils.h"
#include "../../include/parser.h"
#include "../../include/compiler.h"
#include "../../include/vm.h"
#include "../../include/builtin_stdlib.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>

static Module module_cache[UINT8_COUNT];
static uint8_t module_cache_count = 0;
static const char* loading_stack[UINT8_COUNT];
static uint8_t loading_stack_count = 0;

bool traceImports = false;

const char* moduleError = NULL;
static char module_error_buffer[256];

extern VM vm;

char* load_module_source(const char* resolved_path) {
    return readFileSilent(resolved_path);
}

char* load_module_with_fallback(const char* path, char** disk_path, long* mtime,
                                bool* from_embedded) {
    if (disk_path) *disk_path = NULL;
    if (mtime) *mtime = 0;
    if (from_embedded) *from_embedded = false;

    char* source = load_module_source(path);
    if (source) {
        if (disk_path) *disk_path = strdup(path);
        if (mtime) {
            struct stat st;
            if (stat(path, &st) == 0) *mtime = st.st_mtime;
        }
        return source;
    }
    char full[256];
    snprintf(full, sizeof(full), "%s/%s", vm.stdPath ? vm.stdPath : "std", path);
    source = load_module_source(full);
    if (source) {
        if (disk_path) *disk_path = strdup(full);
        if (mtime) {
            struct stat st;
            if (stat(full, &st) == 0) *mtime = st.st_mtime;
        }
        return source;
    }
    const char* embedded = getEmbeddedModule(path);
    if (embedded) {
        if (from_embedded) *from_embedded = true;
        fprintf(stderr, "[warning] Falling back to embedded module %s\n", path);
        return strdup(embedded);
    }
    return NULL;
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
    if (module_cache_count == UINT8_MAX) return false;
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
    moduleError = NULL;
    if (traceImports) fprintf(stderr, "[import] loading %s\n", path);
    for (int i = 0; i < loading_stack_count; i++) {
        if (strcmp(loading_stack[i], path) == 0) {
            snprintf(module_error_buffer, sizeof(module_error_buffer),
                     "Import cycle detected for module `%s`", path);
            moduleError = module_error_buffer;
            return INTERPRET_COMPILE_ERROR;
        }
    }

    if (loading_stack_count < UINT8_MAX)
        loading_stack[loading_stack_count++] = path;

    if (get_module(path)) {
        loading_stack_count--;
        return INTERPRET_OK;
    }

    char* diskPath = NULL;
    long mtime = 0;
    bool fromEmbedded = false;
    char* source = load_module_with_fallback(path, &diskPath, &mtime, &fromEmbedded);
    if (!source) {
        snprintf(module_error_buffer, sizeof(module_error_buffer),
                 "Module `%s` not found", path);
        moduleError = module_error_buffer;
        loading_stack_count--;
        return INTERPRET_RUNTIME_ERROR;
    }

    ASTNode* ast = parse_module_source(source, path);
    if (!ast) {
        free(source);
        loading_stack_count--;
        return INTERPRET_COMPILE_ERROR;
    }

    int startGlobals = vm.variableCount;

    Chunk* chunk = compile_module_ast(ast, path);
    if (!chunk) {
        free(source);
        loading_stack_count--;
        return INTERPRET_COMPILE_ERROR;
    }

    Module mod;
    mod.module_name = strdup(path);
    const char* base = strrchr(path, '/');
    base = base ? base + 1 : path;
    size_t len = strlen(base);
    if (len > 5 && strcmp(base + len - 5, ".orus") == 0) len -= 5;
    mod.name = (char*)malloc(len + 1);
    memcpy(mod.name, base, len);
    mod.name[len] = '\0';
    mod.bytecode = chunk;
    mod.export_count = 0;
    mod.executed = false;
    mod.disk_path = diskPath;
    mod.mtime = mtime;
    mod.from_embedded = fromEmbedded;

    for (int i = startGlobals; i < vm.variableCount && mod.export_count < UINT8_MAX; i++) {
        Export ex;
        ex.name = vm.variableNames[i].name ? vm.variableNames[i].name->chars : NULL;
        if (ex.name && vm.publicGlobals[i]) {
            ex.name = strdup(ex.name);
            ex.value = vm.globals[i];
            ex.index = i;
            mod.exports[mod.export_count++] = ex;
        }
    }

    register_module(&mod);
    loading_stack_count--;
    free(source);
    return INTERPRET_OK;
}
