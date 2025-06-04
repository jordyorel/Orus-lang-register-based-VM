#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/debug.h"
#include "../include/parser.h"
#include "../include/vm.h"

extern VM vm;

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        // Process the input

        ASTNode* ast;
        if (!parse(line, &ast)) {
            printf("Parsing failed.\n");
            fflush(stdout);
            continue;
        }

        Chunk chunk;
        initChunk(&chunk);
        Compiler compiler;
        initCompiler(&compiler, &chunk);
        if (!compile(ast, &compiler)) {
            printf("Compilation failed.\n");
            freeASTNode(ast);
            freeChunk(&chunk);
            fflush(stdout);
            continue;
        }

        InterpretResult result = runChunk(&chunk);
        if (result == INTERPRET_COMPILE_ERROR) {
            printf("Compile error.\n");
        } else if (result == INTERPRET_RUNTIME_ERROR) {
            printf("Runtime error.\n");
        }

        freeASTNode(ast);
        freeChunk(&chunk);
        vm.stackTop = vm.stack;  // Reset stack after execution
        fflush(stdout);
    }
}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Couldn't open the file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
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
    if (!compile(ast, &compiler)) {
        fprintf(stderr, "Compilation failed for \"%s\".\n", path);
        freeASTNode(ast);
        freeChunk(&chunk);
        free(source);
        exit(65);
    }
    InterpretResult result = runChunk(&chunk);
    freeASTNode(ast);   // Free AST after execution
    freeChunk(&chunk);  // Free chunk after execution
    free(source);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    freeTypeSystem();
    return 0;
}