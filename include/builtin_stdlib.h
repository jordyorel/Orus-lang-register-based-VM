#ifndef BUILTIN_STDLIB_H
#define BUILTIN_STDLIB_H

typedef struct {
    const char* name;
    const char* source;
} EmbeddedModule;

extern const EmbeddedModule embeddedStdlib[];
extern const int embeddedStdlibCount;
const char* getEmbeddedModule(const char* name);
void dumpEmbeddedStdlib(const char* dir);

#endif
