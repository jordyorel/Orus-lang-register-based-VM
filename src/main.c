#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/debug.h"
#include "../include/parser.h"
#include "../include/file_utils.h"
#include "../include/vm.h"
#include "../include/error.h"
#include "../include/version.h"

static void printError(ObjError* err) {
    fprintf(stderr, "%s:%d:%d: %s\n", err->location.file, err->location.line,
            err->location.column, err->message->chars);
    vmPrintStackTrace();
}

extern VM vm;

static void repl() {
    char buffer[4096]; // Larger buffer for multiline input
    char line[1024];
    for (;;) {
        printf("> ");
        fflush(stdout);

        // Handle EOF (Ctrl+D) or errors in input
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        // Skip empty lines or lines with just whitespace
        bool isEmpty = true;
        for (int i = 0; line[i] != '\0' && i < sizeof(line); i++) {
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\r') {
                isEmpty = false;
                break;
            }
        }
        if (isEmpty) continue;
        
        // Copy line to buffer for processing
        strcpy(buffer, line);

        // Process the input
        ASTNode* ast;
        if (!parse(buffer, &ast)) {
            printf("Parsing failed.\n");
            fflush(stdout);
            continue;
        }

        // Check if this is a print statement or other statement that doesn't need result echoing
        bool isPrintStmt = false;
        if (ast && ast->type == AST_PRINT) {
            isPrintStmt = true;
        }

        Chunk chunk;
        initChunk(&chunk);
        Compiler compiler;
        initCompiler(&compiler, &chunk, "<repl>", buffer);
        vm.astRoot = ast;
        if (!compile(ast, &compiler, false)) {
            printf("Compilation failed.\n");
            vm.astRoot = NULL;
            freeChunk(&chunk);
            fflush(stdout);
            continue;
        }
        vm.astRoot = NULL;

        InterpretResult result = runChunk(&chunk);
        if (result == INTERPRET_COMPILE_ERROR) {
            printf("Compile error.\n");
        } else if (result == INTERPRET_RUNTIME_ERROR) {
            if (IS_ERROR(vm.lastError)) {
                printError(AS_ERROR(vm.lastError));
            } else {
                printf("Runtime error.\n");
            }
        } else if (!isPrintStmt && vm.stackTop > vm.stack) {
            // Print the result of the expression if there's a value on the stack
            // and it's not a print statement (which already outputs its value)
            printValue(*(vm.stackTop - 1));  // Print the top value on the stack
            printf("\n");
        }

        freeChunk(&chunk);
        vm.stackTop = vm.stack;  // Reset stack after execution
        fflush(stdout);
    }
}


static void runFile(const char* path) {
    char* source = readFile(path);
    if (source == NULL) {
        // readFile already prints an error message when it fails
        exit(65);
    }
    ASTNode* ast;
    if (!parse(source, &ast)) {
        fprintf(stderr, "Parsing failed for \"%s\".\n", path);
        free(source);
        exit(65);
    }
    Chunk chunk;
    initChunk(&chunk);
    Compiler compiler;
    initCompiler(&compiler, &chunk, path, source);
    vm.astRoot = ast;
    if (!compile(ast, &compiler, true)) {
        fprintf(stderr, "Compilation failed for \"%s\".\n", path);
        vm.astRoot = NULL;
        freeChunk(&chunk);
        free(source);
        exit(65);
    }
    vm.astRoot = NULL;
    InterpretResult result = runChunk(&chunk);
    freeChunk(&chunk);  // Free chunk after execution
    free(source);
    if (result == INTERPRET_RUNTIME_ERROR) {
        if (IS_ERROR(vm.lastError)) {
            printError(AS_ERROR(vm.lastError));
        }
        exit(70);
    }
}

int main(int argc, const char* argv[]) {
    bool traceFlag = false;
    const char* path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("Orus %s\n", ORUS_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--trace") == 0) {
            traceFlag = true;
        } else if (!path) {
            path = argv[i];
        } else {
            fprintf(stderr, "Usage: orus [--trace] [path]\n");
            return 64;
        }
    }

    initVM();
    if (traceFlag) vm.trace = true;

    if (!path) {
        repl();
    } else {
        runFile(path);
    }

    freeVM();
    freeTypeSystem();
    return 0;
}