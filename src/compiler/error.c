#include "../../include/error.h"
#include "../../include/compiler.h"
#include "../../include/scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    const char* category = "Compile error";
    if (diagnostic->code == (ErrorCode)ERROR_RUNTIME) {
        category = "Runtime error";
    } else if (diagnostic->code == (ErrorCode)ERROR_TYPE &&
               diagnostic->code != ERROR_PARSE) {
        category = "Runtime type error";
    } else if (diagnostic->code == (ErrorCode)ERROR_IO) {
        category = "Runtime I/O error";
    }

    printf("%s%s [E%04d]%s: %s\n",
           COLOR_RED, category, diagnostic->code, COLOR_RESET,
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

    // Buffer for the help text and notes
    char helpBuffer[256];
    char* note = NULL;
    
    // Secondary span for definition location if available
    if (defToken) {
        diagnostic.secondarySpanCount = 1;
        diagnostic.secondarySpans = malloc(sizeof(SourceSpan));
        diagnostic.secondarySpans[0].line = defToken->line;
        lineStart = defToken->start;
        while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
        diagnostic.secondarySpans[0].column = (int)(defToken->start - lineStart) + 1;
        diagnostic.secondarySpans[0].length = defToken->length;
        diagnostic.secondarySpans[0].filePath = compiler->filePath;
        
        // Provide context-specific help based on the variable's location
        // Get an estimate of scope relationship by checking line numbers
        // This is approximate since we don't store scope depth in tokens directly
        if (defToken->line < useToken->line) {
            // Variable defined before use, likely an inner scope issue
            snprintf(helpBuffer, sizeof(helpBuffer),
                "variable `%s` was defined on line %d but is no longer in scope", 
                name, defToken->line);
            note = "consider moving the variable declaration to an outer scope if you need to use it here";
        } else {
            // Variable defined after use, likely a declaration order issue
            snprintf(helpBuffer, sizeof(helpBuffer),
                "variable `%s` is defined on line %d but used before its declaration", 
                name, defToken->line);
            note = "in Orus, variables must be declared before they are used";
        }
    } else {
        // No definition found, the variable doesn't exist at all
        snprintf(helpBuffer, sizeof(helpBuffer),
             "could not find a declaration of `%s` in this scope or any parent scope", name);
        note = "check for typos or declare the variable before using it";
    }

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "cannot find variable `%s` in this scope", name);
    diagnostic.text.message = msgBuffer;

    diagnostic.text.help = strdup(helpBuffer);
    
    diagnostic.text.notes = &note;
    diagnostic.text.noteCount = 1;

    emitDiagnostic(&diagnostic);

    // Clean up allocated memory
    if (diagnostic.secondarySpans) free(diagnostic.secondarySpans);
    if (diagnostic.text.help) free(diagnostic.text.help);
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

    char helpBuffer[256];
    char* note = NULL;
    
    // Provide specific help based on the mismatched types
    if (strstr(expectedType, "i32") && strstr(actualType, "f64")) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "try using `as i32` to convert the float to an integer");
        note = "floating-point to integer conversions may lose precision";
    } 
    else if (strstr(expectedType, "f64") && (strstr(actualType, "i32") || strstr(actualType, "u32"))) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "try using `as f64` to convert the integer to a float");
    }
    else if (strstr(expectedType, "bool")) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "Orus requires explicit boolean conditions - try a comparison like `!= 0` or `== true`");
    }
    else if (strstr(actualType, "bool")) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "booleans cannot be implicitly converted - use an if statement or conditional instead");
    }
    else if (strstr(expectedType, "array") || strstr(actualType, "array")) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "arrays must have matching element types and dimensions");
        note = "consider creating a new array with the correct type";
    }
    else if (strstr(expectedType, "string") || strstr(actualType, "string")) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "strings cannot be implicitly converted to or from other types");
        note = "use string interpolation for formatting values as strings";
    }
    else {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "try using a compatible type or adding an explicit conversion with `as %s`", 
                expectedType);
    }

    diagnostic.text.help = strdup(helpBuffer);
    
    if (note) {
        diagnostic.text.notes = &note;
        diagnostic.text.noteCount = 1;
    }

    emitDiagnostic(&diagnostic);
    
    if (diagnostic.text.help) free(diagnostic.text.help);
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

    // Generate a suggested alternative name
    char suggestedName[128];
    if (strlen(name) < 120) {
        // Create a suggestion by adding a number or 'new' prefix
        snprintf(suggestedName, sizeof(suggestedName), "%s2", name);
        
        char helpBuffer[256];
        snprintf(helpBuffer, sizeof(helpBuffer),
                "consider using a different name like `%s` or shadowing it in a new scope block",
                suggestedName);
        diagnostic.text.help = strdup(helpBuffer);
        
        char* note = "in Orus, each variable must have a unique name within its scope";
        diagnostic.text.notes = &note;
        diagnostic.text.noteCount = 1;
    } else {
        // Fall back to general advice for unusually long names
        diagnostic.text.help = strdup("rename the variable or remove the previous declaration");
    }

    emitDiagnostic(&diagnostic);
    
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

