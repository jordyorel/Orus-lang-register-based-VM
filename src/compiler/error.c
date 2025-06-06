#include "../../include/error.h"
#include "../../include/compiler.h"
#include "../../include/scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ANSI color codes for pretty diagnostics
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31;1m"
#define COLOR_GREEN   "\x1b[32;1m"
#define COLOR_YELLOW  "\x1b[33;1m"
#define COLOR_BLUE    "\x1b[34;1m"
#define COLOR_MAGENTA "\x1b[35;1m"
#define COLOR_CYAN    "\x1b[36;1m"
#define COLOR_BOLD    "\x1b[1m"

// Helper to fetch a specific line from a source file.
static const char* getSourceLine(const char* filePath, int lineNum) {
    FILE* file = fopen(filePath, "r");
    if (!file) return NULL;

    static char buffer[1024];
    int currentLine = 1;

    while (fgets(buffer, sizeof(buffer), file)) {
        if (currentLine == lineNum) {
            fclose(file);
            char* newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';
            return buffer;
        }
        currentLine++;
    }

    fclose(file);
    return NULL;
}

void emitDiagnostic(Diagnostic* diagnostic) {
    // 1. Header with error code and message
    printf("%sError[E%04d]%s: %s\n",
           COLOR_RED, diagnostic->code, COLOR_RESET,
           diagnostic->text.message);

    // 2. File location
    printf("%s --> %s:%d:%d%s\n",
           COLOR_CYAN, diagnostic->primarySpan.filePath,
           diagnostic->primarySpan.line, diagnostic->primarySpan.column,
           COLOR_RESET);

    // 3. Grab line of source code
    const char* sourceLine = diagnostic->sourceText;
    if (!sourceLine && diagnostic->primarySpan.filePath) {
        sourceLine = getSourceLine(diagnostic->primarySpan.filePath,
                                   diagnostic->primarySpan.line);
    }

    if (sourceLine) {
        printf(" %s%4d |%s %s\n", COLOR_BLUE,
               diagnostic->primarySpan.line, COLOR_RESET, sourceLine);
        printf("      | ");
        for (int i = 0; i < diagnostic->primarySpan.column - 1; i++) {
            putchar(' ');
        }
        printf("%s", COLOR_RED);
        for (int i = 0; i < diagnostic->primarySpan.length; i++) {
            putchar('^');
        }
        printf("%s\n", COLOR_RESET);
    }

    // 4. Secondary spans
    for (int i = 0; i < diagnostic->secondarySpanCount; i++) {
        SourceSpan* span = &diagnostic->secondarySpans[i];
        sourceLine = getSourceLine(span->filePath, span->line);
        if (sourceLine) {
            printf(" %s%4d |%s %s\n", COLOR_BLUE, span->line, COLOR_RESET, sourceLine);
            printf("      | ");
            for (int j = 0; j < span->column - 1; j++) putchar(' ');
            printf("%s", COLOR_CYAN);
            for (int j = 0; j < span->length; j++) putchar('^');
            printf("%s\n", COLOR_RESET);
        }
    }

    // 5. Help message
    if (diagnostic->text.help) {
        printf("%shelp%s: %s\n", COLOR_GREEN, COLOR_RESET, diagnostic->text.help);
    }

    // 6. Notes
    for (int i = 0; i < diagnostic->text.noteCount; i++) {
        printf("%snote%s: %s\n", COLOR_BLUE, COLOR_RESET,
               diagnostic->text.notes[i]);
    }

    printf("\n");
}

// Convenience helpers -------------------------------------------------------

// Emit an undefined variable error with an optional definition location.
void emitUndefinedVarError(Compiler* compiler,
                           Token* useToken,
                           Token* defToken,
                           const char* name) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = ERROR_UNDEFINED_VARIABLE;
    diagnostic.primarySpan.line = useToken->line;
    // compute column from source line start
    const char* lineStart = useToken->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(useToken->start - lineStart) + 1;
    diagnostic.primarySpan.length = useToken->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    if (defToken) {
        diagnostic.secondarySpanCount = 1;
        diagnostic.secondarySpans = malloc(sizeof(SourceSpan));
        diagnostic.secondarySpans[0].line = defToken->line;
        lineStart = defToken->start;
        while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
        diagnostic.secondarySpans[0].column = (int)(defToken->start - lineStart) + 1;
        diagnostic.secondarySpans[0].length = defToken->length;
        diagnostic.secondarySpans[0].filePath = compiler->filePath;
    }

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "cannot find variable `%s` in this scope", name);
    diagnostic.text.message = msgBuffer;

    char helpBuffer[128];
    snprintf(helpBuffer, sizeof(helpBuffer),
             "variable `%s` is defined in an inner scope and is not accessible here", name);
    diagnostic.text.help = helpBuffer;

    char* note = "variables defined in inner scopes are not accessible in outer scopes";
    diagnostic.text.notes = &note;
    diagnostic.text.noteCount = 1;

    emitDiagnostic(&diagnostic);

    if (diagnostic.secondarySpans) free(diagnostic.secondarySpans);
    compiler->hadError = true;
}

// Emit a type mismatch error between expected and actual types.
void emitTypeMismatchError(Compiler* compiler,
                           Token* token,
                           const char* expectedType,
                           const char* actualType) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = ERROR_TYPE_MISMATCH;
    diagnostic.primarySpan.line = token->line;
    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "expected type `%s`, found `%s`", expectedType, actualType);
    diagnostic.text.message = msgBuffer;

    char helpBuffer[128];
    snprintf(helpBuffer, sizeof(helpBuffer),
             "try using a compatible type or adding an explicit conversion");
    diagnostic.text.help = helpBuffer;

    emitDiagnostic(&diagnostic);

    compiler->hadError = true;
}

// Emit an error when a variable is redeclared in the same scope.
void emitRedeclarationError(Compiler* compiler, Token* token, const char* name) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = ERROR_SCOPE_ERROR;
    diagnostic.primarySpan.line = token->line;
    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "variable `%s` already declared in this scope", name);
    diagnostic.text.message = msgBuffer;

    char* help = "rename the variable or remove the previous declaration";
    diagnostic.text.help = help;

    emitDiagnostic(&diagnostic);

    compiler->hadError = true;
}

