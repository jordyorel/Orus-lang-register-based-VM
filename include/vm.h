#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "scanner.h"
#include "type.h"
#include "value.h"
#include <stdbool.h>

#define STACK_MAX 2048
#define FRAMES_MAX 256
#define LOOP_ITERATION_LIMIT 10000
#define TRY_MAX 64
#define MAX_NATIVES 64

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    ObjString* name;
    NativeFn function;
    int arity;      // -1 for variadic
    Type* returnType;
} NativeFunction;

typedef struct {
    int start;          // Bytecode offset of the function body
    uint8_t arity;      // Number of parameters
    Chunk* chunk;       // Owning chunk for the function
} Function;

typedef struct VarName {
    ObjString* name;
    int length;
} VarName;

typedef struct {
    uint8_t* returnAddress;  // Where to return to after function completes
    int stackOffset;         // Where this frame's stack starts
    uint8_t functionIndex;   // Index of the function being called
    Chunk* previousChunk;    // Chunk to restore after return
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
    bool publicGlobals[UINT8_COUNT];
    VarName variableNames[UINT8_COUNT];
    uint16_t variableCount;

    struct ASTNode* astRoot;

    // Path of the file currently being executed. Used for runtime diagnostics.
    const char* filePath;
    int currentLine;
    int currentColumn;

    Function functions[UINT8_COUNT];
    uint16_t functionCount;
    struct ASTNode* functionDecls[UINT8_COUNT];

    // Call frames for function calls
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    TryFrame tryFrames[TRY_MAX];
    int tryFrameCount;

    Value lastError;

    ObjString* loadedModules[UINT8_COUNT];
    uint8_t moduleCount;

    NativeFunction nativeFunctions[MAX_NATIVES];
    int nativeFunctionCount;

    // Garbage collector state
    Obj* objects;
    size_t bytesAllocated;
    bool trace;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
InterpretResult interpret_module(const char* path);
InterpretResult runChunk(Chunk* chunk);  // Execute a pre-compiled chunk
void push(Value value);
Value pop();
void vmPrintStackTrace(void);

// Record a runtime error with the current execution location so the
// caller can emit a diagnostic later. The message format should not
// include a trailing newline.
void vmRuntimeError(const char* message);

// Native function helpers
void defineNative(const char* name, NativeFn function, int arity, Type* returnType);
int findNative(ObjString* name);

extern Type* variableTypes[UINT8_COUNT];

#endif
