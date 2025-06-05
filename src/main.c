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
#include "../include/version.h"

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
        initCompiler(&compiler, &chunk);
        vm.astRoot = ast;
        if (!compile(ast, &compiler)) {
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
            printf("Runtime error.\n");
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
    ASTNode* ast;
    if (!parse(source, &ast)) {
        fprintf(stderr, "Parsing failed for \"%s\".\n", path);
        free(source);
        exit(65);
    }
    Chunk chunk;
    initChunk(&chunk);
    Compiler compiler;
    initCompiler(&compiler, &chunk);
    vm.astRoot = ast;
    if (!compile(ast, &compiler)) {
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
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("Orus %s\n", ORUS_VERSION);
        return 0;
    }

    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: orus [path]\n");
        exit(64);
    }

    freeVM();
    freeTypeSystem();
    return 0;
}