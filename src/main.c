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