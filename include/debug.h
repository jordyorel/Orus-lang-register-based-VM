#ifndef clox_debug_h
#define clox_debug_h

#include "register_chunk.h"
void disassembleRegisterChunk(RegisterChunk* chunk, const char* name);
int disassembleRegisterInstruction(RegisterChunk* chunk, int offset);

#endif
