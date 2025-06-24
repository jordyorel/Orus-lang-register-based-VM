#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/debug.h"
#include "../include/parser.h"
#include "../include/file_utils.h"
#include "../include/modules.h"
#include "../include/builtin_stdlib.h"
#include "../include/error.h"
#include "../include/string_utils.h"
#include "../include/version.h"
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
extern VM vm;

static void deriveRuntimeHelp(const char* message,
                              char** helpOut,
                              const char** noteOut) {
    *helpOut = NULL;
    *noteOut = NULL;

    if (strstr(message, "string interpolation")) {
        *helpOut = strdup("ensure the number of '{}' placeholders matches the number of arguments");
        *noteOut = "each '{}' in the format string corresponds to one argument provided after the format string";
    } else if (strstr(message, "Stack underflow")) {
        *helpOut = strdup("check that every operator has enough input values");
        *noteOut = "this usually means a value was not pushed before the operation";
    } else if (strstr(message, "Operand must") || strstr(message, "Operands must")) {
        *helpOut = strdup("verify the value types or use explicit casts");
        *noteOut = "the operation expected a different type";
    } else if (strstr(message, "Module") && strstr(message, "not found")) {
        *helpOut = strdup("check the module path or adjust the ORUS_STD_PATH environment variable");
        const char* baseNote = "imports are resolved relative to the current file or the standard library path";
        const char* suggestion = NULL;
        const char* start = strchr(message, '`');
        const char* end = start ? strchr(start + 1, '`') : NULL;
        char modName[64];
        if (start && end && end - start - 1 < (int)sizeof(modName)) {
            int len = (int)(end - start - 1);
            memcpy(modName, start + 1, len);
            modName[len] = '\0';
            int bestDist = 4;
            for (int i = 0; i < vm.moduleCount; i++) {
                if (vm.loadedModules[i]) {
                    const char* cand = vm.loadedModules[i]->chars;
                    int dist = levenshteinDistance(modName, cand);
                    if (dist < bestDist) { bestDist = dist; suggestion = cand; }
                }
            }
        }
        if (suggestion) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s. Did you mean `%s`?", baseNote, suggestion);
            *noteOut = strdup(buf);
        } else {
            *noteOut = baseNote;
        }
    } else if (strstr(message, "Import cycle")) {
        *helpOut = strdup("restructure your modules to remove circular dependencies");
        *noteOut = "module A importing B while B imports A causes an import cycle";
    } else if (strstr(message, "already executed")) {
        *helpOut = strdup("import each module only once or use 'use' for reexports");
        *noteOut = "module code runs only on its first import";
    }

    if (!*helpOut) {
        *helpOut = strdup("refer to the runtime error message for more details");
    }
    if (!*noteOut) {
        *noteOut = "a runtime error occurred";
    }
}

