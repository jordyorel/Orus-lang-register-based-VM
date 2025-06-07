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
    // Format like the compiler's diagnostic system
    if (err->location.file) {
        // Print error message with file location
        fprintf(stderr, "%sError%s: %s\n", 
                "\x1b[31;1m", "\x1b[0m", err->message->chars);
        
        // Print file location
        fprintf(stderr, "%s --> %s:%d:%d%s\n",
               "\x1b[36m", err->location.file, err->location.line,
               err->location.column, "\x1b[0m");
        
        // For string interpolation errors, provide helpful note
        if (strstr(err->message->chars, "string interpolation")) {
            fprintf(stderr, "%snote%s: each '{}' in the format string corresponds to one argument provided after the format string\n",
                   "\x1b[34m", "\x1b[0m");
            fprintf(stderr, "%shelp%s: ensure the number of '{}' placeholders matches the number of arguments\n\n",
                   "\x1b[32m", "\x1b[0m");
        }
    } else {
        fprintf(stderr, "%s\n", err->message->chars);
    }
    // Stack trace is already printed elsewhere
    // vmPrintStackTrace();
}

extern VM vm;

static void repl() {
    char buffer[4096]; // Larger buffer for multiline input
    char line[1024];
    vm.filePath = "<repl>";
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
    vm.filePath = path;
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
    vm.filePath = NULL;
    if (result == INTERPRET_RUNTIME_ERROR) {
        fprintf(stderr, "Runtime error in \"%s\".\n", path);
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