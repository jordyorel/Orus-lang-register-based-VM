/**
 * @file chunk.c
 * @brief Bytecode chunk management utilities.
 */

#include<stdlib.h>
#include<stdio.h>

#include "../../include/chunk.h"
#include "../../include/memory.h"

/**
 * Initialize a bytecode Chunk structure.
 *
 * @param chunk Chunk to initialize.
 */
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

/**
 * Free memory used by a Chunk and reset it to the empty state.
 *
 * @param chunk Chunk to free.
 */
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineInfo, chunk->line_info, chunk->line_capcity);
    for (int i = 0; i < chunk->constants.count; i++) {
        Value v = chunk->constants.values[i];
        if (v.type == VAL_STRING) {
            chunk->constants.values[i].as.string = NULL;
        } else if (v.type == VAL_ARRAY) {
            chunk->constants.values[i].as.array = NULL;
        }
    }
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

/**
 * Append a bytecode instruction to a chunk and record its line information.
 *
 * @param chunk  Target chunk.
 * @param byte   Opcode byte to write.
 * @param line   Source line number.
 * @param column Source column number.
 */
void writeChunk(Chunk* chunk, uint8_t byte, int line, int column) {
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
    chunk->line_info[chunk->line_count].column = column;
    chunk->line_info[chunk->line_count].run_length = 1;
    chunk->line_count++;
}

/**
 * Write a constant value into the chunk's constant pool and emit the
 * appropriate instruction.
 *
 * @param chunk  Target chunk.
 * @param value  Constant value.
 * @param line   Source line number.
 * @param column Source column number.
 */
void writeConstant(Chunk* chunk, Value value, int line, int column) {
    int constantindex = addConstant(chunk, value);
    
    if (constantindex < 256) {
        writeChunk(chunk, OP_CONSTANT, line, column);
        writeChunk(chunk, (uint8_t)constantindex, line, column);
    } else {
        writeChunk(chunk, OP_CONSTANT_LONG, line, column);

        // write the constant index in 3 bytes
        writeChunk(chunk, (constantindex >> 16) & 0xFF, line, column);
        writeChunk(chunk, (constantindex >> 8) & 0xFF, line, column);
        writeChunk(chunk, constantindex & 0xFF, line, column);
    }
}

// Adds a constant value to the Chunk, returning the index where it was added.
// Parameters:
//   chunk - Pointer to the Chunk where the constant will be added.
//   value - The constant value to add to the chunk.
/**
 * Add a constant to the chunk and return its index.
 *
 * @param chunk Chunk receiving the value.
 * @param value Constant value.
 * @return      Index of the value in the constant pool.
 */
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

/**
 * Get the number of bytes of code in the chunk.
 *
 * @param chunk Chunk to query.
 */
int len(Chunk* chunk) {
    return chunk->count; 
}

/**
 * Find the source line for a given bytecode offset.
 *
 * @param chunk              Chunk to query.
 * @param instruction_offset Byte index of the instruction.
 * @return                   Source line number or -1.
 */
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

/**
 * Get the source column for a given bytecode offset.
 *
 * @param chunk              Chunk to query.
 * @param instruction_offset Byte index of the instruction.
 * @return                   Source column number.
 */
int get_column(Chunk* chunk, int instruction_offset) {
   int offset = 0;
   for (int i = 0; i < chunk->line_count; i++) {
        offset += chunk->line_info[i].run_length;
        if (instruction_offset < offset) {
            return chunk->line_info[i].column;
        }
   }
   return 1;
}

/**
 * Return the operand byte following an instruction.
 *
 * @param chunk  Chunk containing code.
 * @param offset Offset of the instruction byte.
 * @return       Operand byte.
 */
uint8_t get_code(Chunk* chunk, int offset) {
    if (offset < 0 || offset >= chunk->count - 1) {
        printf("Invalid operand access at offset %d\n", offset);
        exit(EXIT_FAILURE);
    }
    return chunk->code[offset + 1];
}

/**
 * Fetch the constant referenced by an instruction.
 *
 * @param chunk  Chunk containing constants.
 * @param offset Offset of the instruction.
 * @return       Constant value.
 */
Value get_constant(Chunk* chunk, int offset) {
    uint8_t constantIndex = get_code(chunk, offset);
    if (constantIndex >= chunk->constants.count) {
        printf("Invalid constant index: %d\n", constantIndex);
        exit(EXIT_FAILURE);
    }
    return chunk->constants.values[constantIndex];
}
