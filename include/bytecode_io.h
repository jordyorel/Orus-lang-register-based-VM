#ifndef BYTECODE_IO_H
#define BYTECODE_IO_H

/**
 * @file bytecode_io.h
 * @brief Helpers for reading and writing compiled bytecode.
 */

#include "chunk.h"
#include <stdbool.h>

/**
 * Serialize a compiled chunk of bytecode to disk.
 *
 * @param chunk  Chunk to write.
 * @param path   Destination file path.
 * @param mtime  Modification time of the original source file.
 * @return       True on success, false on failure.
 */
bool writeChunkToFile(Chunk* chunk, const char* path, long mtime);

/**
 * Load a bytecode chunk previously written with `writeChunkToFile`.
 *
 * @param path       Source file path.
 * @param out_mtime  Optional pointer to receive the stored modification time.
 * @return           Newly allocated `Chunk` or NULL on error.
 */
Chunk* readChunkFromFile(const char* path, long* out_mtime);

#endif
