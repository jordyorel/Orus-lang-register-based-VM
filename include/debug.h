#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

/* Register-based VM disassembly helpers */
#include "register_chunk.h"
void disassembleRegisterChunk(RegisterChunk* chunk, const char* name);
int disassembleRegisterInstruction(RegisterChunk* chunk, int offset);

#endif