static void printError(ObjError* err) {
    Diagnostic diag;
    memset(&diag, 0, sizeof(Diagnostic));
    diag.code = (ErrorCode)err->type;
    diag.text.message = err->message->chars;
    diag.primarySpan.filePath = err->location.file ? err->location.file : "<runtime>";
    diag.primarySpan.line = err->location.line;
    diag.primarySpan.column = err->location.column;
    diag.primarySpan.length = 1;

    char* help = NULL;
    const char* note = NULL;
    deriveRuntimeHelp(err->message->chars, &help, &note);

    diag.text.help = help;
    diag.text.notes = (char**)&note;
    diag.text.noteCount = 1;

    emitDiagnostic(&diag);

    if (help) free(help);
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
        if (!parse(buffer, "<repl>", &ast)) {
            printf("Parsing failed.\n");
            fflush(stdout);
            continue;
        }

        // Check if this is a print statement or other statement that doesn't need result echoing
        bool isPrintStmt = false;
        if (ast && ast->type == AST_PRINT) {
            isPrintStmt = true;
        }

        freeRegisterChunk(&vm.regChunk);
        initRegisterChunk(&vm.regChunk);
        vm.filePath = "<repl>";
        vm.astRoot = ast;
        if (!compileToRegister(ast, &vm.regChunk, "<repl>", buffer, false)) {
            printf("Compilation failed.\n");
            vm.astRoot = NULL;
            freeRegisterChunk(&vm.regChunk);
            fflush(stdout);
            continue;
        }
        vm.astRoot = NULL;

        initRegisterVM(&vm.regVM, &vm.regChunk);
        InterpretResult result = INTERPRET_OK;
        runRegisterVM(&vm.regVM);
        if (IS_ERROR(vm.lastError)) {
            result = INTERPRET_RUNTIME_ERROR;
        }
        freeRegisterVM(&vm.regVM);
        freeRegisterChunk(&vm.regChunk);
        vm.filePath = NULL;
        if (result == INTERPRET_COMPILE_ERROR) {
            printf("Compile error.\n");
        } else if (result == INTERPRET_RUNTIME_ERROR) {
            if (IS_ERROR(vm.lastError)) {
                printError(AS_ERROR(vm.lastError));
            } else {
                printf("Runtime error.\n");
            }
        } else if (!isPrintStmt && vm.stackTop > vm.stack &&
                   !IS_NIL(*(vm.stackTop - 1))) {
            // Print the result of the expression if there's a value on the stack
            // that isn't nil and it's not a print statement (which already outputs its value)
            printValue(*(vm.stackTop - 1));  // Print the top value on the stack
            printf("\n");
        }

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
    if (!parse(source, path, &ast)) {
        fprintf(stderr, "Parsing failed for \"%s\".\n", path);
        free(source);
        exit(65);
    }
    InterpretResult result = INTERPRET_OK;
    freeRegisterChunk(&vm.regChunk);
    initRegisterChunk(&vm.regChunk);
    vm.filePath = path;
    vm.astRoot = ast;
    if (!compileToRegister(ast, &vm.regChunk, path, source, true)) {
        fprintf(stderr, "Compilation failed for \"%s\".\n", path);
        vm.astRoot = NULL;
        freeRegisterChunk(&vm.regChunk);
        free(source);
        exit(65);
    }
    vm.astRoot = NULL;
    initRegisterVM(&vm.regVM, &vm.regChunk);
    if (vm.trace) {
#ifdef DEBUG_TRACE_EXECUTION
        disassembleRegisterChunk(&vm.regChunk, "register chunk");
        printf("Function offsets:\n");
        for (int i = 0; i < vm.regChunk.functionCount; i++) {
            printf("%d -> %d\n", i, vm.regChunk.functionOffsets[i]);
        }
#endif
    }
    runRegisterVM(&vm.regVM);
    if (IS_ERROR(vm.lastError)) {
        result = INTERPRET_RUNTIME_ERROR;
    }
    freeRegisterVM(&vm.regVM);
    freeRegisterChunk(&vm.regChunk);
    
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

static bool file_has_main(const char* path) {
    char* source = readFile(path);
    if (!source) return false;

    ASTNode* ast;
    bool ok = parse(source, path, &ast);
    bool found = false;
    if (ok && ast) {
        for (ASTNode* node = ast; node; node = node->next) {
            if (node->type == AST_FUNCTION &&
                node->data.function.name.length == 4 &&
                strncmp(node->data.function.name.start, "main", 4) == 0) {
                found = true;
                break;
            }
        }
    }

    free(source);
    return found;
}

static void search_for_main(const char* base, const char* sub, int* count,
                            char* result, size_t resultSize) {
    char dirPath[PATH_MAX];
    if (sub[0] == '\0') {
        snprintf(dirPath, sizeof(dirPath), "%s", base);
    } else {
        snprintf(dirPath, sizeof(dirPath), "%s/%s", base, sub);
    }

    DIR* d = opendir(dirPath);
    if (!d) return;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char relPath[PATH_MAX];
        if (sub[0] == '\0') {
            snprintf(relPath, sizeof(relPath), "%s", entry->d_name);
        } else {
            snprintf(relPath, sizeof(relPath), "%s/%s", sub, entry->d_name);
        }

        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", base, relPath);

        struct stat st;
        if (stat(fullPath, &st) == 0 && S_ISDIR(st.st_mode)) {
            search_for_main(base, relPath, count, result, resultSize);
        } else {
            size_t len = strlen(entry->d_name);
            if (len > 5 && strcmp(entry->d_name + len - 5, ".orus") == 0) {
                if (file_has_main(fullPath)) {
                    (*count)++;
                    if (*count == 1) {
                        strncpy(result, relPath, resultSize - 1);
                        result[resultSize - 1] = '\0';
                    }
                }
            }
        }
    }

    closedir(d);
}

