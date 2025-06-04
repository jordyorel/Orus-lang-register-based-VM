
#include<stdlib.h>
#include<stdio.h>

#include "chunk.h"
#include "memory.h"

// Initializes a Chunk structure to its default state.
// Parameters:
//   chunk - Pointer to the Chunk to be initialized.
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    // chunk->lines = NULL;
    initValueArray(&chunk->constants);
    chunk->line_info = NULL;
    chunk->line_count = 0;
    chunk->line_capcity = 0;
}

// Frees the memory associated with a Chunk and resets it to its default state.
// Parameters:
//   chunk - Pointer to the Chunk to be freed.
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineInfo, chunk->line_info, chunk->line_capcity);
    for (int i = 0; i < chunk->constants.count; i++) {
        Value v = chunk->constants.values[i];
        if (v.type == VAL_STRING) {
            free(v.as.string.chars);
            chunk->constants.values[i].as.string.chars = NULL;
            chunk->constants.values[i].as.string.length = 0;
        }
    }
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

// Adds a byte (instruction) to the Chunk, growing its capacity if necessary.
// Parameters:
//   chunk - Pointer to the Chunk where the byte will be written.
//   byte - The byte (instruction) to add to the chunk.
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    // if line_count is 0 or the last line is different from the current line
    if (chunk->line_capcity < chunk->line_count + 1) {
        int oldcapacity = chunk->line_capcity;
        chunk->line_capcity = GROW_CAPACITY(oldcapacity);
        chunk->line_info = GROW_ARRAY(LineInfo, chunk->line_info, oldcapacity,
                                      chunk->line_capcity);
    }
    chunk->line_info[chunk->line_count].line = line;
    chunk->line_info[chunk->line_count].run_length = 1;
    chunk->line_count++;
}

void writeConstant(Chunk* chunk, Value value, int line) {
    int constantindex = addConstant(chunk, value);
    
    if (constantindex < 256) {
        writeChunk(chunk, OP_CONSTANT, line);
        writeChunk(chunk, (uint8_t)constantindex, line);
    } else {
        writeChunk(chunk, OP_CONSTANT_LONG, line);

        // write the constant index in 3 bytes
        writeChunk(chunk, (constantindex >> 16) & 0xFF, line);
        writeChunk(chunk, (constantindex >> 8) & 0xFF, line);
        writeChunk(chunk, constantindex & 0xFF, line);
    }
}

// Adds a constant value to the Chunk, returning the index where it was added.
// Parameters:
//   chunk - Pointer to the Chunk where the constant will be added.
//   value - The constant value to add to the chunk.
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

int len(Chunk* chunk) { 
    return chunk->count; 
}

int get_line(Chunk* chunk, int instruction_offset) {
   int offset = 0;
   for (int i = 0; i < chunk->line_count; i++) {
        offset += chunk->line_info[i].run_length;
        if (instruction_offset < offset) {
            return chunk->line_info[i].line;
        }
   }
   return -1;
}

uint8_t get_code(Chunk* chunk, int offset) {
    if (offset < 0 || offset >= chunk->count - 1) {
        printf("Invalid operand access at offset %d\n", offset);
        exit(EXIT_FAILURE);
    }
    return chunk->code[offset + 1];
}

Value get_constant(Chunk* chunk, int offset) {
    uint8_t constantIndex = get_code(chunk, offset);
    if (constantIndex >= chunk->constants.count) {
        printf("Invalid constant index: %d\n", constantIndex);
        exit(EXIT_FAILURE);
    }
    return chunk->constants.values[constantIndex];
}
