#ifndef ORUS_ERROR_H
#define ORUS_ERROR_H

#include "common.h"
#include "value.h"

typedef enum {
    ERROR_RUNTIME,
    ERROR_TYPE,
    ERROR_IO
} ErrorType;

typedef struct ObjError {
    Obj obj;
    ErrorType type;
    ObjString* message;
} ObjError;

ObjError* allocateError(ErrorType type, const char* message);

#endif // ORUS_ERROR_H