static void runProject(const char* dir) {
    char manifestPath[PATH_MAX];
    snprintf(manifestPath, sizeof(manifestPath), "%s/orus.json", dir);
    char* json = readFile(manifestPath);
    const char* entry = NULL;
    char* entryAlloc = NULL;
    if (json) {
        char* p = strstr(json, "\"entry\"");
        if (p) {
            p = strchr(p, ':');
            if (p) {
                p++;
                while (*p == ' ' || *p == '\t' || *p == '"') p++;
                char* start = p;
                while (*p && *p != '"' && *p != '\n') p++;
                size_t len = p - start;
                if (len > 0) {
                    entryAlloc = malloc(len + 1);
                    memcpy(entryAlloc, start, len);
                    entryAlloc[len] = '\0';
                    entry = entryAlloc;
                }
            }
        }
    }

    char foundPath[PATH_MAX];
    int mainCount = 0;
    if (!entry) {
        foundPath[0] = '\0';
        search_for_main(dir, "", &mainCount, foundPath, sizeof(foundPath));
        if (mainCount == 0) {
            fprintf(stderr, "No 'main' function found in project.\n");
            free(json);
            return;
        } else if (mainCount > 1) {
            fprintf(stderr, "Multiple 'main' functions found in project.\n");
            free(json);
            return;
        }
        entry = foundPath;
    } else {
        search_for_main(dir, "", &mainCount, foundPath, sizeof(foundPath));
        if (mainCount > 1 || (mainCount == 1 && strcmp(foundPath, entry) != 0)) {
            fprintf(stderr, "Project must contain a single 'main' function.\n");
            free(json);
            if (entryAlloc) free(entryAlloc);
            return;
        }
    }

    chdir(dir);
    runFile(entry);

    free(json);
    if (entryAlloc) free(entryAlloc);
}

int main(int argc, const char* argv[]) {
    bool traceFlag = false;
    bool traceImportsFlag = false;
    bool devFlag = false;
    bool dumpStdlib = false;
    const char* cliStdPath = NULL;
    char defaultStdPath[PATH_MAX];
    const char* path = NULL;
    const char* projectDir = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("Orus %s\n", ORUS_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--trace") == 0) {
            traceFlag = true;
        } else if (strcmp(argv[i], "--trace-imports") == 0) {
            traceImportsFlag = true;
        } else if (strcmp(argv[i], "--std-path") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Usage: --std-path <dir>\n");
                return 64;
            }
            cliStdPath = argv[++i];
        } else if (strcmp(argv[i], "--dump-stdlib") == 0) {
            dumpStdlib = true;
        } else if (strcmp(argv[i], "--dev") == 0) {
            devFlag = true;
        } else if (strcmp(argv[i], "--project") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Usage: orusc --project <dir>\n");
                return 64;
            }
            projectDir = argv[++i];
        } else if (!path) {
            path = argv[i];
        } else {
            fprintf(stderr, "Usage: orusc [--trace] [--trace-imports] [--std-path dir] [--dump-stdlib] [--dev] [--project dir] [path]\n");
            return 64;
        }
    }

    if (!cliStdPath) {
        const char* envPath = getenv("ORUS_PATH");
        if (!envPath || envPath[0] == '\0') {
            if (realpath(argv[0], defaultStdPath)) {
                char* slash = strrchr(defaultStdPath, '/');
                if (slash) {
                    *slash = '\0';
                    strncat(defaultStdPath, "/std", sizeof(defaultStdPath) - strlen(defaultStdPath) - 1);
                    cliStdPath = defaultStdPath;
                }
            }
        }
    }

    initVM();
    if (cliStdPath) vm.stdPath = cliStdPath;
    if (devFlag) vm.devMode = true;
    if (traceFlag) vm.trace = true;
    if (traceImportsFlag) traceImports = true;

    if (dumpStdlib) {
        dumpEmbeddedStdlib(vm.stdPath);
        freeVM();
        freeTypeSystem();
        return 0;
    }

    vm.useRegisterVM = true;
    if (projectDir) {
        runProject(projectDir);
    } else if (!path) {
        repl();
    } else {
        runFile(path);
    }

    freeVM();
    freeTypeSystem();
    return 0;
}