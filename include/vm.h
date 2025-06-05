#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "scanner.h"
#include "type.h"
#include "value.h"

#define STACK_MAX 256
#define FRAMES_MAX 64
#define TRY_MAX 64

typedef struct {
    int start;          // Bytecode offset of the function body
    uint8_t arity;      // Number of parameters
} Function;

typedef struct VarName {
    ObjString* name;
    int length;
} VarName;

typedef struct {
    uint8_t* returnAddress;  // Where to return to after function completes
    int stackOffset;         // Where this frame's stack starts
    uint8_t functionIndex;   // Index of the function being called
} CallFrame;

typedef struct {
    uint8_t* handler;
    uint8_t varIndex;
    int stackDepth;
} TryFrame;

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Value globals[UINT8_COUNT];
    Type* globalTypes[UINT8_COUNT];
    VarName variableNames[UINT8_COUNT];
    uint16_t variableCount;

    struct ASTNode* astRoot;

    Function functions[UINT8_COUNT];
    uint16_t functionCount;

    // Call frames for function calls
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    TryFrame tryFrames[TRY_MAX];
    int tryFrameCount;

    Value lastError;

    // Garbage collector state
    Obj* objects;
    size_t bytesAllocated;
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