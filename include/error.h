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

typedef struct ObjError {
    Obj obj;
    ErrorType type;
    ObjString* message;
    SrcLocation location;
} ObjError;

ObjError* allocateError(ErrorType type, const char* message, SrcLocation location);

#endif // ORUS_ERROR_H
