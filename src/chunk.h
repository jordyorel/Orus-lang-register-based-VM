#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/// Enum representing the possible bytecode operations.
///
/// This enum defines the operations supported in the bytecode,
/// each corresponding to a unique byte value. The values represent
/// the different operations that can be performed in the virtual machine.
typedef enum {
    // op constant
    OP_CONSTANT,
    OP_CONSTANT_LONG,

    // Integer (i32) operations
    OP_ADD_I32,
    OP_SUBTRACT_I32,
    OP_MULTIPLY_I32,
    OP_DIVIDE_I32,
    OP_NEGATE_I32,

    // Unsigned integer (u32) operations
    OP_ADD_U32,
    OP_SUBTRACT_U32,
    OP_MULTIPLY_U32,
    OP_DIVIDE_U32,
    OP_NEGATE_U32,

    // Floating point (f64) operations
    OP_ADD_F64,
    OP_SUBTRACT_F64,
    OP_MULTIPLY_F64,
    OP_DIVIDE_F64,
    OP_NEGATE_F64,

    OP_MODULO_I32,
    OP_MODULO_U32,

    // Type conversion opcodes
    OP_I32_TO_F64,
    OP_U32_TO_F64,

    OP_RETURN,
    OP_POP,
    OP_PRINT,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
} opCode;

typedef struct {
    int line;
    int run_length;
} LineInfo;

// Dynamic array of bytecode:
// count: Number of used entries
// capacity: Number of entries
// uint8_t: dynamic array of bytecode
// Lines: dynamic array of lines
// ValueArray: dynamic array of values
typedef struct {
    int count; // 4 bytes
    int capacity; // 4 bytes
    uint8_t* code; // 8 bytes : instrution
    LineInfo* line_info;
    int line_count; // 8 bytes
    int line_capcity;
    ValueArray constants; // 16 bytes
} Chunk; // 40 bytes 


void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* Chunk, Value value);
void writeConstant(Chunk* chunk, Value value, int line);
int len(Chunk* chunk);
int get_line(Chunk* chunk, int offset);
uint8_t get_code(Chunk* chunk, int offset);
Value get_constant(Chunk* chunk, int offset);


#endif
