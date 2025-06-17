/*
 * Bytecode serialization helpers.
 */
#include "../../include/bytecode_io.h"
#include "../../include/memory.h"
#include "../../include/value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORBC_MAGIC 0x4F524243
#define ORBC_VERSION 1

static bool writeValue(FILE* f, Value v) {
    fwrite(&v.type, 1, 1, f);
    switch (v.type) {
        case VAL_I32:  fwrite(&v.as.i32, sizeof(int32_t), 1, f); break;
        case VAL_I64:  fwrite(&v.as.i64, sizeof(int64_t), 1, f); break;
        case VAL_U32:  fwrite(&v.as.u32, sizeof(uint32_t),1,f); break;
        case VAL_U64:  fwrite(&v.as.u64, sizeof(uint64_t),1,f); break;
        case VAL_F64:  fwrite(&v.as.f64, sizeof(double),1,f); break;
        case VAL_BOOL: fwrite(&v.as.boolean, sizeof(bool),1,f); break;
        case VAL_STRING: {
            int len = v.as.string->length;
            fwrite(&len, sizeof(int),1,f);
            fwrite(v.as.string->chars, 1, len, f);
            break;
        }
        case VAL_ARRAY: {
            int len = v.as.array->length;
            fwrite(&len, sizeof(int),1,f);
            for (int i=0;i<len;i++) {
                if (!writeValue(f, v.as.array->elements[i])) return false;
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

static bool readValue(FILE* f, Value* out) {
    uint8_t type;
    if (fread(&type,1,1,f)!=1) return false;
    out->type = (ValueType)type;
    switch (type) {
        case VAL_I32: fread(&out->as.i32,sizeof(int32_t),1,f); break;
        case VAL_I64: fread(&out->as.i64,sizeof(int64_t),1,f); break;
        case VAL_U32: fread(&out->as.u32,sizeof(uint32_t),1,f); break;
        case VAL_U64: fread(&out->as.u64,sizeof(uint64_t),1,f); break;
        case VAL_F64: fread(&out->as.f64,sizeof(double),1,f); break;
        case VAL_BOOL: fread(&out->as.boolean,sizeof(bool),1,f); break;
        case VAL_STRING: {
            int len; if (fread(&len,sizeof(int),1,f)!=1) return false;
            char* buf = malloc(len+1); if (!buf) return false;
            if (fread(buf,1,len,f)!= (size_t)len) { free(buf); return false; }
            buf[len]=0;
            out->as.string = allocateString(buf,len);
            free(buf);
            break;
        }
        case VAL_ARRAY: {
            int len; if (fread(&len,sizeof(int),1,f)!=1) return false;
            ObjArray* arr = allocateArray(len);
            arr->length = len;
            for (int i=0;i<len;i++) {
                if (!readValue(f, &arr->elements[i])) {
                    free(arr->elements);
                    free(arr);
                    return false;
                }
            }
            out->as.array = arr;
            break;
        }
        default:
            return false;
    }
    return true;
}
/**
 * Write a compiled chunk to a binary .obc file.
 */

bool writeChunkToFile(Chunk* chunk, const char* path, long mtime) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    uint32_t magic = ORBC_MAGIC;
    uint32_t ver = ORBC_VERSION;
    fwrite(&magic,sizeof(uint32_t),1,f);
    fwrite(&ver,sizeof(uint32_t),1,f);
    fwrite(&mtime,sizeof(long),1,f);
    fwrite(&chunk->count,sizeof(int),1,f);
    fwrite(&chunk->line_count,sizeof(int),1,f);
    fwrite(&chunk->constants.count,sizeof(int),1,f);
    fwrite(chunk->code,1,chunk->count,f);
    fwrite(chunk->line_info,sizeof(LineInfo),chunk->line_count,f);
    for(int i=0;i<chunk->constants.count;i++) {
        if(!writeValue(f, chunk->constants.values[i])) { fclose(f); return false; }
    }
    fclose(f);
    return true;
}
/**
 * Read a compiled chunk from a .obc file.
 */

Chunk* readChunkFromFile(const char* path, long* out_mtime) {
    FILE* f = fopen(path,"rb");
    if (!f) return NULL;
    uint32_t magic, ver; long mtime; int codeCount,lineCount,constCount;
    if (fread(&magic,sizeof(uint32_t),1,f)!=1 || magic!=ORBC_MAGIC) { fclose(f); return NULL; }
    if (fread(&ver,sizeof(uint32_t),1,f)!=1 || ver!=ORBC_VERSION) { fclose(f); return NULL; }
    if (fread(&mtime,sizeof(long),1,f)!=1) { fclose(f); return NULL; }
    if (out_mtime) *out_mtime = mtime;
    if (fread(&codeCount,sizeof(int),1,f)!=1) { fclose(f); return NULL; }
    if (fread(&lineCount,sizeof(int),1,f)!=1) { fclose(f); return NULL; }
    if (fread(&constCount,sizeof(int),1,f)!=1) { fclose(f); return NULL; }
    Chunk* chunk = malloc(sizeof(Chunk));
    initChunk(chunk);
    chunk->count = codeCount;
    chunk->capacity = codeCount;
    chunk->code = malloc(codeCount);
    fread(chunk->code,1,codeCount,f);
    chunk->line_count = lineCount;
    chunk->line_capcity = lineCount;
    chunk->line_info = malloc(sizeof(LineInfo)*lineCount);
    fread(chunk->line_info,sizeof(LineInfo),lineCount,f);
    for(int i=0;i<constCount;i++) {
        Value v; if(!readValue(f,&v)) { freeChunk(chunk); free(chunk); fclose(f); return NULL; }
        writeValueArray(&chunk->constants,v);
    }
    fclose(f);
    return chunk;
}
