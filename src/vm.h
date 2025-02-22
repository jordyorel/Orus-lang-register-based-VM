#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "scanner.h"
#include "type.h"
#include "value.h"

#define STACK_MAX 256

typedef struct VarName {
    char* name;
    int length;
} VarName;

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Value globals[UINT8_COUNT];
    Type* globalTypes[UINT8_COUNT];
    VarName variableNames[UINT8_COUNT];
    uint16_t variableCount;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
InterpretResult runChunk(Chunk* chunk);  // Execute a pre-compiled chunk
void push(Value value);
Value pop();

extern Type* variableTypes[UINT8_COUNT];

#endif