// Emit a generic type error with a custom message, help, and note.
void emitGenericTypeError(Compiler* compiler,
                         Token* token,
                         const char* message,
                         const char* help,
                         const char* note) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = ERROR_TYPE_MISMATCH; // Use type mismatch or define a new code if needed
    diagnostic.primarySpan.line = token->line;
    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    diagnostic.text.message = message;
    diagnostic.text.help = help ? strdup(help) : NULL;
    if (note) {
        diagnostic.text.notes = (char**)&note;
        diagnostic.text.noteCount = 1;
    } else {
        diagnostic.text.notes = NULL;
        diagnostic.text.noteCount = 0;
    }

    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

// Emit an error when a function is not found.
void emitUndefinedFunctionError(Compiler* compiler, Token* token) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = ERROR_FUNCTION_CALL;
    diagnostic.primarySpan.line = token->line;
    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
        "cannot find function `%.*s` in this scope",
        token->length, token->start);
    diagnostic.text.message = msgBuffer;
    diagnostic.text.help = strdup("check for typos, missing imports, or incorrect function name");
    const char* note = "functions must be defined before use and imported if from another module";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;

    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

void emitPrivateFunctionError(Compiler* compiler, Token* token) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = ERROR_PRIVATE_ACCESS;
    diagnostic.primarySpan.line = token->line;
    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "function `%.*s` is private", token->length, token->start);
    diagnostic.text.message = msgBuffer;
    diagnostic.text.help = strdup("mark the function with `pub` to allow access from other modules");
    const char* note = "only public items can be accessed from other modules";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;

    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

void emitPrivateVariableError(Compiler* compiler, Token* token) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = ERROR_PRIVATE_ACCESS;
    diagnostic.primarySpan.line = token->line;
    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "variable `%.*s` is private", token->length, token->start);
    diagnostic.text.message = msgBuffer;
    diagnostic.text.help = strdup("mark the variable with `pub` to allow access from other modules");
    const char* note = "only public items can be accessed from other modules";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;

    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

