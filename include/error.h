#ifndef ORUS_ERROR_H
#define ORUS_ERROR_H

#include "common.h"
#include "value.h"
#include "location.h"

typedef enum {
    ERROR_RUNTIME,
    ERROR_TYPE,
    ERROR_IO
} ErrorType;

//==================== Compile time diagnostics ====================//

// Error codes used by the compiler diagnostic engine. These mirror the
// style of Rust's E-prefixed error numbers but are stored as integers for
// simplicity.
typedef enum {
    ERROR_UNDEFINED_VARIABLE = 425,    // E0425
    ERROR_TYPE_MISMATCH      = 308,    // E0308
    ERROR_IMMUTABLE_ASSIGNMENT = 594,  // E0594
    ERROR_SCOPE_ERROR        = 426,    // E0426
    ERROR_FUNCTION_CALL      =  61,    // E0061
} ErrorCode;

// Span of source code used for highlighting errors.
typedef struct {
    int line;
    int column;
    int length;
    const char* filePath;
} SourceSpan;

// Text associated with a diagnostic: the main message, optional help and
// optional notes.
typedef struct {
    const char* message;
    char* help;
    char** notes;
    int noteCount;
} DiagnosticText;

// Structured diagnostic information that can be emitted by the compiler.
typedef struct {
    ErrorCode code;
    DiagnosticText text;
    SourceSpan primarySpan;
    SourceSpan* secondarySpans;  // Optional related spans
    int secondarySpanCount;
    const char* sourceText;      // Cached line of source if available
} Diagnostic;

typedef struct ObjError {
    Obj obj;
    ErrorType type;
    ObjString* message;
    SrcLocation location;
} ObjError;

ObjError* allocateError(ErrorType type, const char* message, SrcLocation location);

#endif // ORUS_ERROR_H