void emitStructFieldTypeMismatchError(Compiler* compiler, Token* token, const char* structName, const char* fieldName, const char* expectedType, const char* actualType) {
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
    char msgBuffer[256];
    snprintf(msgBuffer, sizeof(msgBuffer),
        "type mismatch for field `%s` in struct `%s`: expected `%s`, found `%s`",
        fieldName, structName, expectedType, actualType);
    diagnostic.text.message = msgBuffer;
    diagnostic.text.help = strdup("check the struct definition and the value assigned to this field");
    const char* note = "all struct fields must match their declared types";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;
    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

void emitFieldAccessNonStructError(Compiler* compiler, Token* token, const char* actualType) {
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
        "can only access fields on structs, but found `%s`",
        actualType);
    diagnostic.text.message = msgBuffer;
    diagnostic.text.help = strdup("make sure you are accessing a struct instance");
    const char* note = "field access is only valid on struct types";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;
    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

void emitIsTypeSecondArgError(Compiler* compiler, Token* token, const char* actualType) {
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
        "second argument to `is_type()` must be a string, found `%s`",
        actualType);
    diagnostic.text.message = msgBuffer;
    
    diagnostic.text.help = strdup("provide a string literal representing a type name, e.g., \"i32\", \"string\", etc.");
    
    const char* note = "is_type() checks if a value has the specified type, where the type name must be a string";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;
    
    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

void emitLenInvalidTypeError(Compiler* compiler, Token* token, const char* actualType) {
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
        "`len()` expects an array or string, found `%s`",
        actualType);
    diagnostic.text.message = msgBuffer;
    
    const char* help = "provide an array or string as the argument to len()";
    diagnostic.text.help = strdup(help);
    
    const char* note = "the len() function can only be used with arrays or strings to determine their length";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;
    
    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

void emitBuiltinArgCountError(Compiler* compiler, Token* token,
                              const char* name, int expected, int actual) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));
    diagnostic.code = ERROR_FUNCTION_CALL;
    diagnostic.primarySpan.line = token->line;
    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length;
    diagnostic.primarySpan.filePath = compiler->filePath;

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "%s() expects %d argument%s but %d %s supplied",
             name, expected, expected == 1 ? "" : "s",
             actual, actual == 1 ? "was" : "were");
    diagnostic.text.message = msgBuffer;

    char helpBuffer[128];
    const char* note = NULL;

    // Special handling for built-in functions to provide better help messages
    if (strcmp(name, "type_of") == 0) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "provide a value to check its type: %s(value)", name);
        note = "type_of() returns a string representation of the type of the given value";
    } 
    else if (strcmp(name, "is_type") == 0) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "provide both a value and a type string: %s(value, \"type_name\")", name);
        note = "is_type() checks if a value matches the specified type";
    } 
    else if (strcmp(name, "substring") == 0) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "provide a string, start index, and length: %s(str, start, length)", name);
        note = "substring() extracts a portion of the given string";
    }
    else if (strcmp(name, "len") == 0) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "provide an array or string: %s(value)", name);
        note = "len() returns the length of an array or string";
    }
    else if (strcmp(name, "push") == 0) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "provide an array and a value: %s(array, value)", name);
        note = "push() adds an element to the end of an array";
    }
    else if (strcmp(name, "pop") == 0) {
        snprintf(helpBuffer, sizeof(helpBuffer),
                "provide an array: %s(array)", name);
        note = "pop() removes and returns the last element from an array";
    }
    else {
        // Default for other functions
        snprintf(helpBuffer, sizeof(helpBuffer),
                "provide %d argument%s to %s()",
                expected, expected == 1 ? "" : "s", name);
    }
    
    diagnostic.text.help = strdup(helpBuffer);

    if (note) {
        diagnostic.text.notes = (char**)&note;
        diagnostic.text.noteCount = 1;
    } else {
        diagnostic.text.notes = NULL;
        diagnostic.text.noteCount = 0;
    }

    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

// Emit a simple compiler error when no detailed context is available.
void emitSimpleError(Compiler* compiler, ErrorCode code, const char* message) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = code;
    diagnostic.text.message = message;
    diagnostic.primarySpan.filePath = compiler->filePath;
    diagnostic.primarySpan.line = compiler->currentLine > 0 ? compiler->currentLine : 1;
    diagnostic.primarySpan.column = 1;
    diagnostic.primarySpan.length = 1;

    diagnostic.text.help = strdup("refer to the Orus documentation for possible resolutions");
    const char* note = "a generic compiler error occurred";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;

    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

// Emit a compiler error at a specific token location so the diagnostic caret
// points to the offending part of the source code.
void emitTokenError(Compiler* compiler,
                    Token* token,
                    ErrorCode code,
                    const char* message) {
    if (compiler->panicMode) return;
    compiler->panicMode = true;

    Diagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(Diagnostic));

    diagnostic.code = code;
    diagnostic.text.message = message;
    diagnostic.primarySpan.filePath = compiler->filePath;
    diagnostic.primarySpan.line = token->line;

    const char* lineStart = token->start;
    while (lineStart > compiler->sourceCode && lineStart[-1] != '\n') lineStart--;
    diagnostic.primarySpan.column = (int)(token->start - lineStart) + 1;
    diagnostic.primarySpan.length = token->length > 0 ? token->length : 1;

    diagnostic.text.help = strdup("check the highlighted token for mistakes");
    const char* note = "the compiler encountered an unexpected token here";
    diagnostic.text.notes = (char**)&note;
    diagnostic.text.noteCount = 1;

    emitDiagnostic(&diagnostic);
    if (diagnostic.text.help) free(diagnostic.text.help);
    compiler->hadError = true;
}

